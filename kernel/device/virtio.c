#include <kernel/device/virtio.h>
//#include <lock/mutex.h>
#include <kernel/device/buffer_head.h>
#include <kernel/util.h>
#include <kernel/mmu.h>
//#include <param.h>
#include <kernel/riscv.h>
#include <kernel/types.h>
#include <kernel/device/block_device.h>

// the address of virtio mmio register r.
#define R(r) ((volatile uint32_t *)(VIRTIO0 + (r)))
#define availOffset (sizeof(struct virtq_desc) * VIRTIO_DESCIPTOR_NUM)
void *virtioDriverBuffer;


static struct disk {

	struct virtq_desc *desc;
	struct virtq_avail *avail;
	struct virtq_used *used;					// 上面三个都是virtio_disk协议的数据结构
	char free[VIRTIO_DESCIPTOR_NUM];	// 用来描述请求队列的状态
	struct buffer_head* buffers[VIRTIO_DESCIPTOR_NUM];
	uint16_t used_idx;

} disk;

void virtio_disk_init(void) {
	uint32_t status = 0;

	// 检查设备的魔术值、版本、设备ID和厂商ID，确保找到了virtio磁盘设备。如果条件不满足，会触发panic
	if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 || *R(VIRTIO_MMIO_VERSION) != 1 ||
	    *R(VIRTIO_MMIO_DEVICE_ID) != 2 || *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551) {
		panic("could not find virtio disk");
	}

	// 将状态寄存器置零，用于重置设备
	*R(VIRTIO_MMIO_STATUS) = status;

	// 设置ACKNOWLEDGE和DRIVER状态位，表示驱动程序已经意识到设备的存在，并准备开始驱动
	status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
	*R(VIRTIO_MMIO_STATUS) = status;

	status |= VIRTIO_CONFIG_S_DRIVER;
	*R(VIRTIO_MMIO_STATUS) = status;

	// 协商设备和驱动程序所支持的特性，
	// 将驱动程序支持的特性写入VIRTIO_MMIO_DRIVER_FEATURES寄存器。
	uint64_t features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
	features &= ~(1 << VIRTIO_BLK_F_RO);
	features &= ~(1 << VIRTIO_BLK_F_SCSI);
	features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
	features &= ~(1 << VIRTIO_BLK_F_MQ);
	features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
	features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
	features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
	*R(VIRTIO_MMIO_DRIVER_FEATURES) = features;


	// 选择队列0进行初始化
	*R(VIRTIO_MMIO_QUEUE_SEL) = 0;

	// 确保队列0未被使用。如果队列0已被使用，触发panic
	if (*R(VIRTIO_MMIO_QUEUE_PFN) != 0)
		panic("virtio disk should not be ready");

	// 检查最大队列大小。如果最大队列大小为0，触发panic。如果最大队列大小小于NUM（预定义的队列大小），触发panic
	uint32_t max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (max == 0)
		panic("virtio disk has no queue 0");
	if (max < VIRTIO_DESCIPTOR_NUM)
		panic("virtio disk max queue too short");

	// 输入page size
	*R(VIRTIO_MMIO_PAGE_SIZE) = PAGE_SIZE;

	// 分配内存用于存储队列相关的描述符（desc）、可用环（avail）和已使用环（used）。如果分配失败，触发panic。然后使用memset将分配的内存清零
	disk.desc = (void *)virtioDriverBuffer;
	disk.avail = (void *)((uint64_t)disk.desc + availOffset);
	disk.used = (void *)((uint64_t)disk.desc + PAGE_SIZE);

	// 这里后续考虑用一次alloc()

	if (!disk.desc || !disk.used)
		panic("virtio disk kalloc");
	memset(disk.desc, 0, PAGE_SIZE);
	memset(disk.used, 0, PAGE_SIZE);

	// 设置队列的大小
	*R(VIRTIO_MMIO_QUEUE_NUM) = VIRTIO_DESCIPTOR_NUM;

	// 设置Queue Align
	*R(VIRTIO_MMIO_QUEUE_ALIGN) = PAGE_SIZE;

	// 设置QUEUE PFN
	*R(VIRTIO_MMIO_QUEUE_PFN) = (uint64_t)disk.desc >> 12;


	// 将所有的NUM个描述符设置为未使用状态
	for (int i = 0; i < VIRTIO_DESCIPTOR_NUM; i++)
		disk.free[i] = 1;

	// 设置DRIVER_OK状态位，告诉设备驱动程序已准备完毕
	status |= VIRTIO_CONFIG_S_DRIVER_OK;
	*R(VIRTIO_MMIO_STATUS) = status;
}

static int alloc_desc() {
	for (int i = 0; i < VIRTIO_DESCIPTOR_NUM; i++) {
		if (disk.free[i]) {
			disk.free[i] = 0;
			return i;
		}
	}
	return -1;
}

static void free_desc(int i) {
	if (i >= VIRTIO_DESCIPTOR_NUM)
		panic("free number is above maximu");
	if (disk.free[i])
		panic("desc is already freed");
	disk.desc[i].addr = 0;
	disk.desc[i].len = 0;
	disk.desc[i].flags = 0;
	disk.desc[i].next = 0;
	disk.free[i] = 1;
}

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

static int alloc3_desc(int *idx) {
	for (int i = 0; i < 3; i++) {
		idx[i] = alloc_desc();
		if (idx[i] < 0) {
			for (int j = 0; j < i; j++)
				free_desc(idx[j]);
			return -1;
		}
	}
	return 0;
}

/**
 * @brief virtio读写接口，此函数使用忙等策略，若磁盘未准备好，则驱动会一直阻塞等待到磁盘就绪。
 * 		  调用示例可以参见virtioTest函数
 * @param b
 * 要读或写的缓冲区描述符（定义在fs/buf.h）。在调用之前，b->blockno需要设置为要读或写的扇区号，
 * 		  每个扇区512字节。若为读取，调用此函数后，b->data的内容就是对应扇区的数据；若为写入，则b->data
 * 		  需要提前写入要写入的数据
 * @param write 是否读。设为0表示读取，1表示写入
 */
void virtio_disk_rw(struct buffer_head* buffer, int write) {

	// the spec's Section 5.2 says that legacy block operations use
	// three descriptors: one for type/reserved/sector, one for the
	// data, one for a 1-byte status result.

	// allocate the three descriptors.
	int idx[3];
	while (1) {
		if (alloc3_desc(idx) == 0) {
			break;
		}
		panic("there is no free des");
	}

	// format the three descriptors.
	// qemu's virtio-blk.c reads them.
	struct virtio_blk_req request = {
		.type = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN,
		.reserved = 0,
		.sector = buffer->b_blocknr,
	};

	disk.desc[idx[0]].addr = (uint64)&request;
	disk.desc[idx[0]].len = sizeof(request);
	disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
	disk.desc[idx[0]].next = idx[1];
	//disk.info[idx[0]].status = 0xff; // device writes 0 on success

	assert(buffer->b_io == 0xff);
	disk.desc[idx[1]].addr = (uint64)&buffer->b_io;
	disk.desc[idx[1]].len = buffer->b_size;
	disk.desc[idx[1]].flags = write? 0 : VRING_DESC_F_WRITE; // device reads b->data
	// write=1时用b->data写device，write=0时用device写b->data
	// 这里的VRING_DESC_F_WRITE标志指设备写入数据，和write参数正好相反
	disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
	disk.desc[idx[1]].next = idx[2];

	disk.desc[idx[2]].addr = (uint64_t)&buffer->b_io;
	disk.desc[idx[2]].len = 1;
	disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
	disk.desc[idx[2]].next = 0;


	// tell the device the first index in our chain of descriptors.
	// 然后硬盘就会做同样的动作
	//	读disk.avail->idx
	// 	读disk.avail->ring[disk.avail->idx % VIRTIO_DESCIPTOR_NUM], 得到idx[0]
	//  通过idx[0] 读disk.desc[idx[0]]，拿到request和next=idx[1]
	// 	读disk.desc[idx[1]]，拿到data，len next=idx[2]

	disk.avail->ring[disk.avail->idx % VIRTIO_DESCIPTOR_NUM] = idx[0];
	disk.avail->idx += 1;

	__sync_synchronize();

	*R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number

	kprintf("enter virtio wait!\n");

	while (buffer->b_io) {

		// 在这里理论上
		continue;
		// 先忙等，等到做了mutex再做阻塞
	}

	kprintf("exit virtio wait!\n");

	free_chain(idx[0]);
}

/**
 * @brief virtio驱动的中断处理函数
 */
void virtio_trap_handler() {
	*R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

	__sync_synchronize();

	// disk.used->idx 是由设备维护的
	while (disk.used_idx != disk.used->idx) {
		int id = disk.used->ring[disk.used_idx % VIRTIO_DESCIPTOR_NUM].id;
		// 这个id就是传入请求时的idx[0]

		__sync_synchronize();
		disk.used_idx += 1;
	}

	kprintf("virtioTest: finish virtio intr\n");
}

int32 virtio_disk_read(struct block_device* bdev, struct buffer_head* buffer) {
	virtio_disk_rw(buffer, 0);
	return 0;
}

int32 virtio_disk_write(struct block_device* bdev, struct buffer_head* buffer) {
	virtio_disk_rw(buffer, 1);
	return 0;
}



const struct block_operations virtio_disk_ops = {
	.read_block = virtio_disk_read,
	.write_block = virtio_disk_write,
	.open = NULL,
	.release = NULL,
	.ioctl = NULL,
};

struct block_device virtio_disk = {
	.bd_dev = 0x12345678,
	.bd_openers = 0,
	.bd_inode = NULL,
	.bd_super = NULL,
	.bd_list = {NULL, NULL},
	.bd_block_size = VIRTIO_SECTOR_SIZE,
	.bd_nr_blocks = 0,				// 在解析设备树时设置
	.bd_private = NULL,
	.bd_mode = 0,
	.bd_ops = &virtio_disk_ops,


};