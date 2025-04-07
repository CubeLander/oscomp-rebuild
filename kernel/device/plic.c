#include <kernel/mmu.h>
#include <kernel/sched.h>
#include <kernel/types.h>
#include <kernel/util.h>
#include <kernel/device/virtio.h>

//
// the riscv Platform Level Interrupt Controller (PLIC)
// RISCV平台级中断控制器
//

void plicInit() {
	// 开启VIRTIO的中断
	// OpenSBI默认会将大部分中断和异常委托给S模式

	// *(uint32_t*)(PLIC + UART0_IRQ*4) = 1; // 暂时不开启UART的中断
	*(uint32_t *)(PLIC + VIRTIO0_IRQ * 4) = 1;
}

void plicInitHart() {
	int hart = read_tp();

	// 允许VIRTIO的IRQ中断
	*(uint32 *)PLIC_SENABLE(hart) = (1 << VIRTIO0_IRQ);

	// 设置优先级为0
	*(uint32 *)PLIC_SPRIORITY(hart) = 0;
}

/**
 * @brief 向PLIC索要当前中断的IRQ编号
 */
int plicClaim() {
	int hart = read_tp();
	int irq = *(uint32 *)PLIC_SCLAIM(hart);
	return irq;
}

/**
 * @brief 告知plic我们已经处理完了irq代指的中断
 * @param irq 处理完的中断编号
 */
void plicComplete(int irq) {
	int hart = read_tp();
	*(uint32 *)PLIC_SCLAIM(hart) = irq;
}


void external_trap_handler() {
	int irq = plicClaim();

	if (irq == VIRTIO0_IRQ) {
		// Note: call virtio intr handler
		kprintf("[cpu %d] catch virtio intr\n", read_tp());
		virtio_trap_handler();
	} else {
		kprintf("[cpu %d] unknown externel interrupt irq = %d\n", read_tp(), irq);
	}

	if (irq) {
		plicComplete(irq);
	}
}
