#ifndef _SCHED_H_
#define _SCHED_H_

#include <kernel/sched/process.h>
// kernel.elf kernel supports at most 32 processes
#define NPROC 32
#define TIME_SLICE_LEN  2
extern struct task_struct* current_percpu[NCPU];




#define current (current_percpu[read_tp()])

void init_scheduler();
void insert_to_ready_queue( struct task_struct* proc );
struct task_struct *alloc_empty_process();
void switch_to(struct task_struct*);
void schedule();
struct task_struct *find_process_by_pid(pid_t pid);
void set_current_task(struct task_struct* task);



#endif
