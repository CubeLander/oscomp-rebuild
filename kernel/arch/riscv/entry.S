.section .text

.global smode_trap_vector
smode_trap_vector:
	csrrsi t0, sstatus, 0x2	# 关中断
	
	csrr t0, sscratch
	ld tp , 0(t0)		# 从sscratch中读取task_struct
	ld


.global save_context
save_context:
    addi sp, sp, -576        # 留出栈空间（35 + 32 个寄存器 × 8 字节）

    # 保存 CSR
    csrr t0, sepc
    sd   t0, 0(sp)
    csrr t0, sstatus
    sd   t0, 256(sp)
    csrr t0, stval
    sd   t0, 264(sp)
    csrr t0, scause
    sd   t0, 272(sp)

    sd a0, 280(sp)           # orig_a0（保存调用时的 a0）

    # 保存整数寄存器
    sd ra,   8(sp)
    sd gp,  24(sp)
    sd tp,  32(sp)
    sd t0,  40(sp)
    sd t1,  48(sp)
    sd t2,  56(sp)
    sd s0,  64(sp)
    sd s1,  72(sp)
    sd a0,  80(sp)
    sd a1,  88(sp)
    sd a2,  96(sp)
    sd a3, 104(sp)
    sd a4, 112(sp)
    sd a5, 120(sp)
    sd a6, 128(sp)
    sd a7, 136(sp)
    sd s2, 144(sp)
    sd s3, 152(sp)
    sd s4, 160(sp)
    sd s5, 168(sp)
    sd s6, 176(sp)
    sd s7, 184(sp)
    sd s8, 192(sp)
    sd s9, 200(sp)
    sd s10, 208(sp)
    sd s11, 216(sp)
    sd t3, 224(sp)
    sd t4, 232(sp)
    sd t5, 240(sp)
    sd t6, 248(sp)
    sd sp,  16(sp)           # 注意最后保存的 sp 是原始 sp（不是新的）

    # 保存 FPU
    fsd ft0,  288(sp)
    fsd ft1,  296(sp)
    fsd ft2,  304(sp)
    fsd ft3,  312(sp)
    fsd ft4,  320(sp)
    fsd ft5,  328(sp)
    fsd ft6,  336(sp)
    fsd ft7,  344(sp)
    fsd fs0,  352(sp)
    fsd fs1,  360(sp)
    fsd fa0,  368(sp)
    fsd fa1,  376(sp)
    fsd fa2,  384(sp)
    fsd fa3,  392(sp)
    fsd fa4,  400(sp)
    fsd fa5,  408(sp)
    fsd fa6,  416(sp)
    fsd fa7,  424(sp)
    fsd fs2,  432(sp)
    fsd fs3,  440(sp)
    fsd fs4,  448(sp)
    fsd fs5,  456(sp)
    fsd fs6,  464(sp)
    fsd fs7,  472(sp)
    fsd fs8,  480(sp)
    fsd fs9,  488(sp)
    fsd fs10, 496(sp)
    fsd fs11, 504(sp)
    fsd ft8,  512(sp)
    fsd ft9,  520(sp)
    fsd ft10, 528(sp)
    fsd ft11, 536(sp)

    ret


.global restore_context
restore_context:
    # 恢复 FPU
    fld ft0,  288(sp)
    fld ft1,  296(sp)
    fld ft2,  304(sp)
    fld ft3,  312(sp)
    fld ft4,  320(sp)
    fld ft5,  328(sp)
    fld ft6,  336(sp)
    fld ft7,  344(sp)
    fld fs0,  352(sp)
    fld fs1,  360(sp)
    fld fa0,  368(sp)
    fld fa1,  376(sp)
    fld fa2,  384(sp)
    fld fa3,  392(sp)
    fld fa4,  400(sp)
    fld fa5,  408(sp)
    fld fa6,  416(sp)
    fld fa7,  424(sp)
    fld fs2,  432(sp)
    fld fs3,  440(sp)
    fld fs4,  448(sp)
    fld fs5,  456(sp)
    fld fs6,  464(sp)
    fld fs7,  472(sp)
    fld fs8,  480(sp)
    fld fs9,  488(sp)
    fld fs10, 496(sp)
    fld fs11, 504(sp)
    fld ft8,  512(sp)
    fld ft9,  520(sp)
    fld ft10, 528(sp)
    fld ft11, 536(sp)

    # 恢复 CSR
    ld t0,  0(sp)
    csrw sepc, t0
    ld t0, 256(sp)
    csrw sstatus, t0
    ld t0, 264(sp)
    csrw stval, t0
    ld t0, 272(sp)
    csrw scause, t0

    # 恢复通用寄存器
    ld ra,   8(sp)
    ld gp,  24(sp)
    ld tp,  32(sp)
    ld t0,  40(sp)
    ld t1,  48(sp)
    ld t2,  56(sp)
    ld s0,  64(sp)
    ld s1,  72(sp)
    ld a0,  80(sp)
    ld a1,  88(sp)
    ld a2,  96(sp)
    ld a3, 104(sp)
    ld a4, 112(sp)
    ld a5, 120(sp)
    ld a6, 128(sp)
    ld a7, 136(sp)
    ld s2, 144(sp)
    ld s3, 152(sp)
    ld s4, 160(sp)
    ld s5, 168(sp)
    ld s6, 176(sp)
    ld s7, 184(sp)
    ld s8, 192(sp)
    ld s9, 200(sp)
    ld s10, 208(sp)
    ld s11, 216(sp)
    ld t3, 224(sp)
    ld t4, 232(sp)
    ld t5, 240(sp)
    ld t6, 248(sp)
    ld sp,  16(sp)      # 恢复原始栈指针

    addi sp, sp, 576    # 回滚整个上下文栈
    ret
