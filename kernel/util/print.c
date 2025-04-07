#include <kernel/device/interface.h>
#include <kernel/util.h>
// #include <lock/mutex.h> // TODO: 暂时不实现 lock / mutex
// #include <proc/thread.h>
#include <kernel/riscv.h>
#include <kernel/mmu.h>
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

void panic(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	spinlock_lock(&pr_lock);
	vprintfmt(output, NULL, fmt, ap);
	spinlock_unlock(&pr_lock);

	va_end(ap);

	intr_off();
	SBI_SYSTEM_RESET(0, 0);

	while (1)
		;
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

/**
 * @brief 在发生异常时，打印寄存器的信息
 */
void printReg(struct trapframe *tf) {
	// mtx_lock(&pr_lock);

	printfNoLock("ra  = 0x%016lx\t", tf->regs.ra);
	printfNoLock("sp  = 0x%016lx\t", tf->regs.sp);
	printfNoLock("gp  = 0x%016lx\n", tf->regs.gp);
	printfNoLock("tp  = 0x%016lx\t", tf->regs.tp);
	printfNoLock("t0  = 0x%016lx\t", tf->regs.t0);
	printfNoLock("t1  = 0x%016lx\n", tf->regs.t1);
	printfNoLock("t2  = 0x%016lx\t", tf->regs.t2);
	printfNoLock("s0  = 0x%016lx\t", tf->regs.s0);
	printfNoLock("s1  = 0x%016lx\n", tf->regs.s1);
	printfNoLock("a0  = 0x%016lx\t", tf->regs.a0);
	printfNoLock("a1  = 0x%016lx\t", tf->regs.a1);
	printfNoLock("a2  = 0x%016lx\n", tf->regs.a2);
	printfNoLock("a3  = 0x%016lx\t", tf->regs.a3);
	printfNoLock("a4  = 0x%016lx\t", tf->regs.a4);
	printfNoLock("a5  = 0x%016lx\n", tf->regs.a5);
	printfNoLock("a6  = 0x%016lx\t", tf->regs.a6);
	printfNoLock("a7  = 0x%016lx\t", tf->regs.a7);
	printfNoLock("s2  = 0x%016lx\n", tf->regs.s2);
	printfNoLock("s3  = 0x%016lx\t", tf->regs.s3);
	printfNoLock("s4  = 0x%016lx\t", tf->regs.s4);
	printfNoLock("s5  = 0x%016lx\n", tf->regs.s5);
	printfNoLock("s6  = 0x%016lx\t", tf->regs.s6);
	printfNoLock("s7  = 0x%016lx\t", tf->regs.s7);
	printfNoLock("s8  = 0x%016lx\n", tf->regs.s8);
	printfNoLock("s9  = 0x%016lx\t", tf->regs.s9);
	printfNoLock("s10 = 0x%016lx\t", tf->regs.s10);
	printfNoLock("s11 = 0x%016lx\n", tf->regs.s11);
	printfNoLock("t3  = 0x%016lx\t", tf->regs.t3);
	printfNoLock("t4  = 0x%016lx\t", tf->regs.t4);
	printfNoLock("t5  = 0x%016lx\n", tf->regs.t5);
	printfNoLock("t6  = 0x%016lx\n", tf->regs.t6);

	// mtx_unlock(&pr_lock);
}



static int vsnprintf_internal(char* str, size_t size, const char* format, va_list args);

/**
 * snprintf - 格式化字符串并将结果写入缓冲区，同时确保不会溢出
 * @str: 目标缓冲区
 * @size: 缓冲区的大小（包括终止符'\0'）
 * @format: 格式化字符串
 * @...: 可变参数列表
 *
 * 将格式化字符串写入到指定缓冲区，最多写入size-1个字符，
 * 并确保缓冲区以'\0'结尾。
 *
 * 返回值: 如果缓冲区足够大，返回写入的字符数（不包括终止符'\0'）；
 *         否则返回格式化完成后本应该写入的字符数（不包括终止符'\0'）
 */
int snprintf(char* str, size_t size, const char* format, ...) {
	va_list args;
	int result;

	// 如果缓冲区大小为0或为NULL，不进行任何写入
	if (size == 0 || str == NULL) { return 0; }

	va_start(args, format);
	result = vsnprintf_internal(str, size, format, args);
	va_end(args);

	return result;
}


// 内部实现，处理格式化并写入字符
static int vsnprintf_internal(char* str, size_t size, const char* format, va_list args) {
	size_t count = 0; // 已写入或需要写入的字符数
	char* s;
	int num, len;
	char padding, temp[32];
	uint32 unum;

	if (size == 0) { return 0; }

	// 确保至少有一个字节用于终止符
	size--;

	while (*format && count < size) {
		if (*format != '%') {
			// 普通字符直接写入
			str[count++] = *format++;
			continue;
		}

		// 处理格式说明符
		format++;

		// 处理填充（暂时只支持0填充）
		padding = ' ';
		if (*format == '0') {
			padding = '0';
			format++;
		}

		// 处理不同类型的格式化
		switch (*format) {
		case 's': // 字符串
			s = va_arg(args, char*);
			if (s == NULL) { s = "(null)"; }
			len = strlen(s);
			// 复制字符串，但不超过剩余空间
			while (*s && count < size) { str[count++] = *s++; }
			// 即使无法完全写入，也要计算总长度
			count += strlen(s);
			break;

		case 'd': // 有符号整数
		case 'i':
			num = va_arg(args, int);

			// 处理负数
			if (num < 0) {
				if (count < size) {
					str[count++] = '-';
				} else {
					count++;
				}
				num = -num;
			}

			// 转换为字符串
			len = 0;
			do {
				temp[len++] = '0' + (num % 10);
				num /= 10;
			} while (num > 0);

			// 反向写入
			while (len > 0 && count < size) { str[count++] = temp[--len]; }
			// 计算剩余未写入的字符
			count += len;
			break;

		case 'u': // 无符号整数
			unum = va_arg(args, uint32);

			// 转换为字符串
			len = 0;
			do {
				temp[len++] = '0' + (unum % 10);
				unum /= 10;
			} while (unum > 0);

			// 反向写入
			while (len > 0 && count < size) { str[count++] = temp[--len]; }
			// 计算剩余未写入的字符
			count += len;
			break;

		case 'x': // 十六进制（小写）
		case 'X': // 十六进制（大写）
			unum = va_arg(args, uint32);

			// 转换为字符串
			len = 0;
			do {
				int digit = unum % 16;
				if (digit < 10) {
					temp[len++] = '0' + digit;
				} else {
					temp[len++] = (*format == 'x' ? 'a' : 'A') + (digit - 10);
				}
				unum /= 16;
			} while (unum > 0);

			// 反向写入
			while (len > 0 && count < size) { str[count++] = temp[--len]; }
			// 计算剩余未写入的字符
			count += len;
			break;

		case 'c': // 字符
			if (count < size) {
				str[count++] = (char)va_arg(args, int);
			} else {
				count++;
				va_arg(args, int); // 跳过参数
			}
			break;

		case '%': // 百分号
			if (count < size) {
				str[count++] = '%';
			} else {
				count++;
			}
			break;

		default: // 不支持的格式，原样输出
			if (count < size) {
				str[count++] = *format;
			} else {
				count++;
			}
		}

		format++;
	}

	// 添加终止符
	if (size > 0) { str[count < size ? count : size] = '\0'; }

	// 计算剩余格式字符串会产生的字符数
	while (*format) {
		if (*format == '%') {
			format++;
			if (*format == 's') {
				s = va_arg(args, char*);
				if (s) { count += strlen(s); }
			} else if (*format == 'c' || *format == 'd' || *format == 'i' || *format == 'u' || *format == 'x' || *format == 'X') {
				va_arg(args, int); // 跳过参数
				count++;
			} else if (*format == '%') {
				count++;
			}
		} else {
			count++;
		}
		format++;
	}

	return count;
}
