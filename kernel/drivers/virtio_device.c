#include <kernel.h>
#include <kernel/device/blockdevice.h>
#include <kernel/device/buffer_head.h>
#include <kernel/drivers/virtio_device.h>
#include <kernel/drivers/virtio_mmio.h>
#include <kernel/riscv.h>
#include <kernel/vfs.h>

struct blockdevice virtio_bd;
struct block_operations virtio_ops = {
    .read_blocks = virtio_read_blocks,
    .write_blocks = virtio_write_blocks,
    // .open = virtio_open,
    // .release = virtio_release,
    // .ioctl = virtio_ioctl,
};

void init_virtio_bd(void) {
	kprintf("init_virtio_bd: start\n");
	virtio_bd.bd_dev = 0;
	virtio_bd.bd_openers = 0;
	virtio_bd.bd_inode = NULL; // 这些字段都是第一次挂载时设置的
	virtio_bd.bd_super = NULL;
	list_add(&virtio_bd.bd_list, &block_device_list);
	virtio_bd.bd_block_size = 512; // virtio disk的块大小
	virtio_bd.bd_nr_blocks = 0;    // 这个值在挂载时设置
	atomic_set(&virtio_bd.bd_refcnt, 1);
	spinlock_init(&virtio_bd.bd_lock);
	kprintf("init_virtio_bd: end\n");
}

int32 virtio_read_blocks(struct blockdevice* bdev, void* buffer, sector_t sector, size_t blockcount) {
	bdev; //只有一个virtio disk，所以不需要判断
	for (int i = 0; i < blockcount; i++) {
		struct buffer_head* bh = buffer_alloc(&virtio_bd, sector + i);
		CHECK_PTR_VALID(bh, PTR_TO_ERR(bh));
		virtio_disk_rw(bh, READ);
		size_t offset = i * bdev->bd_block_size;
		memcpy(buffer + offset, bh->b_data + offset, bdev->bd_block_size);
	}
	return 0;
}

int32 virtio_write_blocks(struct blockdevice* bdev, const void* buffer, sector_t sector, size_t count) {
	bdev; //只有一个virtio disk，所以不需要判断
	for (int i = 0; i < ROUNDUP(count / bdev->bd_block_size, bdev->bd_block_size); i++) {
		struct buffer_head* bh = buffer_alloc(&virtio_bd, sector + i);
		CHECK_PTR_VALID(bh, PTR_TO_ERR(bh));

		size_t offset = i * bdev->bd_block_size;

		memcpy(bh->b_data + offset, buffer + offset, bdev->bd_block_size);

		virtio_disk_rw(bh, WRITE);
	}
	return 0;
}