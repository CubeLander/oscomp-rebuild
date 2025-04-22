#pragma once

#include <kernel/types.h>
#include <kernel.h>

struct mutex {
	atomic_t owner;	// 维护持有锁的pid，-1为空
	struct list_head wait_queue;
	spinlock_t wait_lock;
	const char* name; // 锁的名字
};

struct mutex* mutex_alloc(const char* name);
void mutex_free(struct mutex*);
int32 mutex_lock(struct mutex*);
int32 mutex_unlock(struct mutex*);
