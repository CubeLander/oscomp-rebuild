#include <kernel/device/buffer_head.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/types.h>
#include <kernel/util/hashtable.h>
#include <kernel/util/list.h>
#include <kernel/util/string.h>

#include <kernel/util/print.h>

// 分配一个新的 buffer_head
// struct buffer_head *buffer_alloc(gfp_t gfp_flags) {
struct buffer_head* buffer_alloc(struct blockdevice* bdev, sector_t block) {
	struct buffer_head* bh;
	// bh = kmalloc(sizeof(struct buffer_head), gfp_flags);
	bh = kmalloc(sizeof(struct buffer_head));
	CHECK_PTR_VALID(bh, ERR_TO_PTR(-ENOMEM));
	memset(bh, 0, sizeof(struct buffer_head));

	bh->b_bdev = bdev;
	bh->b_size = bdev->bd_block_size;
	// bh->b_data = kmalloc(size, GFP_KERNEL);
	bh->b_data = kmalloc(bdev->bd_block_size);
	if(PTR_IS_ERROR(bh->b_data)){
		kfree(bh);
		return ERR_TO_PTR(-ENOMEM);
	}
	return bh;
}

// 释放对缓冲区的引用
void buffer_free(struct buffer_head* bh) {
	if (!bh) return;
	kfree(bh->b_data);
	if (bh->b_private) {
		kfree(bh->b_private);
	}
	kfree(bh);
}

// 标记缓冲区为脏
void mark_buffer_dirty(struct buffer_head* bh) {
	if (bh) set_buffer_dirty(bh);
}

// 提交一个脏缓冲区
int32 sync_dirty_buffer(struct buffer_head* bh) {
	int32 ret = 0;

	if (!buffer_dirty(bh)) return 0;

	lock_buffer(bh);
	if (buffer_dirty(bh)) {
		ret = bh->b_bdev->bd_ops->write_blocks(bh->b_bdev, bh->b_data, bh->b_blocknr, 1);
		if (ret == 0) clear_buffer_dirty(bh);
	}
	unlock_buffer(bh);

	return ret;
}

// 锁住一个缓冲区
void lock_buffer(struct buffer_head* bh) {
	spinlock_lock(&bh->b_lock);
	set_buffer_locked(bh);
}

// 解锁一个缓冲区
void unlock_buffer(struct buffer_head* bh) {
	clear_buffer_locked(bh);
	spinlock_unlock(&bh->b_lock);
}

// 等待缓冲区操作完成
void wait_on_buffer(struct buffer_head* bh) {
	while (buffer_locked(bh)) {
		// 在实际实现中，这里应该使用睡眠/唤醒机制
		// 简单起见，使用简单的轮询
		// yield_cpu(); // 让出CPU时间
	}
}