#include <kernel.h>

struct mutex* mutex_alloc(const char* name) {
	if (name == NULL) {
		panic("mutex_alloc: name is NULL");
	}
	struct mutex* new_mutex = kmalloc(sizeof(struct mutex));
	CHECK_PTR_VALID(new_mutex, ERR_TO_PTR(-ENOMEM));
	atomic_set(&new_mutex->owner, -1);
	INIT_LIST_HEAD(&new_mutex->wait_queue);
	spinlock_init(&new_mutex->wait_lock);
	new_mutex->name = kstrdup(name);
	return new_mutex;
}

void mutex_free(struct mutex* mtx) {
	if (mtx == NULL) {
		panic("mutex_free: mtx is NULL");
	}
	kfree((void*)mtx->name);
	kfree(mtx);
}

int32 mutex_lock(struct mutex* mtx) {
	CHECK_PTR_VALID(mtx, -EINVAL);
	kprintf("mutex_lock: trying to lock mutex = %s\n", mtx->name);
	if (atomic_read(&mtx->owner) == -1) {
		atomic_set(&mtx->owner, current->pid);
	} else {
		kprintf("mutex_lock: blocking pid = %d\n", current->pid);
		current->state = TASK_INTERRUPTIBLE; 
		spinlock_lock(&mtx->wait_lock);
		list_add_tail(&current->wait_queue_node, &mtx->wait_queue);
		spinlock_unlock(&mtx->wait_lock);
		schedule();
		// 直接嵌入保存内核上下文到process->ktrapframe的汇编代码
	}
	return 0;
}

// 我们这里为了实现简单，直接把信号队列当成信号栈使了。
// 如果说有进程等待信号量，返回0，不然返回增加后的信号资源量sem->value
int32 mutex_unlock(struct mutex* mtx) {
	CHECK_PTR_VALID(mtx, -EINVAL);
	spinlock_lock(&mtx->wait_lock);
	task_t* waiting_task = list_first_entry(&mtx->wait_queue, task_t, wait_queue_node);
	if(waiting_task == NULL) {
		atomic_set(&mtx->owner, -1);
	} else {
		kprintf("mutex_unlock: waking up pid = %d\n", waiting_task->pid);
		waiting_task->state = TASK_RUNNING;
		list_del(&current->wait_queue_node);
		insert_to_ready_queue(waiting_task);
		atomic_set(&mtx->owner, -1);
	}
	spinlock_unlock(&mtx->wait_lock);
	return 0;
}