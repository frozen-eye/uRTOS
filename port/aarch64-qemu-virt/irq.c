#include "kernel.h"
#include "port.h"
#include "gic.h"
#include "smp.h"

void os_irq_handler(void)
{
    uint32_t irq = gic_acknowledge();

    if (irq == 1023U) {
        return;
    }

    if (irq == (uint32_t)IRQ_VIRT_TIMER) {
        if (port_cpu_id() == 0U) {
            os_tick_handler();
            port_timer_reprogram();
        }
    } else if (irq == (uint32_t)IRQ_SGI_RESCHEDULE) {
        os_schedule();
    }

    port_gic_eoi(irq);
}

void port_boot_banner(void)
{
    port_uart_puts("uRTOS/qemu-aarch64 boot\n");
}
