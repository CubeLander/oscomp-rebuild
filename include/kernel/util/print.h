#pragma once

#include <stdarg.h>
//#define kprintf(fmt, ...) kprintf(fmt, ##__VA_ARGS__)
//#define kprintf(fmt, ...)

//#define panic(fmt, ...) kprintf(fmt, ##__VA_ARGS__)
//#define assert(x) if (!(x)) panic("Assertion failed: %s\n", #x)
#define assert(x)
struct trapframe;
void printReg(struct trapframe *tf);

void kprintf(const char *fmt, ...);
void panic(const char *fmt, ...);

// 初始化锁
void printInit(void);


// 输出到字符串
void ksprintf(char *buf, const char *fmt, ...);
int snprintf(char* str, size_t size, const char* format, ...);
