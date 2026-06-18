#include "port.h"

uint32_t port_cpu_id(void)
{
    return 0U;
}

void port_cpu_init(uint32_t cpu_id)
{
    (void)cpu_id;
}

void port_wfe(void)
{
    __asm__ volatile("wfi" ::: "memory");
}

void port_smp_boot_secondaries(void)
{
}

void port_smp_send_reschedule_ipi(void)
{
}

void os_cpu_wfi(void)
{
    __asm__ volatile("wfi" ::: "memory");
}
