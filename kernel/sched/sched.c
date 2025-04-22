/*
 * implementing the scheduler
 */

#include <kernel/mm/kmalloc.h>
#include <kernel/mm/mm_struct.h>
#include <kernel/sched/pid.h>
#include <kernel/sched/sched.h>
#include <kernel/sched/task.h>
#include <kernel/trapframe.h>
#include <kernel.h>
#include <kernel/util/list.h>
#include <kernel/util/string.h>

struct list_head ready_queue;
task_t* procs[NPROC];

//
// initialize process pool (the procs[] array). added @lab3_1
//
void init_scheduler() {
	// kprintf("init_scheduler: start\n");
	INIT_LIST_HEAD(&ready_queue);
	pid_init();
	memset(procs, 0, sizeof(task_t*) * NPROC);

	for (int32 i = 0; i < NPROC; ++i) {
		procs[i] = NULL;
	}
	kprintf("Scheduler initiated\n");
}

task_t* alloc_task() {
	for (int32 i = 0; i < NPROC; i++) {
		if (procs[i] == NULL) {
			procs[i] = (task_t*)kmalloc(sizeof(task_t));
			memset(procs[i], 0, sizeof(task_t));
			return procs[i];
		}
	}

	panic("cannot find any free process structure.\n");
	return NULL;
}
//
// insert a process, proc, into the END of ready queue.
//
void insert_to_ready_queue(task_t* proc) {
	kprintf("going to insert process %d to ready queue.\n", proc->pid);
	list_add_tail(&proc->ready_queue_node, &ready_queue);
}

//
// choose a proc from the ready queue, and put it to run.
// note: schedule() does not take care of previous current process. If the
// current process is still runnable, you should place it into the ready queue
// (by calling ready_queue_insert), and then call schedule().
//
void schedule() {
	kprintf("schedule: start\n");
	if (((current->state == TASK_INTERRUPTIBLE) | (current->state == TASK_UNINTERRUPTIBLE)) && current->schedule_context == NULL) {
		current->schedule_context = (struct trapframe*)kmalloc(sizeof(struct trapframe));
		store_all_registers(current->schedule_context);
	}
	set_current_task(list_first_entry(&ready_queue, task_t, ready_queue_node));
	list_del(ready_queue.next);
	kprintf("going to schedule process %d to run in s-mode.\n", current->pid);

	restore_all_registers(current->schedule_context);
	return;
}

void switch_to_user() {
	current->kernel_sp = ROUNDUP(read_reg(sp),PAGE_SIZE);     // process's kernel stack
	write_csr(sstatus, read_csr(sstatus) & ~SSTATUS_SPP);
	kprintf("return to user\n");
	extern void return_to_context();
	return_to_context();
}

/**
 * find_process_by_pid - Find a process by its PID
 * @pid: Process ID to search for
 *
 * Searches the process table for a process with the given PID.
 *
 * Returns: Pointer to the task_struct if found, NULL if not found
 */
task_t* find_process_by_pid(pid_t pid) {
	if (pid <= 0) return NULL;

	// Search the process table
	for (int32 i = 0; i < NPROC; i++) {
		if (procs[i] && procs[i]->pid == pid) {
			return procs[i];
		}
	}

	return NULL; // Process not found
}