#ifndef OS_SYNC_H
#define OS_SYNC_H

#include <stdint.h>

typedef struct os_task os_task_t;

typedef struct os_mutex {
    uint32_t    lock_word;
    uint8_t     locked;
    os_task_t   *owner;
    os_task_t   *wait_head;
    os_task_t   *wait_tail;
} os_mutex_t;

typedef struct os_sem {
    uint32_t    lock_word;
    uint32_t    count;
    os_task_t   *wait_head;
    os_task_t   *wait_tail;
} os_sem_t;

void os_mutex_init(os_mutex_t *mutex);
int  os_mutex_trylock(os_mutex_t *mutex);
void os_mutex_lock(os_mutex_t *mutex);
void os_mutex_unlock(os_mutex_t *mutex);

void os_sem_init(os_sem_t *sem, uint32_t count);
int  os_sem_trywait(os_sem_t *sem);
void os_sem_wait(os_sem_t *sem);
void os_sem_post(os_sem_t *sem);

#endif /* OS_SYNC_H */
