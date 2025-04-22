#pragma once
#include <kernel/mm/memlayout.h>
void plic_init(void);
void plic_init_hart(void);
int plic_claim(void);
void plic_complete(int);
