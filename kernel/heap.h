#ifndef OS_HEAP_H
#define OS_HEAP_H

#include <stddef.h>

void os_heap_init(void);
void *os_mem_alloc(size_t size);
void os_mem_free(void *ptr);

#endif /* OS_HEAP_H */
