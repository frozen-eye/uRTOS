#include "port.h"
#include "gic.h"
#include "os_config.h"

#include <stdint.h>

static uint64_t timer_freq;

static inline uint64_t read_cntfrq_el0(void)
{
    uint64_t v;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v));
    return v;
}

static inline uint64_t read_cntpct_el0(void)
{
    uint64_t v;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(v));
    return v;
}

static inline void write_cntp_tval_el0(uint32_t v)
{
    __asm__ volatile("msr cntp_tval_el0, %0" :: "r"((uint64_t)v));
}

static inline void write_cntp_ctl_el0(uint32_t v)
{
    __asm__ volatile("msr cntp_ctl_el0, %0" :: "r"((uint64_t)v));
}

static inline uint32_t read_cntp_ctl_el0(void)
{
    uint64_t v;
    __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r"(v));
    return (uint32_t)v;
}

void port_timer_init(void)
{
    timer_freq = read_cntfrq_el0();
    if (timer_freq == 0) {
        timer_freq = 62500000; /* QEMU virt fallback */
    }

    port_timer_reprogram();
    write_cntp_ctl_el0(0); /* armed but disabled until port_timer_start() */
}

void port_timer_start(void)
{
    write_cntp_ctl_el0(1); /* enable, unmasked */
}

void port_timer_reprogram(void)
{
    uint32_t tval = (uint32_t)(timer_freq / OS_TICK_HZ);
    if (tval == 0) {
        tval = 1;
    }
    write_cntp_tval_el0(tval);
}

uint64_t port_timer_frequency(void)
{
    return timer_freq;
}

void port_gic_init(void)
{
    gic_init();
}

void port_gic_eoi(uint32_t irq)
{
    gic_end_interrupt(irq);
}

void port_irq_enable(void)
{
    __asm__ volatile("msr daifclr, #2" ::: "memory");
}

void port_irq_disable(void)
{
    __asm__ volatile("msr daifset, #2" ::: "memory");
}

void port_early_init(void)
{
    port_uart_init();
}
