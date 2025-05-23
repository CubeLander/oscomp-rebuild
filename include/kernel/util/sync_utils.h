#ifndef _SYNC_UTILS_H_
#define _SYNC_UTILS_H_
// 等待所有核心到达同步点，之后再一起执行
static inline void sync_barrier(volatile int32 *counter, int32 all) {

  int32 local;

  asm volatile("amoadd.w %0, %2, (%1)\n"
               : "=r"(local)
               : "r"(counter), "r"(1)
               : "memory");

  if (local + 1 < all) {
    do {
      asm volatile("lw %0, (%1)\n" : "=r"(local) : "r"(counter) : "memory");
    } while (local < all);
  }
}

#define memory_barrier() __sync_synchronize()

#endif