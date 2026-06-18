#include "port.h"
#include "gic.h"
#include "os_config.h"

#include <stdint.h>

static uint64_t timer_freq;

static inline uint32_t read_cntfrq(void)
{
    uint32_t v;
    __asm__ volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(v));
    return v;
}

static inline void write_cntp_tval(uint32_t v)
{
    __asm__ volatile("mcr p15, 0, %0, c14, c2, 0" :: "r"(v));
}

static inline void write_cntp_ctl(uint32_t v)
{
    __asm__ volatile("mcr p15, 0, %0, c14, c2, 1" :: "r"(v));
}

void port_timer_init(void)
{
    timer_freq = read_cntfrq();
    if (timer_freq == 0) {
        timer_freq = 62500000;
    }

    port_timer_reprogram();
    write_cntp_ctl(0);
}

void port_timer_start(void)
{
    write_cntp_ctl(1);
}

void port_timer_reprogram(void)
{
    uint32_t tval = (uint32_t)(timer_freq / OS_TICK_HZ);
    if (tval == 0) {
        tval = 1;
    }
    write_cntp_tval(tval);
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
    __asm__ volatile("cpsie i" ::: "memory");
}

void port_irq_disable(void)
{
    __asm__ volatile("cpsid i" ::: "memory");
}

void port_early_init(void)
{
    port_uart_init();
}
