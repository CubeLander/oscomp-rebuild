#pragma once
#include <kernel/types.h>


typedef struct task task_t;

typedef struct cpu{
	task_t *current_task; // 当前运行的进程
	uint32 hartid;
}cpu_t;