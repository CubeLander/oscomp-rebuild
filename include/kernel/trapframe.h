#ifndef _TRAPFRAME_H_
#define _TRAPFRAME_H_

#include <kernel/riscv.h>
#include <kernel/types.h>

typedef struct trapframe {
  	registers_t regs;			/* offset:[0,256)   */ 	/*x0-x31*/
	s_registers_t sregs;		/* offset:[256,304) */	/*sstatus-satp*/
	float_registers_t fregs;	/* offset:[304,560) */
}trapframe_t;/* offset:304 */
// clang-format off
#define store_all_registers(trapframe)                                        \
  asm volatile(                                                               \
      "sd x0,   0(%0)\n"                                                      \
      "sd ra,   8(%0)\n"                                                      \
      "sd sp,  16(%0)\n"                                                      \
      "sd gp,  24(%0)\n"                                                      \
      "sd tp,  32(%0)\n"                                                      \
      "sd t0,  40(%0)\n"                                                      \
      "sd t1,  48(%0)\n"                                                      \
      "sd t2,  56(%0)\n"                                                      \
      "sd s0,  64(%0)\n"                                                      \
      "sd s1,  72(%0)\n"                                                      \
      "sd a0,  80(%0)\n"                                                      \
      "sd a1,  88(%0)\n"                                                      \
      "sd a2,  96(%0)\n"                                                      \
      "sd a3, 104(%0)\n"                                                      \
      "sd a4, 112(%0)\n"                                                      \
      "sd a5, 120(%0)\n"                                                      \
      "sd a6, 128(%0)\n"                                                      \
      "sd a7, 136(%0)\n"                                                      \
      "sd s2, 144(%0)\n"                                                      \
      "sd s3, 152(%0)\n"                                                      \
      "sd s4, 160(%0)\n"                                                      \
      "sd s5, 168(%0)\n"                                                      \
      "sd s6, 176(%0)\n"                                                      \
      "sd s7, 184(%0)\n"                                                      \
      "sd s8, 192(%0)\n"                                                      \
      "sd s9, 200(%0)\n"                                                      \
      "sd s10,208(%0)\n"                                                      \
      "sd s11,216(%0)\n"                                                      \
      "sd t3, 224(%0)\n"                                                      \
      "sd t4, 232(%0)\n"                                                      \
      "sd t5, 240(%0)\n"                                                      \
      "sd t6, 248(%0)\n"                                                      \
      /* s_registers_t begins at offset 256 */                                \
      "csrr t0, sstatus\n"                                                    \
      "sd t0, 256(%0)\n"                                                      \
      "csrr t0, stvec\n"                                                      \
      "sd t0, 264(%0)\n"                                                      \
      "csrr t0, sepc\n"                                                       \
      "sd t0, 272(%0)\n"                                                      \
      "csrr t0, scause\n"                                                     \
      "sd t0, 280(%0)\n"                                                      \
      "csrr t0, stval\n"                                                      \
      "sd t0, 288(%0)\n"                                                      \
      "csrr t0, satp\n"                                                       \
      "sd t0, 296(%0)\n"                                                      \
      :                                                                       \
      : "r"(trapframe)                                                        \
      : "t0", "memory")

#define store_fp_registers(trapframe)                                       \
asm volatile(                                                             \
	"fsd ft0,  304(%0)\n"                                                 \
	"fsd ft1,  312(%0)\n"                                                 \
	"fsd ft2,  320(%0)\n"                                                 \
	"fsd ft3,  328(%0)\n"                                                 \
	"fsd ft4,  336(%0)\n"                                                 \
	"fsd ft5,  344(%0)\n"                                                 \
	"fsd ft6,  352(%0)\n"                                                 \
	"fsd ft7,  360(%0)\n"                                                 \
	"fsd fs0,  368(%0)\n"                                                 \
	"fsd fs1,  376(%0)\n"                                                 \
	"fsd fa0,  384(%0)\n"                                                 \
	"fsd fa1,  392(%0)\n"                                                 \
	"fsd fa2,  400(%0)\n"                                                 \
	"fsd fa3,  408(%0)\n"                                                 \
	"fsd fa4,  416(%0)\n"                                                 \
	"fsd fa5,  424(%0)\n"                                                 \
	"fsd fa6,  432(%0)\n"                                                 \
	"fsd fa7,  440(%0)\n"                                                 \
	"fsd fs2,  448(%0)\n"                                                 \
	"fsd fs3,  456(%0)\n"                                                 \
	"fsd fs4,  464(%0)\n"                                                 \
	"fsd fs5,  472(%0)\n"                                                 \
	"fsd fs6,  480(%0)\n"                                                 \
	"fsd fs7,  488(%0)\n"                                                 \
	"fsd fs8,  496(%0)\n"                                                 \
	"fsd fs9,  504(%0)\n"                                                 \
	"fsd fs10,512(%0)\n"                                                 \
	"fsd fs11,520(%0)\n"                                                 \
	"fsd ft8, 528(%0)\n"                                                 \
	"fsd ft9, 536(%0)\n"                                                 \
	"fsd ft10,544(%0)\n"                                                 \
	"fsd ft11,552(%0)\n"                                                 \
	:                                                                     \
	: "r"(trapframe)                                                      \
	: "memory")
	



#define restore_all_registers(trapframe)                                      \
asm volatile(                                                               \
	"ld x0,   0(%0)\n"                                                      \
	"ld ra,   8(%0)\n"                                                      \
	"ld sp,  16(%0)\n"                                                      \
	"ld gp,  24(%0)\n"                                                      \
	"ld tp,  32(%0)\n"                                                      \
	"ld t0,  40(%0)\n"                                                      \
	"ld t1,  48(%0)\n"                                                      \
	"ld t2,  56(%0)\n"                                                      \
	"ld s0,  64(%0)\n"                                                      \
	"ld s1,  72(%0)\n"                                                      \
	"ld a0,  80(%0)\n"                                                      \
	"ld a1,  88(%0)\n"                                                      \
	"ld a2,  96(%0)\n"                                                      \
	"ld a3, 104(%0)\n"                                                      \
	"ld a4, 112(%0)\n"                                                      \
	"ld a5, 120(%0)\n"                                                      \
	"ld a6, 128(%0)\n"                                                      \
	"ld a7, 136(%0)\n"                                                      \
	"ld s2, 144(%0)\n"                                                      \
	"ld s3, 152(%0)\n"                                                      \
	"ld s4, 160(%0)\n"                                                      \
	"ld s5, 168(%0)\n"                                                      \
	"ld s6, 176(%0)\n"                                                      \
	"ld s7, 184(%0)\n"                                                      \
	"ld s8, 192(%0)\n"                                                      \
	"ld s9, 200(%0)\n"                                                      \
	"ld s10,208(%0)\n"                                                      \
	"ld s11,216(%0)\n"                                                      \
	"ld t3, 224(%0)\n"                                                      \
	"ld t4, 232(%0)\n"                                                      \
	"ld t5, 240(%0)\n"                                                      \
	"ld t6, 248(%0)\n"                                                      \
	/* s_registers_t begins at offset 256 */                                \
	"ld t0, 256(%0)\n"                                                      \
	"csrw sstatus, t0\n"                                                   \
	"ld t0, 264(%0)\n"                                                      \
	"csrw stvec, t0\n"                                                      \
	"ld t0, 272(%0)\n"                                                      \
	"csrw sepc, t0\n"                                                       \
	"ld t0, 280(%0)\n"                                                      \
	"csrw scause, t0\n"                                                     \
	"ld t0, 288(%0)\n"                                                      \
	"csrw stval, t0\n"                                                      \
	"ld t0, 296(%0)\n"                                                      \
	"csrw satp, t0\n"                                                       \
	:                                                                       \
	: "r"(trapframe)                                                        \
	: "t0", "memory")



#define restore_fp_registers(trapframe)                                     \
asm volatile(                                                             \
	"fld ft0,  304(%0)\n"                                                 \
	"fld ft1,  312(%0)\n"                                                 \
	"fld ft2,  320(%0)\n"                                                 \
	"fld ft3,  328(%0)\n"                                                 \
	"fld ft4,  336(%0)\n"                                                 \
	"fld ft5,  344(%0)\n"                                                 \
	"fld ft6,  352(%0)\n"                                                 \
	"fld ft7,  360(%0)\n"                                                 \
	"fld fs0,  368(%0)\n"                                                 \
	"fld fs1,  376(%0)\n"                                                 \
	"fld fa0,  384(%0)\n"                                                 \
	"fld fa1,  392(%0)\n"                                                 \
	"fld fa2,  400(%0)\n"                                                 \
	"fld fa3,  408(%0)\n"                                                 \
	"fld fa4,  416(%0)\n"                                                 \
	"fld fa5,  424(%0)\n"                                                 \
	"fld fa6,  432(%0)\n"                                                 \
	"fld fa7,  440(%0)\n"                                                 \
	"fld fs2,  448(%0)\n"                                                 \
	"fld fs3,  456(%0)\n"                                                 \
	"fld fs4,  464(%0)\n"                                                 \
	"fld fs5,  472(%0)\n"                                                 \
	"fld fs6,  480(%0)\n"                                                 \
	"fld fs7,  488(%0)\n"                                                 \
	"fld fs8,  496(%0)\n"                                                 \
	"fld fs9,  504(%0)\n"                                                 \
	"fld fs10,512(%0)\n"                                                 \
	"fld fs11,520(%0)\n"                                                 \
	"fld ft8, 528(%0)\n"                                                 \
	"fld ft9, 536(%0)\n"                                                 \
	"fld ft10,544(%0)\n"                                                 \
	"fld ft11,552(%0)\n"                                                 \
	:                                                                     \
	: "r"(trapframe)                                                      \
	: "memory")
  





// clang-format on


#endif