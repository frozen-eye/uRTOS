#ifndef KERNEL_H
#define KERNEL_H

#include "os.h"

extern volatile uint64_t os_tick_count;
extern volatile int os_reschedule_pending;

void os_scheduler_init(void);
void os_schedule(void);
void os_tick_handler(void);

void os_critical_enter(void);
void os_critical_exit(void);

#endif /* KERNEL_H */
