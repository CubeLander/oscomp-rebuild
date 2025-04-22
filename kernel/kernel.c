/*
 * Supervisor-mode startup codes
 */

#include <kernel/boot/dtb.h>
#include <kernel/device/blockdevice.h>
#include <kernel/device/sbi.h>
#include <kernel/drivers/plic.h>
#include <kernel/drivers/virtio_device.h>
#include <kernel/elf.h>

#include <kernel/riscv.h>

#include <kernel.h>
#include <kernel/syscall/syscall.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <stdio.h>

// 分配 (NCPU + 1) 个保护页 + NCPU 个实际栈页
__attribute__((aligned(PAGE_SIZE))) char stack0[PAGE_SIZE * (NCPU + 1 + NCPU)];

__attribute__((aligned(PAGE_SIZE))) char emergency_stack_top[PAGE_SIZE];
// tp寄存器保存current_percpu中当前进程指针位置，然后通过偏移量反推hartid
task_t idle_tasks[NCPU] = {0};
cpuinfo_t cpuinfos[NCPU] = {0};

void setup_stack_guard_pages(void) {
	// 计算保护页的起始地址
	for (int i = 0; i <= NCPU; i++) {
		void* guard_page = stack0 + (i * 2 - 1) * (PAGE_SIZE);

		// 标记为不可访问
		pgt_map_pages(g_kernel_pagetable, (vaddr_t)guard_page, (paddr_t)guard_page, PAGE_SIZE, 0); // 无权限
	}
	kprintf("setup_stack_guard_pages: complete\n");

	// 刷新TLB
	flush_tlb();
}

static void kernel_vm_init(void) {
	kprintf("kernel_vm_init: start, membase = 0x%lx, memsize=0x%lx\n", memInfo.start, memInfo.size);
	// extern struct mm_struct init_mm;
	//  映射内核代码段和只读段
	g_kernel_pagetable = (pagetable_t)alloc_page()->paddr;
	// init_mm.pagetable = g_kernel_pagetable;
	//  之后它会被加入内核的虚拟空间，先临时用一个页
	memset(g_kernel_pagetable, 0, PAGE_SIZE);

	extern char _ftext[], _etext[], _fdata[], _end[];
	// kprintf("_etext=%lx,_ftext=%lx\n", _etext, _ftext);
	// PLIC
	pgt_map_pages(g_kernel_pagetable, PLIC, PLIC, 0x400000, prot_to_type(PROT_READ | PROT_WRITE, 0));
	pgt_map_pages(g_kernel_pagetable, (uint64)_ftext, (uint64)_ftext, (uint64)(_etext - _ftext), prot_to_type(PROT_READ | PROT_EXEC, 0));

	// 映射内核HTIF段
	pgt_map_pages(g_kernel_pagetable, (uint64)_etext, (uint64)_etext, (uint64)(_fdata - _etext), prot_to_type(PROT_READ | PROT_WRITE, 0));

	// 映射内核数据段
	pgt_map_pages(g_kernel_pagetable, (uint64)_fdata, (uint64)_fdata, (uint64)(_end - _fdata), prot_to_type(PROT_READ | PROT_WRITE, 0));
	// 映射内核数据段
	pgt_map_pages(g_kernel_pagetable, (uint64)_fdata, (uint64)_fdata, (uint64)(_end - _fdata), prot_to_type(PROT_READ | PROT_WRITE, 0));

	// 对于剩余的物理内存空间做直接映射
	pgt_map_pages(g_kernel_pagetable, (uint64)_end, (uint64)_end, DRAM_BASE + memInfo.size - (uint64)_end, prot_to_type(PROT_READ | PROT_WRITE, 0));
	// // satp不通过这层映射找g_kernel_pagetable，但是为了维护它，也需要做一个映射
	// pgt_map_pages(g_kernel_pagetable, (uint64)g_kernel_pagetable,
	//               (uint64)g_kernel_pagetable, PAGE_SIZE,
	//               prot_to_type(PROT_READ | PROT_WRITE, 0));
	setup_stack_guard_pages();
	// 映射内核栈
	// pgt_map_pages(init_mm.pagetable, (uint64)init_mm.pagetable, )

	// pagetable_dump(g_kernel_pagetable);

	// // 6. 映射MMIO区域（如果有需要）
	// // 例如UART、PLIC等外设的内存映射IO区域

	kprintf("kern_vm_init: complete\n");
}

/**
 * setup_init_fds - Set up standard file descriptors for init process
 * @init_task: Pointer to the init task structure
 *
 * Sets up standard input, output, and error for the init process.
 * This must be called before the init process starts executing.
 *
 * Returns: 0 on success, negative error code on failure
 */
int32 setup_init_fds(task_t* init_task) {
	int32 fd, console_fd;
	task_t* saved_task = current;

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
	task_t* init_task = alloc_task();
	task_init(init_task); // No parent for init
	init_task->parent = init_task;
	int32 error = -1;

	// Create the init process task structure
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

void start_trap() {
	while (1)
		;
}

// 这个boot_trapframe应该给每个核都发一个
void setup_cpu(int cpuid) {
	write_csr(sscratch, &cpuinfos[cpuid]);
	cpuinfos[cpuid].hart_id = cpuid;
	cpuinfos[cpuid].current_task = &idle_tasks[cpuid];
	write_csr(sstatus, read_csr(sstatus) | SSTATUS_SIE);

	extern char smode_trap_vector[];
	write_csr(stvec, (uint64)smode_trap_vector);
	write_csr(sie, read_csr(sie) | SIE_SEIE | SIE_STIE | SIE_SSIE);
	uint64 ksp = read_reg(sp);
	current->kernel_sp = ROUNDUP(ksp, PAGE_SIZE);

	return;
}

void idle_loop() {
	while (1) {
		schedule();
		wfi();
	}
}

void init_idle_task(){
	task_init(current);
	current->pid = -hartid;
}


//
// s_start: S-mode entry point of kernel
//
volatile static int counter = 0;
void s_start(int cpuid, uintptr_t dtb) {
	setup_cpu(cpuid);
	memory_barrier();

	if (hartid == 0) plic_init(); // set up interrupt controller
	plic_init_hart();             // ask PLIC for device interrupts

	if (hartid == 0) {
		// init_dtb(dtb);
		kprintf("Enter supervisor mode...\n");

		parseDtb(dtb);
		init_page_manager();
		kernel_vm_init();
		pagetable_activate(g_kernel_pagetable);
		create_init_mm();
		kmem_init();

		init_scheduler();
		init_idle_task();

		// init_scheduler();
		vfs_init();
		blockDeviceManager_init();
		init_virtio_bd();
		do_mount(0, 0, 0, 0, 0);
		create_init_process();
	}
	if (NCPU > 1) sync_barrier(&counter, NCPU);

	pagetable_activate(g_kernel_pagetable);
	// sync_barrier(&sync_counter, NCPU);

	//  写入satp寄存器并刷新tlb缓存
	//    从这里开始，所有内存访问都通过MMU进行虚实转换

	kprintf("Switch to user mode...\n");
	// the application code (elf) is first loaded into memory, and then put into
	// execution added @lab3_1
	idle_loop();
	// we should never reach here.
}