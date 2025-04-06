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

/**
 * setup_init_fds - Set up standard file descriptors for init process
 * @init_task: Pointer to the init task structure
 *
 * Sets up standard input, output, and error for the init process.
 * This must be called before the init process starts executing.
 *
 * Returns: 0 on success, negative error code on failure
 */
int32 setup_init_fds(struct task_struct* init_task) {
	int32 fd, console_fd;
	struct task_struct* saved_task = current_task();

	// Temporarily set current to init task
	set_current_task(init_task);

	// Set up stdin, stdout, stderr
	for (fd = 0; fd < 3; fd++) {
		if (fd != do_open("/dev/console", O_RDWR, 0)) {
			kprintf("Failed to open /dev/console for fd %d\n", fd);
			set_current_task(saved_task);
			return -1;
		}
	}

	// Restore original task
	set_current_task(saved_task);
	return 0;
}

int32 create_init_process(void) {
	struct task_struct* init_task;
	int32 error = -1;

	// Create the init process task structure
	init_task = alloc_process(); // No parent for init
	if (!init_task) return -ENOMEM;

	// Set up process ID and other basic attributes
	init_task->pid = 1;
	init_task->parent = NULL; // No parent

	setup_init_fds(init_task);
	// Load the init binary
	error = load_init_binary(init_task, "/sbin/init");
	if (error) goto fail_exec;

	// Add to scheduler queue and start running
	insert_to_ready_queue(init_task);

	// we should never reach here.
	return 0;

fail_exec:
	// Clean up file descriptors
	for (int fd = 0; fd < init_task->fdtable->max_fds; fd++) {
		if (init_task->fdtable->fd_array[fd]) fdtable_closeFd(init_task->fdtable, fd);
	}
fail_fds:
	fs_struct_unref(init_task->fs);
fail_fs:
	free_process(init_task);
	return error;
}



//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
volatile static int32 sig = 1;
volatile static int counter = 0;

void s_start(uintptr_t hartid, uintptr_t dtb) {
	write_tp(hartid);
	// 最重要！先把中断服务程序挂上去，不然崩溃都不知道怎么死的。

	if (hartid == 0) {
		// spike_file_init(); //TODO: 将文件系统迁移到 QEMU
		// init_dtb(dtb);
		parseDtb(dtb);
	}
	if (NCPU > 1) sync_barrier(&counter, NCPU);
	write_csr(sie, read_csr(sie) | SIE_SEIE | SIE_STIE); // 不启用核间中断（暂时） TODO

	// init timing. added @lab1_3
	// lab1_challenge1 为了调试便利，禁用了外部时钟中断：
	// timerinit(hartid);

	// switch to supervisor mode (S mode) and jump to s_start(), i.e., set pc to
	// mepc
	// asm volatile("mret");
	// 在内核初始化早期添加
	// kprintf("Current privilege level: %ld\n", (read_csr(mstatus) >> 11) & 3);
	// kprintf("stvec: 0x%lx\n", read_csr(stvec));
	// kprintf("medeleg: 0x%lx\n", read_csr(medeleg));
	// kprintf("mideleg: 0x%lx\n", read_csr(mideleg));
	// kprintf("sstatus: 0x%lx\n", read_csr(sstatus));
	// kprintf("sie: 0x%lx\n", read_csr(sie));
	// kprintf("In m_start, hartid:%d\n", hartid);

	//write_csr(stvec, (uint64)start_trap);

	extern void init_idle_task(void);

	kprintf("Enter supervisor mode...\n");
	write_csr(satp, 0);

	if (hartid == 0) {
		init_page_manager();
		//pagetable_activate(g_kernel_pagetable);
		//boot_trapframe.kernel_satp = MAKE_SATP(g_kernel_pagetable);
		create_init_mm();
		kmem_init();
		init_scheduler();

		init_idle_task();
		// kmalloc在形式上需要使用init_mm的“用户虚拟空间分配器”
		// 所以我们在启用kmalloc之前，需要先初始化0号进程

		// init_scheduler();
		vfs_init();
		sig = 0;
	} else {
		while (sig) {
		}
		pagetable_activate(g_kernel_pagetable);
	}

	// sync_barrier(&sync_counter, NCPU);

	//  写入satp寄存器并刷新tlb缓存
	//    从这里开始，所有内存访问都通过MMU进行虚实转换

	kprintf("Switch to user mode.....\n");


	// the application code (elf) is first loaded into memory, and then put into
	// execution added @lab3_1
	create_init_process();
	schedule();
	// we should never reach here.
	return;
}

// This is a dummy file for RISC-V architecture detection
// Save this file as dummy_riscv.c in your project root

int main() { return 0; }

// Compile with:
// riscv64-unknown-elf-gcc -o dummy_riscv dummy_riscv.c