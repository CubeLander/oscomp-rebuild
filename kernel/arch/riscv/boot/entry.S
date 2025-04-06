#include <kernel/mm/memlayout.h>
//#include <kernel/riscv.h>

	# qemu -kernel loads the kernel at 0x80000000
        # and causes each hart (i.e. CPU) to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
#define PAGE_SIZE 4096
.section .text.entry
.global _entry
_entry:
    # 关中断
    csrci sstatus, 2        # 直接清除 sstatus 的第 1 位 (SIE)

    # 保存传入的参数
    mv s0, a0      # 保存 hartid 到 s0
    mv s1, a1      # 保存 dtb 到 s1
    la sp, stack0

    # 每个CPU的栈是: 保护页 + 实际栈页
    # CPU 0: 保护页0, 栈页0
    # CPU 1: 保护页1, 栈页1
    # 但保护页是共享的: 栈页0也用保护页1
    
    # 先计算到第一个保护页之后
		li t0, 4096
		add t1, a0, 0
		li t2, 2
		mul t1, t1, t2
		addi t1, t1, 1
		mul t1, t1, t0
    add sp, sp, t1	# sp = sp + 4096 * (2 * hartid + 1)
    
    # 接下来跳转到s_start
	  call early_boot

		# Convert _g_pagetable_pa to satp format
		li t0, 8                   # SATP_MODE_SV39 = 8
		slli t0, t0, 60            # Shift left by 60 bits to position the mode bits
		la t1, _g_pagetable_pa     # Load address of _g_pagetable_pa
		srli t1, t1, 12            # Shift right by 12 bits (as per MAKE_SATP macro)
		or t0, t0, t1              # Combine mode bits and PPN

		# Write to satp register
		csrw satp, t0              # Write to satp CSR

		# Flush TLB
		sfence.vma zero, zero      # Flush all TLB entries

		# Adjust stack pointer before enabling paging
		li t1, 0xff000000          # Virtual base address
		li t2, 0x80000000          # Physical base address
		sub t1, t1, t2             # Calculate offset
		add sp, sp, t1             # Adjust stack pointer
		# add s1, s1, t1						 # Adjust dtb

    # 准备调用 s_start，恢复参数到 a0 和 a1
    mv a0, s0      # 将 hartid 放回 a0
    mv a1, s1      # 将 dtb 放回 a1
    call s_start
spin:
        j spin

	// 下面的代码并非延迟槽，实际上，RISCV没有延迟槽
	nop
	nop
