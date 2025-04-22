#pragma once
#include <kernel/types.h>


typedef struct task task_t;

typedef struct cpuinfo{
	task_t *current_task; // 当前运行的进程
	uint32 hart_id;
}cpuinfo_t;


#define current (current_task())
static inline task_t* current_task(){
	cpuinfo_t* cpuinfo = (cpuinfo_t*)read_csr(sscratch);
	return cpuinfo->current_task;
}

#define hartid (current_hart())
static inline uint32 current_hart(){
	cpuinfo_t* cpuinfo = (cpuinfo_t*)read_csr(sscratch);
	return cpuinfo->hart_id;
}

static inline void set_current_task(task_t* new_task){
	cpuinfo_t* cpuinfo = (cpuinfo_t*)read_csr(sscratch);
	cpuinfo->current_task = new_task;
}

