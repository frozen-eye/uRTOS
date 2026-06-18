#ifndef OS_SPINLOCK_H
#define OS_SPINLOCK_H

#include <stdint.h>

typedef struct {
    volatile uint32_t lock;
} os_spinlock_t;

void os_spinlock_init(os_spinlock_t *sl);
void os_spinlock_acquire(os_spinlock_t *sl);
void os_spinlock_release(os_spinlock_t *sl);

#endif /* OS_SPINLOCK_H */
