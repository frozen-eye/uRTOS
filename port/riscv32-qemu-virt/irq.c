#include "kernel.h"
#include "port.h"
#include "os.h"

static inline uint32_t read_mcause(void)
{
    uint32_t v;
    __asm__ volatile("csrr %0, mcause" : "=r"(v));
    return v;
}

void os_irq_handler(void)
{
    uint32_t cause = read_mcause();

    if (cause == 0x80000007U) {
        os_tick_handler();
        port_timer_reprogram();
        return;
    }

    os_panic("unexpected trap");
}

void port_boot_banner(void)
{
    port_uart_puts("uRTOS/qemu-riscv32 boot\n");
}
