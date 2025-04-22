#include <kernel/device/interface.h>
#include <kernel.h>
// #include <lock/mutex.h> // TODO: 暂时不实现 lock / mutex
// #include <proc/thread.h>
#include <kernel/riscv.h>

#include <kernel/device/sbi.h>
#include <kernel/types.h>

// 建立一个printf的锁，保证同一个printf中的数据都能在一次输出完毕
// mutex_t pr_lock;
spinlock_t pr_lock = SPINLOCK_INIT;
void printInit() {
	// mtx_init(&pr_lock, "kprintf", false, MTX_SPIN); // 此处禁止调试信息输出！否则会递归获取锁
}

// vprintfmt只调用output输出可输出字符，不包括0，所以需要记得在字符串后补0
static void outputToStr(void *data, const char *buf, size_t len) {
	char **strBuf = (char **)data;
	for (int i = 0; i < len; i++) {
		(*strBuf)[i] = buf[i];
	}
	(*strBuf)[len] = 0;
	*strBuf += len;
}

static void output(void *data, const char *buf, size_t len) {
	for (int i = 0; i < len; i++) {
		cons_putc(buf[i]);
	}
}

static void printfNoLock(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintfmt(output, NULL, fmt, ap);
	va_end(ap);
}

void kprintf(const char *fmt, ...) {
//void kprintf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	// mtx_lock(&pr_lock); // todo lock
	spinlock_lock(&pr_lock);
	vprintfmt(output, NULL, fmt, ap);
	spinlock_unlock(&pr_lock);
	// mtx_unlock(&pr_lock);

	va_end(ap);
}

// sprintf无需加锁
void ksprintf(char *buf, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	char *mybuf = buf;

	vprintfmt(outputToStr, &mybuf, fmt, ap);

	va_end(ap);
}

// TODO: 加上时间戳、CPU编号等信息
void _log(const char *file, int line, const char *func, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	// mtx_lock(&pr_lock);
	// 输出日志头
	printfNoLock("%s %2d %12s:%-4d %12s()" SGR_RESET ": ",
		     FARM_INFO "[INFO]" SGR_RESET SGR_BLUE, read_tp(), file, line, func);
	// 输出实际内容
	vprintfmt(output, NULL, fmt, ap);
	// mtx_unlock(&pr_lock);

	va_end(ap);
}

void _warn(const char *file, int line, const char *func, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	// mtx_lock(&pr_lock);
	// 输出日志头
	printfNoLock("%s %2d %12s:%-4d %12s()" SGR_RESET ": ",
		     FARM_WARN "[WARN]" SGR_RESET SGR_YELLOW, read_tp(), file, line, func);
	// 输出实际内容
	vprintfmt(output, NULL, fmt, ap);
	// mtx_unlock(&pr_lock);

	va_end(ap);
}

// static void print_stack(u64 kstack) {
// 	kprintf("panic in Thread %s. kernel sp = %lx\n", cpu_this()->cpu_running->td_name, kstack);

// 	extern thread_t *threads;
// 	extern void *kstacks;
// 	u64 stackTop = (u64)kstacks + TD_KSTACK_SIZE * (cpu_this()->cpu_running - threads + 1);
// 	u64 epc;
// 	asm volatile("auipc %0, 0" : "=r"(epc));

// 	char *buf = kmalloc(32 * PAGE_SIZE);
// 	buf[0] = 0;
// 	char *pbuf = buf;
// 	if (stackTop - TD_KSTACK_SIZE < kstack && kstack <= stackTop) {
// 		kprintf("Kernel Stack Used: 0x%lx\n", stackTop - kstack);
// 		kprintf(pbuf, "Memory(Start From 0x%016lx)\n", kstack);
// 		ksprintf(pbuf, "0x%016lx \'[", kstack);
// 		pbuf += 21;
// 		for (u64 sp = kstack; sp < stackTop; sp++) {
// 			char ch = *(char *)sp; // 内核地址，可以直接访问
// 			ksprintf(pbuf, "0x%02x, ", ch);
// 			pbuf += 6;
// 		}
// 		ksprintf(pbuf, "]\' kern/kernel.asm 0x%016lx", epc);
// 		kprintf("%s\n", buf);
// 	} else {
// 		kprintf("[ERROR] sp is out of ustack range[0x%016lx, 0x%016lx]. Maybe program has crashed!\n", stackTop - TD_KSTACK_SIZE, stackTop);
// 	}
// 	kfree(buf);
// }


void _error(const char *file, int line, const char *func, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	uint64 sp;
	asm volatile("mv %0, sp" : "=r"(sp));
	// print_stack(sp); //TODO: 打印栈信息（感觉貌似还挺有用的？）

	// mtx_lock(&pr_lock);
	// 输出日志头
	printfNoLock("%s %2d %12s:%-4d %12s()  !TEST FINISH! " SGR_RESET ": ",
		     FARM_ERROR "[ERROR]" SGR_RESET SGR_RED, read_tp(), file, line, func);
	// 输出实际内容
	vprintfmt(output, NULL, fmt, ap);
	printfNoLock("\n\n");
	// mtx_unlock(&pr_lock);

	va_end(ap);

	// cpu_halt();
	intr_off();
	SBI_SYSTEM_RESET(0, 0);

	while (1)
		;
}