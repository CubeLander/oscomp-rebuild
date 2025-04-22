#ifndef _SCHED_H_
#define _SCHED_H_

#include <kernel/sched/task.h>
#include <kernel/sched/cpu.h>
#include <kernel/riscv.h>
// kernel.elf kernel supports at most 32 processes
#define NPROC 32
#define TIME_SLICE_LEN  2



void init_scheduler();
void insert_to_ready_queue( task_t* proc );
task_t *alloc_empty_process();
void switch_to(task_t*);
void schedule();
task_t *find_process_by_pid(pid_t pid);
void set_current_task(task_t* task);



#endif
