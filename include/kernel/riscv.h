#ifndef _RISCV_H_
#define _RISCV_H_

#include <kernel/types.h>
#include <kernel/config.h> 
#include <stdatomic.h>

// fields of mstatus, the Machine mode Status register
#define MSTATUS_MPP_MASK (3L << 11) // previous mode mask
#define MSTATUS_MPP_M (3L << 11)    // machine mode (m-mode)
#define MSTATUS_MPP_S (1L << 11)    // supervisor mode (s-mode)
#define MSTATUS_MPP_U (0L << 11)    // user mode (u-mode)
#define MSTATUS_MIE (1L << 3)       // machine-mode interrupt enable
#define MSTATUS_MPIE (1L << 7)      // preserve MIE bit

// values of mcause, the Machine Cause register
#define IRQ_S_EXT 9                 // s-mode external interrupt
#define IRQ_S_TIMER 5               // s-mode timer interrupt
#define IRQ_S_SOFT 1                // s-mode software interrupt
#define IRQ_M_SOFT 3                // m-mode software interrupt

// fields of mip, the Machine Interrupt Pending register
#define MIP_SEIP (1 << IRQ_S_EXT)   // s-mode external interrupt pending
#define MIP_SSIP (1 << IRQ_S_SOFT)  // s-mode software interrupt pending
#define MIP_STIP (1 << IRQ_S_TIMER) // s-mode timer interrupt pending
#define MIP_MSIP (1 << IRQ_M_SOFT)  // m-mode software interrupt pending

// physical memory protection choices
#define PMP_R 0x01
#define PMP_W 0x02
#define PMP_X 0x04
#define PMP_A 0x18
#define PMP_L 0x80
#define PMP_SHIFT 2

#define PMP_TOR 0x08
#define PMP_NA4 0x10
#define PMP_NAPOT 0x18

// exceptions
#define CAUSE_MISALIGNED_FETCH 0x0     // Instruction address misaligned
#define CAUSE_FETCH_ACCESS 0x1         // Instruction access fault
#define CAUSE_ILLEGAL_INSTRUCTION 0x2  // Illegal Instruction
#define CAUSE_BREAKPOINT 0x3           // Breakpoint
#define CAUSE_MISALIGNED_LOAD 0x4      // Load address misaligned
#define CAUSE_LOAD_ACCESS 0x5          // Load access fault
#define CAUSE_MISALIGNED_STORE 0x6     // Store/AMO address misaligned
#define CAUSE_STORE_ACCESS 0x7         // Store/AMO access fault
#define CAUSE_USER_ECALL 0x8           // Environment call from U-mode
#define CAUSE_SUPERVISOR_ECALL 0x9     // Environment call from S-mode
#define CAUSE_MACHINE_ECALL 0xb        // Environment call from M-mode
#define CAUSE_FETCH_PAGE_FAULT 0xc     // Instruction page fault
#define CAUSE_LOAD_PAGE_FAULT 0xd      // Load page fault
#define CAUSE_STORE_PAGE_FAULT 0xf     // Store/AMO page fault

// irqs (interrupts). added @lab1_3
#define CAUSE_MTIMER 0x8000000000000007
#define CAUSE_MTIMER_S_TRAP 0x8000000000000001

//Supervisor interrupt-pending register
#define SIP_SSIP (1L << 1)

// core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8 * (hartid))
#define CLINT_MTIME (CLINT + 0xBFF8)  // cycles since boot.

// fields of sstatus, the Supervisor mode Status register
#define SSTATUS_SPP (1L << 8)   // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5)  // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4)  // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)   // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)   // User Interrupt Enable
#define SSTATUS_SUM 0x00040000
#define SSTATUS_FS 0x00006000

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9)  // external
#define SIE_STIE (1L << 5)  // timer
#define SIE_SSIE (1L << 1)  // software

// Machine-mode Interrupt Enable
#define MIE_MEIE (1L << 11)  // external
#define MIE_MTIE (1L << 7)   // timer
#define MIE_MSIE (1L << 3)   // software

#define read_reg(reg)              \
  ({                                     \
    unsigned long __tmp;                 \
    asm("mv %0, " #reg : "=r"(__tmp)); \
    __tmp;                               \
  })

  #define write_reg(reg, val) ({ asm volatile("mv " #reg ", %0" ::"rK"(val)); })


  #define read_csr(reg)                             \
  ({                                              	\
    unsigned long __tmp;                          			\
    asm volatile("csrr %0, " #reg : "=r"(__tmp)); 	\
    __tmp;                                        	\
  })


static inline int32 supports_extension(char ext) {
  return read_csr(misa) & (1 << (ext - 'A'));
}


#define write_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"rK"(val)); })


#define swap_csr(reg, val)                                             \
   ({                                                                  \
    unsigned long __tmp;                                              \
    asm volatile("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "rK"(val)); \
    __tmp;                                                         \
  })

#define set_csr(reg, bit)                                             \
  ({                                                                  \
    unsigned long __tmp;                                              \
    asm volatile("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "rK"(bit)); \
    __tmp;                                                            \
  })

// enable device interrupts
static inline void intr_on(void) { write_csr(sstatus, read_csr(sstatus) | SSTATUS_SIE); }

// disable device interrupts
static inline void intr_off(void) { write_csr(sstatus, read_csr(sstatus) & ~SSTATUS_SIE); }

// are device interrupts enabled?
static inline int32 is_intr_enable(void) {
  //  unsigned long x = r_sstatus();
  unsigned long x = read_csr(sstatus);
  return (x & SSTATUS_SIE) != 0;
}

// read sp, the stack pointer
static inline unsigned long read_sp(void) {
  unsigned long x;
  asm volatile("mv %0, sp" : "=r"(x));
  return x;
}

// read tp, the thread pointer, holding hartid (core number), the index into cpus[].
static inline unsigned long read_tp(void) {
  unsigned long x;
  asm volatile("mv %0, tp" : "=r"(x));
  return x;
}

// write tp, the thread pointer, holding hartid (core number), the index into cpus[].
static inline void write_tp(unsigned long x) { asm volatile("mv tp, %0" : : "r"(x)); }

typedef struct ptrace_regs {
	unsigned long epc;        /*  0 */
	unsigned long ra;         /*  8 */
	unsigned long sp;         /* 16 */
	unsigned long gp;         /* 24 */
	unsigned long tp;         /* 32 */
	unsigned long t0;         /* 40 */
	unsigned long t1;         /* 48 */
	unsigned long t2;         /* 56 */
	unsigned long s0;         /* 64 */
	unsigned long s1;         /* 72 */
	unsigned long a0;         /* 80 */
	unsigned long a1;         /* 88 */
	unsigned long a2;         /* 96 */
	unsigned long a3;         /* 104 */
	unsigned long a4;         /* 112 */
	unsigned long a5;         /* 120 */
	unsigned long a6;         /* 128 */
	unsigned long a7;         /* 136 */
	unsigned long s2;         /* 144 */
	unsigned long s3;         /* 152 */
	unsigned long s4;         /* 160 */
	unsigned long s5;         /* 168 */
	unsigned long s6;         /* 176 */
	unsigned long s7;         /* 184 */
	unsigned long s8;         /* 192 */
	unsigned long s9;         /* 200 */
	unsigned long s10;        /* 208 */
	unsigned long s11;        /* 216 */
	unsigned long t3;         /* 224 */
	unsigned long t4;         /* 232 */
	unsigned long t5;         /* 240 */
	unsigned long t6;         /* 248 */
	/* Supervisor/Machine CSRs */
	unsigned long status;     /* 256 */
	unsigned long badaddr;    /* 264 */
	unsigned long cause;      /* 272 */
	unsigned long orig_a0;    /* 280 */
	unsigned long ft0;
	unsigned long ft1;
	unsigned long ft2;
	unsigned long ft3;
	unsigned long ft4;
	unsigned long ft5;
	unsigned long ft6;
	unsigned long ft7;
	unsigned long fs0;
	unsigned long fs1;
	unsigned long fa0;
	unsigned long fa1;
	unsigned long fa2;
	unsigned long fa3;
	unsigned long fa4;
	unsigned long fa5;
	unsigned long fa6;
	unsigned long fa7;
	unsigned long fs2;
	unsigned long fs3;
	unsigned long fs4;
	unsigned long fs5;
	unsigned long fs6;
	unsigned long fs7;
	unsigned long fs8;
	unsigned long fs9;
	unsigned long fs10;
	unsigned long fs11;
	unsigned long ft8;
	unsigned long ft9;
	unsigned long ft10;
	unsigned long ft11;
} regs_t;

#define PT_EPC      0
#define PT_RA       8
#define PT_SP       16
#define PT_GP       24
#define PT_TP       32
#define PT_T0       40
#define PT_T1       48
#define PT_T2       56
#define PT_S0       64
#define PT_S1       72
#define PT_A0       80
#define PT_A1       88
#define PT_A2       96
#define PT_A3       104
#define PT_A4       112
#define PT_A5       120
#define PT_A6       128
#define PT_A7       136
#define PT_S2       144
#define PT_S3       152
#define PT_S4       160
#define PT_S5       168
#define PT_S6       176
#define PT_S7       184
#define PT_S8       192
#define PT_S9       200
#define PT_S10      208
#define PT_S11      216
#define PT_T3       224
#define PT_T4       232
#define PT_T5       240
#define PT_T6       248

#define PT_STATUS   256
#define PT_BADADDR  264
#define PT_CAUSE    272
#define PT_ORIG_A0  280
#define PT_SIZE_ON_STACK  288



// following lines are added @lab2_1
static inline void flush_tlb(void) { asm volatile("sfence.vma zero, zero"); }
#define PAGE_SIZE 4096  // bytes per page
/* 
 * Mark parameters that must be page-aligned.
 * For documentation and static analysis.
 */
#define __page_aligned


// extract the property bits of a pte



// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))



#endif
