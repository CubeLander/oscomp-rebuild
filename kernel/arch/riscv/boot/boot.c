/*
 * Supervisor-mode startup codes
 */

#include <kernel/boot/dtb.h>
#include <kernel/device/sbi.h>
#include <kernel/elf.h>
#include <kernel/mmu.h>
#include <kernel/riscv.h>
#include <kernel/sched.h>
#include <kernel/syscall/syscall.h>
#include <kernel/types.h>
#include <kernel/util.h>
#include <kernel/vfs.h>

#define __boot_code __attribute__((section(".boot_text")))
//#define __boot_code 
#define __boot_data __attribute__((section(".boot_data")))
//#define __boot_data 

__boot_data __attribute__((aligned(PAGE_SIZE))) char stack0[(NCPU * 2 + 1)  *  PAGE_SIZE];

__boot_code void early_sync_barrier(volatile int32* counter, int32 all);
__boot_code void early_write_tp(uint64 x);
__boot_code void* early_memset(void* dest, int byte, size_t len);
__boot_code int32 early_map_pages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 size, int32 perm);
__boot_code int32 early_map_page(pagetable_t pagetable, vaddr_t va, paddr_t pa, int32 perm);
__boot_code pte_t* early_page_walk(pagetable_t pagetable, uint64 va, int32 alloc);
__boot_code uint64 early_prot_to_type(int32 prot, int32 user);
__boot_code void* early_alloc_page(void);

extern char _ftext[], _etext[], _fdata[], _end[];
 extern char _g_pagetable_va[], _g_pagetable_pa[];
//__attribute__((aligned(PAGE_SIZE))) char _g_pagetable_pa[PAGE_SIZE];

#define KERN_VA_TO_PA(x) ((uint64)(((uint64)x) - 0xff000000 + 0x80000000))
__boot_code void early_vm_init(void) {
	// extern struct mm_struct init_mm;
	//  映射内核代码段和只读段
	g_kernel_pagetable = (pagetable_t)_g_pagetable_va;
	pagetable_t pagetable_phaddr = (pagetable_t)_g_pagetable_pa;
	// init_mm.pagetable = pagetable_phaddr;
	//  之后它会被加入内核的虚拟空间，先临时用一个页
	early_memset(pagetable_phaddr, 0, PAGE_SIZE);
	//kprintf("early_vm_init: pagetable_phaddr = %lx\n", pagetable_phaddr);
	early_map_pages(pagetable_phaddr, 0x80200000, 0x80200000, ((uint64)memInfo.size - 0x200000), early_prot_to_type(PROT_READ | PROT_EXEC | PROT_WRITE, 0));
	//kprintf("early_vm_init: stage 1 done = %lx\n", pagetable_phaddr);

	// //kprintf("_etext=%lx,_ftext=%lx\n", _etext, _ftext);
	//early_map_pages(pagetable_phaddr, 0xff000000, 0x80000000, ((uint64)_end - 0xff000000), early_prot_to_type(PROT_READ | PROT_EXEC | PROT_WRITE, 0));
	//kprintf("early_vm_init: stage 2 done = %lx\n", pagetable_phaddr);
	//early_map_pages(pagetable_phaddr, 0xbf000000, 0xbf000000, PAGE_SIZE, early_prot_to_type(PROT_READ | PROT_EXEC | PROT_WRITE, 0));

}

__boot_code void start_trap() { while (1); }
__boot_data struct trapframe boot_trapframe;
// 这个boot_trapframe应该给每个核都发一个
// 这个boot_trapframe应该给每个核都发一个

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
__boot_data volatile int32 sig = 1;
__boot_data volatile int counter = 0;

__boot_code void early_boot(uintptr_t hartid, uintptr_t dtb) {
	write_tp(hartid);
	//kprintf("In early_boot, hartid:%d\n", hartid);
	SBI_PUTCHAR('0' + hartid);
	//write_csr(sstatus, read_csr(sstatus) | SSTATUS_SIE);
	write_csr(stvec, (uint64)start_trap);
	write_csr(sscratch, (uint64)&boot_trapframe);
	// 最重要！先把中断服务程序挂上去，不然崩溃都不知道怎么死的。

	if (hartid == 0) {
		// spike_file_init(); //TODO: 将文件系统迁移到 QEMU
		parseDtb(dtb);
		write_csr(satp, 0);
		early_vm_init();
	}
	if (NCPU > 1) early_sync_barrier(&counter, NCPU);
	write_csr(sie, read_csr(sie) | SIE_SEIE | SIE_STIE); // 不启用核间中断（暂时） TODO
	return;
}

__boot_code void early_write_tp(uint64 x) { asm volatile("mv tp, %0" : : "r"(x)); }

__boot_code void early_sync_barrier(volatile int32* counter, int32 all) {

	int32 local;

	asm volatile("amoadd.w %0, %2, (%1)\n" : "=r"(local) : "r"(counter), "r"(1) : "memory");

	if (local + 1 < all) {
		do {
			asm volatile("lw %0, (%1)\n" : "=r"(local) : "r"(counter) : "memory");
		} while (local < all);
	}
}

__boot_code void* early_memset(void* dest, int byte, size_t len) {
	if ((((uintptr_t)dest | len) & (sizeof(uintptr_t) - 1)) == 0) {
		uintptr_t word = byte & 0xFF;
		word |= word << 8;
		word |= word << 16;
		word |= word << 16 << 16;

		int32* d = dest;
		while ((uintptr_t)d < (uintptr_t)(dest + len)) *d++ = word;
	} else {
		char* d = dest;
		while (d < (char*)(dest + len)) *d++ = byte;
	}
	return dest;
}

__boot_code int32 early_map_pages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 size, int32 perm) {
	// size可以不对齐
	size = ROUNDUP(size, PAGE_SIZE);
	// //kprintf("early_map_pages: start\n");
	for (uint64 off = 0; off < size; off += PAGE_SIZE) {
		early_map_page(pagetable, va + off, pa + off, perm);
	}
	// //kprintf("early_map_pages: complete\n");

	return 0;
}

__boot_code int32 early_map_page(pagetable_t pagetable, vaddr_t va, paddr_t pa, int32 perm) {
	//kprintf("early_map_page: va=%lx pa=%lx perm=%lx\n", va, pa, perm);
	if (pagetable == NULL) {
		return -1;
	}
	uint64 aligned_va = ROUNDDOWN(va, PAGE_SIZE);
	uint64 aligned_pa = ROUNDDOWN(pa, PAGE_SIZE);

	// 检查虚拟地址是否有效
	if (aligned_va >= MAXVA) {
		return -1;
	}

	// 查找页表项，必要时分配页表
	pte_t* pte = early_page_walk(pagetable, aligned_va, 1);
	if (pte == NULL) {
		return -1;
	}

	// 检查是否已映射
	if (*pte & PTE_V) {
		// 页已映射，可能需要更新权限
		if (PTE2PA(*pte) == aligned_pa) {
			// 同一物理页，只更新权限
			*pte = PA2PPN(aligned_pa) | perm | PTE_V;
		} else {
			// 映射到不同物理页，报错
			return -1;
		}
	} else {
		// 创建新映射
		// //kprintf("create page=%lx perm: %lx\n", aligned_pa, perm);
		*pte = PA2PPN(aligned_pa) | perm | PTE_V;
	}

	return 0;
}

/**
 * 在页表中查找页表项
 */
__boot_code pte_t* early_page_walk(pagetable_t pagetable, uint64 va, int32 alloc) {
	if (pagetable == NULL) {
		return NULL;
	}

	// 检查虚拟地址是否有效
	if (va >= MAXVA) {
		return NULL;
	}

	// starting from the page directory
	pagetable_t pt = pagetable;

	for (int32 level = 2; level > 0; level--) {

		pte_t* pte = pt + PX(level, va);

		if (*pte & PTE_V) {

			pt = (pagetable_t)PTE2PA(*pte);
		} else {

			if (alloc && ((pt = (pte_t*)early_alloc_page()) != 0)) {
				early_memset(pt, 0, PAGE_SIZE);
				// writes the physical address of newly allocated page to pte, to
				// establish the page table tree.

				*pte = PA2PPN(pt) | PTE_V;
			} else {
				return 0;

			} // returns NULL, if alloc == 0, or no more physical page remains
		}
	}

	// return a PTE which contains phisical address of a page
	return pt + PX(0, va);
}

/**
 * 将保护标志(PROT_*)转换为页表项标志
 */
__boot_code uint64 early_prot_to_type(int32 prot, int32 user) {
	uint64 perm = 0;
	if (prot & PROT_READ) perm |= PTE_R | PTE_A;
	if (prot & PROT_WRITE) perm |= PTE_W | PTE_D;
	if (prot & PROT_EXEC) perm |= PTE_X | PTE_A;
	if (perm == 0) perm = PTE_R;
	if (user) perm |= PTE_U;
	return perm;
}




extern char _early_boot_page_pool[]; // 物理页池的起始地址
extern char _early_boot_page_pool_end[]; // 物理页池的结束地址
__boot_data void* early_page_pool = _early_boot_page_pool;
//void* early_page_pool = (void*)0x80240000;
__boot_code void* early_alloc_page(void) {
	void* ret = early_page_pool;
	early_page_pool += PAGE_SIZE;
	if ((uintptr_t)early_page_pool >= (uintptr_t)_early_boot_page_pool_end) {
		// 物理页池用完了
		return NULL;
	}
	return ret;
}