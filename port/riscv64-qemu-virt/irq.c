#include "kernel.h"
#include "port.h"
#include "os.h"

static inline uint64_t read_mcause(void)
{
    uint64_t v;
    __asm__ volatile("csrr %0, mcause" : "=r"(v));
    return v;
}

void os_irq_handler(void)
{
    uint64_t cause = read_mcause();

    if (cause == 0x8000000000000007UL) {
        os_tick_handler();
        port_timer_reprogram();
        return;
    }

    os_panic("unexpected trap");
}

void port_boot_banner(void)
{
    port_uart_puts("uRTOS/qemu-riscv64 boot\n");
}
