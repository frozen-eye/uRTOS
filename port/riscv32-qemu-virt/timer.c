#include "port.h"
#include "mmio.h"
#include "os_config.h"

#include <stdint.h>

#define CLINT_BASE          0x02000000UL
#define CLINT_MTIME_LO      (CLINT_BASE + 0xBFF8UL)
#define CLINT_MTIME_HI      (CLINT_BASE + 0xBFFCU)
#define CLINT_MTIMECMP_LO   (CLINT_BASE + 0x4000UL)
#define CLINT_MTIMECMP_HI   (CLINT_BASE + 0x4004UL)

static uint64_t timer_freq;

static uint64_t read_mtime(void)
{
    uint32_t hi = mmio_read32((volatile uint32_t *)CLINT_MTIME_HI);
    uint32_t lo = mmio_read32((volatile uint32_t *)CLINT_MTIME_LO);
    return ((uint64_t)hi << 32) | lo;
}

static void write_mtimecmp0(uint64_t when)
{
    /* High word first to avoid a spurious timer interrupt on RV32. */
    mmio_write32((volatile uint32_t *)CLINT_MTIMECMP_HI, 0xFFFFFFFFU);
    mmio_write32((volatile uint32_t *)CLINT_MTIMECMP_LO, (uint32_t)when);
    mmio_write32((volatile uint32_t *)CLINT_MTIMECMP_HI, (uint32_t)(when >> 32));
}

void port_timer_init(void)
{
    timer_freq = 10000000UL;
    port_timer_reprogram();
}

void port_timer_start(void)
{
    uint64_t now = read_mtime();
    write_mtimecmp0(now + (timer_freq / OS_TICK_HZ));
}

void port_timer_reprogram(void)
{
    uint64_t now = read_mtime();
    uint64_t delta = timer_freq / OS_TICK_HZ;
    if (delta == 0) {
        delta = 1;
    }
    write_mtimecmp0(now + delta);
}

uint64_t port_timer_frequency(void)
{
    return timer_freq;
}

void port_irq_enable(void)
{
    __asm__ volatile("csrs mstatus, %0" :: "r"(0x8U));
}

void port_irq_disable(void)
{
    __asm__ volatile("csrc mstatus, %0" :: "r"(0x8U));
}

void port_early_init(void)
{
    port_uart_init();
}
