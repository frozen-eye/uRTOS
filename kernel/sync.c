#include "os_sync.h"
#include "kernel.h"
#include "smp.h"
#include "spinlock.h"

static os_spinlock_t *obj_lock(uint32_t *word)
{
    return (os_spinlock_t *)word;
}

static os_task_t *sync_current(void)
{
    os_task_t *self = os_cpu_current();

    if (self == NULL) {
        os_panic("sync call outside task");
    }

    return self;
}

static void wait_enqueue(os_task_t **head, os_task_t **tail, os_task_t *task)
{
    task->wait_next = NULL;

    if (*tail != NULL) {
        (*tail)->wait_next = task;
    } else {
        *head = task;
    }

    *tail = task;
}

static os_task_t *wait_dequeue(os_task_t **head, os_task_t **tail)
{
    os_task_t *task = *head;

    if (task == NULL) {
        return NULL;
    }

    *head = task->wait_next;

    if (*head == NULL) {
        *tail = NULL;
    }

    task->wait_next = NULL;
    return task;
}

static void task_prepare_block(os_task_t *self)
{
    os_spinlock_acquire(&os_sched_lock);
    self->state = OS_TASK_BLOCKED;
    self->run_cpu = -1;
    os_spinlock_release(&os_sched_lock);
}

static void task_wake(os_task_t *task)
{
    os_spinlock_acquire(&os_sched_lock);
    task->state = OS_TASK_READY;
    task->run_cpu = -1;
    os_spinlock_release(&os_sched_lock);
}

static void schedule_after_wake(void)
{
#if OS_CPU_COUNT > 1
    os_smp_send_reschedule_ipi();
#endif
    os_yield();
}

void os_mutex_init(os_mutex_t *mutex)
{
    os_spinlock_init(obj_lock(&mutex->lock_word));
    mutex->locked = 0U;
    mutex->owner = NULL;
    mutex->wait_head = NULL;
    mutex->wait_tail = NULL;
}

int os_mutex_trylock(os_mutex_t *mutex)
{
    os_task_t *self = sync_current();
    os_spinlock_t *sl = obj_lock(&mutex->lock_word);

    os_spinlock_acquire(sl);

    if (mutex->locked) {
        if (mutex->owner == self) {
            os_spinlock_release(sl);
            return 0;
        }

        os_spinlock_release(sl);
        return -1;
    }

    mutex->locked = 1U;
    mutex->owner = self;
    os_spinlock_release(sl);
    return 0;
}

void os_mutex_lock(os_mutex_t *mutex)
{
    os_task_t *self = sync_current();
    os_spinlock_t *sl = obj_lock(&mutex->lock_word);

    for (;;) {
        os_spinlock_acquire(sl);

        if (mutex->locked && mutex->owner == self) {
            os_spinlock_release(sl);
            return;
        }

        if (!mutex->locked) {
            mutex->locked = 1U;
            mutex->owner = self;
            os_spinlock_release(sl);
            return;
        }

        wait_enqueue(&mutex->wait_head, &mutex->wait_tail, self);
        task_prepare_block(self);
        os_spinlock_release(sl);
        os_yield();
    }
}

void os_mutex_unlock(os_mutex_t *mutex)
{
    os_task_t *self = sync_current();
    os_spinlock_t *sl = obj_lock(&mutex->lock_word);
    os_task_t *waiter;

    os_spinlock_acquire(sl);

    if (!mutex->locked || mutex->owner != self) {
        os_spinlock_release(sl);
        os_panic("mutex unlock fault");
    }

    waiter = wait_dequeue(&mutex->wait_head, &mutex->wait_tail);
    if (waiter != NULL) {
        mutex->owner = waiter;
        mutex->locked = 1U;
        os_spinlock_release(sl);
        task_wake(waiter);
        schedule_after_wake();
        return;
    }

    mutex->locked = 0U;
    mutex->owner = NULL;
    os_spinlock_release(sl);
}

void os_sem_init(os_sem_t *sem, uint32_t count)
{
    os_spinlock_init(obj_lock(&sem->lock_word));
    sem->count = count;
    sem->wait_head = NULL;
    sem->wait_tail = NULL;
}

int os_sem_trywait(os_sem_t *sem)
{
    os_spinlock_t *sl = obj_lock(&sem->lock_word);

    os_spinlock_acquire(sl);

    if (sem->count == 0U) {
        os_spinlock_release(sl);
        return -1;
    }

    sem->count--;
    os_spinlock_release(sl);
    return 0;
}

void os_sem_wait(os_sem_t *sem)
{
    os_task_t *self = sync_current();
    os_spinlock_t *sl = obj_lock(&sem->lock_word);

    for (;;) {
        os_spinlock_acquire(sl);

        if (sem->count > 0U) {
            sem->count--;
            os_spinlock_release(sl);
            return;
        }

        wait_enqueue(&sem->wait_head, &sem->wait_tail, self);
        task_prepare_block(self);
        os_spinlock_release(sl);
        os_yield();
    }
}

void os_sem_post(os_sem_t *sem)
{
    os_spinlock_t *sl = obj_lock(&sem->lock_word);
    os_task_t *waiter;

    os_spinlock_acquire(sl);
    sem->count++;

    waiter = wait_dequeue(&sem->wait_head, &sem->wait_tail);
    if (waiter != NULL) {
        os_spinlock_release(sl);
        task_wake(waiter);
        schedule_after_wake();
        return;
    }

    os_spinlock_release(sl);
}
