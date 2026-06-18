#include "heap.h"
#include "os_config.h"

#include <stdint.h>

#define HEAP_ALIGN          16U
#define HEAP_MAP_CAPACITY   (OS_MAX_TASKS * 2U + 4U)

typedef struct {
    uint32_t offset;
    uint32_t size;
    uint8_t  used;
} heap_region_t;

static uint8_t os_heap[OS_HEAP_SIZE] __attribute__((aligned(16)));
static heap_region_t heap_map[HEAP_MAP_CAPACITY];
static size_t map_count;

static size_t align_up(size_t size)
{
    return (size + HEAP_ALIGN - 1U) & ~(HEAP_ALIGN - 1U);
}

static int insert_region(size_t index, uint32_t offset, uint32_t size, uint8_t used)
{
    if (map_count >= HEAP_MAP_CAPACITY) {
        return -1;
    }

    for (size_t i = map_count; i > index; i--) {
        heap_map[i] = heap_map[i - 1U];
    }

    heap_map[index].offset = offset;
    heap_map[index].size = size;
    heap_map[index].used = used;
    map_count++;
    return 0;
}

static void remove_region(size_t index)
{
    if (index >= map_count) {
        return;
    }

    for (size_t i = index; i + 1U < map_count; i++) {
        heap_map[i] = heap_map[i + 1U];
    }
    map_count--;
}

static void coalesce_map(void)
{
    size_t i = 0;

    while (i < map_count) {
        if (heap_map[i].used) {
            i++;
            continue;
        }

        if (i + 1U < map_count && !heap_map[i + 1U].used) {
            heap_map[i].size += heap_map[i + 1U].size;
            remove_region(i + 1U);
            continue;
        }

        i++;
    }
}

static heap_region_t *region_from_ptr(void *ptr)
{
    uintptr_t base = (uintptr_t)os_heap;
    uintptr_t addr = (uintptr_t)ptr;

    if (addr < base || addr >= base + OS_HEAP_SIZE) {
        return NULL;
    }

    uint32_t offset = (uint32_t)(addr - base);

    for (size_t i = 0; i < map_count; i++) {
        if (heap_map[i].offset == offset && heap_map[i].used) {
            return &heap_map[i];
        }
    }

    return NULL;
}

void os_heap_init(void)
{
    map_count = 1U;
    heap_map[0].offset = 0U;
    heap_map[0].size = OS_HEAP_SIZE;
    heap_map[0].used = 0U;
}

void *os_mem_alloc(size_t size)
{
    size_t need = align_up(size);
    if (need == 0U) {
        need = HEAP_ALIGN;
    }

    for (size_t i = 0; i < map_count; i++) {
        if (heap_map[i].used || heap_map[i].size < need) {
            continue;
        }

        if (heap_map[i].size > need + HEAP_ALIGN) {
            uint32_t tail_offset = heap_map[i].offset + (uint32_t)need;
            uint32_t tail_size = heap_map[i].size - (uint32_t)need;

            if (insert_region(i + 1U, tail_offset, tail_size, 0) != 0) {
                /* Map full — use the whole free region without splitting. */
            } else {
                heap_map[i].size = (uint32_t)need;
            }
        }

        heap_map[i].used = 1U;
        return os_heap + heap_map[i].offset;
    }

    return NULL;
}

void os_mem_free(void *ptr)
{
    heap_region_t *region;

    if (ptr == NULL) {
        return;
    }

    region = region_from_ptr(ptr);
    if (region == NULL) {
        return;
    }

    region->used = 0U;
    coalesce_map();
}
