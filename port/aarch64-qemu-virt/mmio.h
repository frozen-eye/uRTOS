#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>

static inline void mmio_write32(volatile uint32_t *addr, uint32_t value)
{
    *addr = value;
}

static inline uint32_t mmio_read32(volatile uint32_t *addr)
{
    return *addr;
}

static inline void mmio_write64(volatile uint64_t *addr, uint64_t value)
{
    *addr = value;
}

static inline uint64_t mmio_read64(volatile uint64_t *addr)
{
    return *addr;
}

#endif /* MMIO_H */
