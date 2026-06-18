#include "port.h"
#include "gic.h"
#include "smp.h"

#include <stdint.h>

extern os_cpu_t os_cpus[];
extern char _start;

#define PSCI_CPU_ON 0xC4000003UL

static uint64_t psci_hvc(uint64_t fn, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    register uint64_t x0 asm("x0") = fn;
    register uint64_t x1 asm("x1") = arg0;
    register uint64_t x2 asm("x2") = arg1;
    register uint64_t x3 asm("x3") = arg2;

    __asm__ volatile(
        "hvc #0"
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x3)
        : "memory");

    return x0;
}

static inline void write_tpidr_el1(uint64_t v)
{
    __asm__ volatile("msr tpidr_el1, %0" :: "r"(v));
}

static inline uint64_t read_mpidr_el1(void)
{
    uint64_t v;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(v));
    return v;
}

uint32_t port_cpu_id(void)
{
    return (uint32_t)(read_mpidr_el1() & 0xFFU);
}

void port_wfe(void)
{
    __asm__ volatile("wfe" ::: "memory");
}

void port_cpu_init(uint32_t cpu_id)
{
    write_tpidr_el1((uint64_t)(uintptr_t)&os_cpus[cpu_id]);

    if (cpu_id == 0U) {
        return;
    }

    gic_cpu_init();
}

void port_smp_boot_secondaries(void)
{
    for (uint32_t i = 1U; i < OS_CPU_COUNT; i++) {
        uint64_t target = (uint64_t)i;
        uint64_t entry = (uint64_t)(uintptr_t)&_start;
        uint64_t rc = psci_hvc(PSCI_CPU_ON, target, entry, 0U);

        if (rc != 0U) {
            continue;
        }

        os_cpu_online[i] = 1U;
    }

    __asm__ volatile("dmb ish" ::: "memory");
    __asm__ volatile("sev" ::: "memory");
}

void port_smp_send_reschedule_ipi(void)
{
    uint32_t me = port_cpu_id();
    uint16_t mask = 0U;

    for (uint32_t i = 0U; i < OS_CPU_COUNT; i++) {
        if (i != me && os_cpu_online[i] != 0U) {
            mask |= (uint16_t)(1U << i);
        }
    }

    if (mask != 0U) {
        gic_send_sgi(IRQ_SGI_RESCHEDULE, mask);
    }
}
