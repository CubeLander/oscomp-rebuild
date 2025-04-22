// See LICENSE for license details.
// borrowed from https://github.com/riscv/riscv-pk:
// machine/atomic.h

#ifndef _RISCV_SPINLOCK_H_
#define _RISCV_SPINLOCK_H_
#include <stdint.h>
#include <kernel/util/atomic.h>
#include <kernel/riscv.h>


typedef struct spinlock{
  atomic_flag lock;
  uint64 irq_flags;
} spinlock_t;

#define SPINLOCK_INIT  ATOMIC_FLAG_INIT 

static inline void spinlock_init(spinlock_t* lock) {
	atomic_flag_clear(&lock->lock);
}

static inline int32 spinlock_trylock(spinlock_t* lock) {
    return atomic_flag_test_and_set(&lock->lock) == 0;  // 成功返回 1
}


static inline void spinlock_lock(spinlock_t* lock) {
	
    while (atomic_flag_test_and_set(&lock->lock)) {
        // 自旋等待
    }
	lock->irq_flags = read_sstatus();
	write_csr(sstatus, lock->irq_flags & ~0x2); // 设置 SIE (bit 1)
}

static inline void spinlock_unlock(spinlock_t* lock) {
	int64 flags = lock->irq_flags;
    atomic_flag_clear(&lock->lock);
	write_csr(sstatus, flags); 	// 恢复到原来的 SIE (bit 1)
	// 注意，这里的flags临时变量字段很关键，这能让sstatus正确恢复到之前
	// 保存的irq_flags，而不被其他线程忙等锁完，冲进来改掉。
}

#endif
