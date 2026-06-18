#include "spinlock.h"

void os_spinlock_init(os_spinlock_t *sl)
{
    sl->lock = 0U;
}

void os_spinlock_acquire(os_spinlock_t *sl)
{
#if defined(__aarch64__)
    uint32_t tmp;

    __asm__ volatile(
            "1:\n"
            "ldaxr   %w0, %1\n"
            "cbnz    %w0, 2f\n"
            "stxr    %w0, %w2, %1\n"
            "cbnz    %w0, 1b\n"
            "b       3f\n"
            "2:\n"
            "clrex\n"
            "wfe\n"
            "b       1b\n"
            "3:"
            : "=&r"(tmp), "+Q"(sl->lock)
            : "r"(1U)
            : "memory");
#elif defined(__arm__)
    uint32_t tmp;
    uint32_t one = 1U;

    __asm__ volatile(
        "1:\n"
        "ldrex   %0, [%1]\n"
        "cmp     %0, #0\n"
        "bne     2f\n"
        "strex   %0, %3, [%1]\n"
        "cmp     %0, #0\n"
        "bne     1b\n"
        "b       3f\n"
        "2:\n"
        "clrex\n"
        "wfe\n"
        "b       1b\n"
        "3:"
        : "=&r"(tmp)
        : "r"(&sl->lock), "r"(one)
        : "memory");
#else
    while (sl->lock != 0U) {
        __asm__ volatile("" ::: "memory");
    }
    sl->lock = 1U;
#endif
}

void os_spinlock_release(os_spinlock_t *sl)
{
#if defined(__aarch64__)
    __asm__ volatile(
        "stlr    %w0, %1\n"
        "sev"
        :
        : "r"(0U), "Q"(sl->lock)
        : "memory");
#else
    sl->lock = 0U;
    __asm__ volatile("dmb" ::: "memory");
#if defined(__arm__) || defined(__aarch64__)
    __asm__ volatile("sev" ::: "memory");
#endif
#endif
}
