#include <kernel.h>
#include <kernel/drivers/plic.h>
//
// the riscv Platform Level Interrupt Controller (PLIC).
//

void plic_init(void) {
    // set desired IRQ priorities non-zero (otherwise disabled).
    *(uint32 *) (PLIC + UART0_IRQ * 4) = 1;
    *(uint32 *) (PLIC + VIRTIO0_IRQ * 4) = 1;
}

void plic_init_hart(void) {

    // set enable bits for this hart's S-mode
    // for the uart and virtio disk.
    *(uint32 *) PLIC_SENABLE(hartid) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);

    // set this hart's S-mode priority threshold to 0.
    *(uint32 *) PLIC_SPRIORITY(hartid) = 0;
}

// ask the PLIC what interrupt we should serve.
int plic_claim(void) {
#ifndef QEMU
    const int irq = *(uint32 *) PLIC_MCLAIM(hartid);
#else
    const int irq = *(uint32 *) PLIC_SCLAIM(hartid);
#endif
    return irq;
}

// tell the PLIC we've served this IRQ.
void plic_complete(const int irq) {
#ifndef QEMU
    *(uint32 *) PLIC_MCLAIM(hartid) = irq;
#else
    *(uint32 *) PLIC_SCLAIM(hartid) = irq;
#endif
}
