.section .text

.global smode_trap_vector
smode_trap_vector:

	/*save context*/
	csrr t2, sscratch		// t2 = cpuinfo_t cpuinfo
	ld t1, 0(t2)			// t1 = cpuinfo->current_task
	ld t0, 0(t1)			// t0 = current_task->trap_context
	call save_trapframe		// save_trapframe(t0)
	/*restore kernel sp*/
	csrr t3, sstatus
	andi t3, t3, 0x100		// SSTATUS_SPP
	bnez t3, from_kernel
	// from user
	ld sp, 16(t1)			// 恢复内核栈
from_kernel:
	/*the main trap handler logic*/
	call kernel_trap_handler
.global return_to_context
return_to_context:
	/*restore context*/
	csrr t2, sscratch		// t2 = cpuinfo_t cpuinfo
	ld t1, 0(t2)			// t1 = cpuinfo->current_task
	ld t0, 0(t1)			// t0 = current_task->trap_context
	call restore_trapframe
	/*开中断*/
	csrr t0, sstatus
	ori t0, t0, 0x2
	csrw sstatus, t0
	sret










.global save_trapframe
save_trapframe:
    // Save GPRs (x0–x31)
    sd x0,   0(t0)
    sd ra,   8(t0)
    sd sp,  16(t0)
    sd gp,  24(t0)
    sd tp,  32(t0)
    sd t0,  40(t0)
    sd t1,  48(t0)
    sd t2,  56(t0)
    sd s0,  64(t0)
    sd s1,  72(t0)
    sd a0,  80(t0)
    sd a1,  88(t0)
    sd a2,  96(t0)
    sd a3, 104(t0)
    sd a4, 112(t0)
    sd a5, 120(t0)
    sd a6, 128(t0)
    sd a7, 136(t0)
    sd s2, 144(t0)
    sd s3, 152(t0)
    sd s4, 160(t0)
    sd s5, 168(t0)
    sd s6, 176(t0)
    sd s7, 184(t0)
    sd s8, 192(t0)
    sd s9, 200(t0)
    sd s10,208(t0)
    sd s11,216(t0)
    sd t3, 224(t0)
    sd t4, 232(t0)
    sd t5, 240(t0)
    sd t6, 248(t0)

    // Save CSR: offset = 256+
    csrr t1, sstatus
    sd t1, 256(t0)
    csrr t1, stvec
    sd t1, 264(t0)
    csrr t1, sepc
    sd t1, 272(t0)
    csrr t1, scause
    sd t1, 280(t0)
    csrr t1, stval
    sd t1, 288(t0)
    csrr t1, satp
    sd t1, 296(t0)

    // Save FPU: f0–f31, offset = 304+
    fsd f0,  304(t0)
    fsd f1,  312(t0)
    fsd f2,  320(t0)
    fsd f3,  328(t0)
    fsd f4,  336(t0)
    fsd f5,  344(t0)
    fsd f6,  352(t0)
    fsd f7,  360(t0)
    fsd f8,  368(t0)
    fsd f9,  376(t0)
    fsd f10, 384(t0)
    fsd f11, 392(t0)
    fsd f12, 400(t0)
    fsd f13, 408(t0)
    fsd f14, 416(t0)
    fsd f15, 424(t0)
    fsd f16, 432(t0)
    fsd f17, 440(t0)
    fsd f18, 448(t0)
    fsd f19, 456(t0)
    fsd f20, 464(t0)
    fsd f21, 472(t0)
    fsd f22, 480(t0)
    fsd f23, 488(t0)
    fsd f24, 496(t0)
    fsd f25, 504(t0)
    fsd f26, 512(t0)
    fsd f27, 520(t0)
    fsd f28, 528(t0)
    fsd f29, 536(t0)
    fsd f30, 544(t0)
    fsd f31, 552(t0)

    ret


.global restore_trapframe
restore_trapframe:
    // Restore GPRs (x1–x31)
    ld ra,   8(t0)
    ld sp,  16(t0)
    ld gp,  24(t0)
    ld tp,  32(t0)
    ld t0,  40(t0)     // 小心这里 t0 会变，但现在正好用完
    ld t1,  48(t0)
    ld t2,  56(t0)
    ld s0,  64(t0)
    ld s1,  72(t0)
    ld a0,  80(t0)
    ld a1,  88(t0)
    ld a2,  96(t0)
    ld a3, 104(t0)
    ld a4, 112(t0)
    ld a5, 120(t0)
    ld a6, 128(t0)
    ld a7, 136(t0)
    ld s2, 144(t0)
    ld s3, 152(t0)
    ld s4, 160(t0)
    ld s5, 168(t0)
    ld s6, 176(t0)
    ld s7, 184(t0)
    ld s8, 192(t0)
    ld s9, 200(t0)
    ld s10,208(t0)
    ld s11,216(t0)
    ld t3, 224(t0)
    ld t4, 232(t0)
    ld t5, 240(t0)
    ld t6, 248(t0)

    // Restore CSR: offset 256+
    ld t1, 256(t0)
    csrw sstatus, t1
    ld t1, 264(t0)
    csrw stvec, t1
    ld t1, 272(t0)
    csrw sepc, t1
    ld t1, 280(t0)
    csrw scause, t1
    ld t1, 288(t0)
    csrw stval, t1
    ld t1, 296(t0)
    csrw satp, t1

    // Restore FPRs f0–f31
    fld f0,  304(t0)
    fld f1,  312(t0)
    fld f2,  320(t0)
    fld f3,  328(t0)
    fld f4,  336(t0)
    fld f5,  344(t0)
    fld f6,  352(t0)
    fld f7,  360(t0)
    fld f8,  368(t0)
    fld f9,  376(t0)
    fld f10, 384(t0)
    fld f11, 392(t0)
    fld f12, 400(t0)
    fld f13, 408(t0)
    fld f14, 416(t0)
    fld f15, 424(t0)
    fld f16, 432(t0)
    fld f17, 440(t0)
    fld f18, 448(t0)
    fld f19, 456(t0)
    fld f20, 464(t0)
    fld f21, 472(t0)
    fld f22, 480(t0)
    fld f23, 488(t0)
    fld f24, 496(t0)
    fld f25, 504(t0)
    fld f26, 512(t0)
    fld f27, 520(t0)
    fld f28, 528(t0)
    fld f29, 536(t0)
    fld f30, 544(t0)
    fld f31, 552(t0)

    ret
