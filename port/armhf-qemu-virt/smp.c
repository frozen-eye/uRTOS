#include "port.h"
#include "gic.h"
#include "smp.h"

#include <stdint.h>

extern os_cpu_t os_cpus[];
extern char _start;

#define PSCI_CPU_ON 0x84000003U

static uint32_t psci_smc(uint32_t fn, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    register uint32_t r0 asm("r0") = fn;
    register uint32_t r1 asm("r1") = arg0;
    register uint32_t r2 asm("r2") = arg1;
    register uint32_t r3 asm("r3") = arg2;

    __asm__ volatile(
        "smc #0"
        : "+r"(r0)
        : "r"(r1), "r"(r2), "r"(r3)
        : "memory");

    return r0;
}

static inline void write_tpidrprw(uint32_t v)
{
    __asm__ volatile("mcr p15, 0, %0, c13, c0, 4" :: "r"(v));
}

static inline uint32_t read_mpidr(void)
{
    uint32_t v;

    __asm__ volatile("mrc p15, 0, %0, c0, c0, 5" : "=r"(v));
    return v;
}

uint32_t port_cpu_id(void)
{
    return read_mpidr() & 0xFFU;
}

void port_wfe(void)
{
    __asm__ volatile("wfe" ::: "memory");
}

void port_cpu_init(uint32_t cpu_id)
{
    write_tpidrprw((uint32_t)(uintptr_t)&os_cpus[cpu_id]);

    if (cpu_id == 0U) {
        return;
    }

    gic_cpu_init();
}

void port_smp_boot_secondaries(void)
{
    for (uint32_t i = 1U; i < OS_CPU_COUNT; i++) {
        uint32_t target = i;
        uint32_t entry = (uint32_t)(uintptr_t)&_start;
        uint32_t rc = psci_smc(PSCI_CPU_ON, target, entry, 0U);

        if (rc != 0U) {
            continue;
        }

        os_cpu_online[i] = 1U;
    }

    __asm__ volatile("dmb" ::: "memory");
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
