//
// driver for qemu's virtio disk device.
// uses qemu's mmio interface to virtio.
//
// qemu ... -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
//

#include <kernel.h>
#include <kernel/device/blockdevice.h>
#include <kernel/device/buffer_head.h>
#include <kernel/drivers/virtio_mmio.h>
#include <kernel/mm/pagetable.h>
#include <kernel/riscv.h>
#include <kernel/util.h>
#include <kernel/vfs.h>

// the address of virtio mmio register r.
#define R(r) ((volatile uint32*)(VIRTIO0 + (r)))
struct virtio_blk_config blk_cfg;
static struct disk {
	// the virtio driver and device mostly communicate through a set of
	// structures in RAM. pages[] allocates that memory. pages[] is a
	// global (instead of calls to kalloc()) because it must consist of
	// two contiguous pages of page-aligned physical memory.
	char pages[2 * PAGE_SIZE];
	// a set (not a ring) of DMA descriptors, with which the
	// driver tells the device where to read and write individual
	// disk operations. there are NUM descriptors.
	// most commands consist of a "chain" (a linked list) of a couple of
	// these descriptors.
	struct virtq_desc* desc;

	// a ring in which the driver writes descriptor numbers
	// that the driver would like the device to process.  it only
	// includes the head descriptor of each chain. the ring has
	// NUM elements.
	struct virtq_avail* avail;

	// a ring in which the device writes descriptor numbers that
	// the device has finished processing (just the head of each chain).
	// there are NUM used ring entries.
	struct virtq_used* used;

	// our own book-keeping.
	char free[NUM];  // is a descriptor free?
	uint16 used_idx; // we've looked this far in used[2..NUM].

	// track info about in-flight operations,
	// for use when completion interrupt arrives.
	// indexed by first descriptor index of chain.
	struct {
		struct buffer_head* b;
		char status;
	} info[NUM];

	// disk command headers.
	// one-for-one with descriptors, for convenience.
	struct virtio_blk_req ops[NUM];

	spinlock_t vdisk_lock;

} __attribute__((aligned(PAGE_SIZE))) disk;

struct mutex* disk_mutex = NULL;

void virtio_disk_init(void) {
	uint32 status = 0;

	status = pgt_map_pages(g_kernel_pagetable, VIRTIO0, VIRTIO0, PAGE_SIZE, prot_to_type(PROT_READ | PROT_WRITE, 0));
	if (status) {
		kprintf("virtio_disk_init: virtio disk mmap failed with %d\n", status);
		return;
	}

	spinlock_init(&disk.vdisk_lock);

	if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 || *R(VIRTIO_MMIO_VERSION) != 1 || *R(VIRTIO_MMIO_DEVICE_ID) != 2 || *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551) {
		kprintf("could not find virtio disk\n");
	}

	blk_cfg = *(struct virtio_blk_config*)R(VIRTIO_MMIO_BLK_CONFIG);

	// // reset device
	// *R(VIRTIO_MMIO_STATUS) = status;

	// // set ACKNOWLEDGE status bit
	status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
	*R(VIRTIO_MMIO_STATUS) = status;

	// set DRIVER status bit
	status |= VIRTIO_CONFIG_S_DRIVER;
	*R(VIRTIO_MMIO_STATUS) = status;

	// negotiate features
	uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
	features &= ~(1 << VIRTIO_BLK_F_RO);
	features &= ~(1 << VIRTIO_BLK_F_SCSI);
	features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
	features &= ~(1 << VIRTIO_BLK_F_MQ);
	features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
	features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
	features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
	*R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

	// tell device that feature negotiation is complete.
	status |= VIRTIO_CONFIG_S_FEATURES_OK;
	*R(VIRTIO_MMIO_STATUS) = status;

	// tell device we're completely ready.
	status |= VIRTIO_CONFIG_S_DRIVER_OK;
	*R(VIRTIO_MMIO_STATUS) = status;

	*R(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PAGE_SIZE;
	// initialize queue 0.
	*R(VIRTIO_MMIO_QUEUE_SEL) = 0;

	const uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (max == 0) panic("virtio disk has no queue 0");
	if (max < NUM) panic("virtio disk max queue too short");

	*R(VIRTIO_MMIO_QUEUE_NUM) = NUM;

	memset(disk.pages, 0, sizeof(disk.pages));
	*R(VIRTIO_MMIO_QUEUE_PFN) = ((uint64)disk.pages) >> PAGE_SHIFT;

	disk.desc = (struct virtq_desc*)disk.pages;
	disk.avail = (struct virtq_avail*)(disk.pages + NUM * sizeof(struct virtq_desc));
	disk.used = (struct virtq_used*)(disk.pages + PAGE_SIZE);

	// all NUM descriptors start out unused.
	for (int i = 0; i < NUM; i++) disk.free[i] = 1;

	// plic.c and trap.c arrange for interrupts from VIRTIO0_IRQ.
	kprintf("virtio_disk_init: success\n");

	kprintf("virtio_blk_config: capacity %d blk_size %d\n", blk_cfg.capacity, blk_cfg.blk_size);
	kprintf("virtio_blk_config: size %d seg_max %d\n", blk_cfg.size_max, blk_cfg.seg_max);
	kprintf("virtio_blk_config: geometry: heads %d cylinders %d sectors %d\n", blk_cfg.geometry.heads, blk_cfg.geometry.cylinders, blk_cfg.geometry.sectors);
}

// find a free descriptor, mark it non-free, return its index.
static int alloc_desc() {
	for (int i = 0; i < NUM; i++) {
		if (disk.free[i]) {
			disk.free[i] = 0;
			return i;
		}
	}
	return -1;
}

// mark a descriptor as free.
static void free_desc(int i) {
	if (i >= NUM) panic("free_desc 1");
	if (disk.free[i]) panic("free_desc 2");
	disk.desc[i].addr = 0;
	disk.desc[i].len = 0;
	disk.desc[i].flags = 0;
	disk.desc[i].next = 0;
	disk.free[i] = 1;
	// wakeup(&disk.free[0]);
}

// free a chain of descriptors.
static void free_chain(int i) {
	while (1) {
		int flag = disk.desc[i].flags;
		int nxt = disk.desc[i].next;
		free_desc(i);
		if (flag & VRING_DESC_F_NEXT)
			i = nxt;
		else
			break;
	}
}

// allocate three descriptors (they need not be contiguous).
// disk transfers always use three descriptors.
static int alloc3_desc(int* idx) {
	for (int i = 0; i < 3; i++) {
		idx[i] = alloc_desc();
		if (idx[i] < 0) {
			for (int j = 0; j < i; j++) free_desc(idx[j]);
			return -1;
		}
	}
	return 0;
}

void virtio_disk_rw(struct buffer_head* b, int write) {
	kprintf("virtio_disk_rw: %s %d\n", write ? "write" : "read", b->b_blocknr);
	uint64 sector = b->b_blocknr * (b->b_size / 512);

	//spinlock_lock(&disk.vdisk_lock);

	// the spec's Section 5.2 says that legacy block operations use
	// three descriptors: one for type/reserved/sector, one for the
	// data, one for a 1-byte status result.

	// allocate the three descriptors.
	int idx[3];
	while (1) {
		if (alloc3_desc(idx) == 0) {
			break;
		}
		kprintf("virtio_disk_rw: alloc failead\n");
		// sleep(&disk.free[0], &disk.vdisk_lock);
	}

	// format the three descriptors.
	// qemu's virtio-blk.c reads them.

	struct virtio_blk_req* buf0 = &disk.ops[idx[0]];

	if (write)
		buf0->type = VIRTIO_BLK_T_OUT; // write the disk
	else
		buf0->type = VIRTIO_BLK_T_IN; // read the disk
	buf0->reserved = 0;
	buf0->sector = sector;

	disk.desc[idx[0]].addr = (uint64)buf0;
	disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
	disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
	disk.desc[idx[0]].next = idx[1];

	disk.desc[idx[1]].addr = (uint64)b->b_data;
	disk.desc[idx[1]].len = b->b_size;
	if (write)
		disk.desc[idx[1]].flags = 0; // device reads b->data
	else
		disk.desc[idx[1]].flags = VRING_DESC_F_WRITE; // device writes b->data
	disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
	disk.desc[idx[1]].next = idx[2];

	disk.info[idx[0]].status = 0xff; // device writes 0 on success
	disk.desc[idx[2]].addr = (uint64)&disk.info[idx[0]].status;
	disk.desc[idx[2]].len = 1;
	disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
	disk.desc[idx[2]].next = 0;

	// record struct buffer_head for virtio_disk_intr().
	b->b_end_io = 1;
	disk.info[idx[0]].b = b;

	// tell the device the first index in our chain of descriptors.
	disk.avail->ring[disk.avail->idx % NUM] = idx[0];

	__sync_synchronize();

	// tell the device another avail ring entry is available.
	disk.avail->idx += 1; // not % NUM ...

	__sync_synchronize();

	*R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number

	// Wait for virtio_disk_intr() to say request has finished.
	while (b->b_end_io == 1) {
		// sleep(b, &disk.vdisk_lock);
		//  目前是忙等
	}

	disk.info[idx[0]].b = 0;
	free_chain(idx[0]);

}

void virtio_disk_intr() {
	spinlock_lock(&disk.vdisk_lock);

	// the device won't raise another interrupt until we tell it
	// we've seen this interrupt, which the following line does.
	// this may race with the device writing new entries to
	// the "used" ring, in which case we may process the new
	// completion entries in this interrupt, and have nothing to do
	// in the next interrupt, which is harmless.
	*R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

	__sync_synchronize();

	// the device increments disk.used->idx when it
	// adds an entry to the used ring.

	while (disk.used_idx != disk.used->idx) {
		__sync_synchronize();
		int id = disk.used->ring[disk.used_idx % NUM].id;

		if (disk.info[id].status != 0) panic("virtio_disk_intr status");

		struct buffer_head* b = disk.info[id].b;
		b->b_end_io = 0; // disk is done with buf
		// wakeup(b);
		// TODO:加入调度器的支持

		disk.used_idx += 1;
		kprintf("virtio_disk_intr: write disk.used_idx\n");
	}

	spinlock_unlock(&disk.vdisk_lock);
}
