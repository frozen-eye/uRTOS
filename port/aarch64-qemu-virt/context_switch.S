/*
 * uRTOS — context save/restore for cooperative yield and IRQ preemption.
 *
 * IRQ stack frame (SP points here after save, 34 x 8 = 272 bytes):
 *   [0]  x0
 *   ...
 *   [18] x18
 *   [19] x19
 *   ...
 *   [29] x29
 *   [30] x30
 *   [31] elr_el1
 *   [32] spsr_el1
 *
 * Cooperative yield frame (7 x 8 = 56 bytes):
 *   x19-x28, x29(fp), x30(lr)
 */

    .section ".text", "ax"

    .equ FRAME_REGS,     31
    .equ FRAME_EXTRA,    2
    .equ FRAME_SIZE,     ((FRAME_REGS + FRAME_EXTRA) * 8)
    .equ YIELD_SIZE,     (12 * 8)
    .equ SPSR_EL1_TASK,   0x005   /* EL1h, IRQ/FIQ unmasked */

    .extern os_current_task
    .extern os_schedule
    .extern os_irq_handler
    .extern os_panic
    .extern port_timer_start

    .global os_yield
    .global os_task_init_context
    .global os_start_first_task
    .global os_sync_exception
    .global os_irq_handler_asm
    .global os_fiq_exception
    .global os_serror_exception
    .global irq_stack_top

/* os_task_t field offsets */
    .equ TASK_stack_top, 0
    .equ TASK_fn,        16
    .equ TASK_arg,       24
    .equ TASK_stack,     48

/*
 * void os_task_init_context(os_task_t *task)
 * Build an IRQ-compatible frame (elr/spsr at lowest address).
 */
os_task_init_context:
    mov     x19, x0

    ldr     x0, [x19, #TASK_stack]
    add     x0, x0, #4096           /* OS_TASK_STACK_SIZE */
    sub     x0, x0, #(17 * 16)      /* 17 stp pairs == IRQ save size */
    and     x0, x0, #~15            /* frame base == future SP (elr/spsr) */

    mov     x1, x0
    add     x2, x0, #(17 * 16)
1:
    cmp     x1, x2
    b.ge    2f
    str     xzr, [x1], #8
    b       1b
2:
    adr     x1, task_trampoline
    str     x1, [x0, #0]            /* elr_el1 */
    mov     x2, #SPSR_EL1_TASK
    str     x2, [x0, #8]            /* spsr_el1 */
    str     x1, [x0, #24]           /* x30 slot (x29/x30 pair at +16) */
    str     x19, [x0, #256]         /* x0 slot (task pointer) */

    str     x0, [x19, #TASK_stack_top]
    ret

task_trampoline:
    mov     x19, x0
    ldr     x0, [x19, #TASK_arg]
    ldr     x1, [x19, #TASK_fn]
    blr     x1
1:
    wfi
    b       1b

/*
 * void os_start_first_task(void)
 * Start running the current task (frame prepared by os_task_init_context).
 */
os_start_first_task:
    bl      port_timer_start

    ldr     x0, =os_current_task
    ldr     x0, [x0]
    ldr     x1, [x0, #TASK_stack_top]
    mov     sp, x1

    msr     daifclr, #2

    b       restore_full_frame

/*
 * void os_yield(void)
 * Cooperative context switch — saves the same full frame as IRQ preemption.
 */
os_yield:
    stp     x0, x1, [sp, #-16]!
    stp     x2, x3, [sp, #-16]!
    stp     x4, x5, [sp, #-16]!
    stp     x6, x7, [sp, #-16]!
    stp     x8, x9, [sp, #-16]!
    stp     x10, x11, [sp, #-16]!
    stp     x12, x13, [sp, #-16]!
    stp     x14, x15, [sp, #-16]!
    stp     x16, x17, [sp, #-16]!
    stp     x18, xzr, [sp, #-16]!

    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    stp     x27, x28, [sp, #-16]!
    stp     x29, x30, [sp, #-16]!

    /* Synthetic return state: resume after the bl os_yield call site. */
    mov     x0, x30
    mov     x1, #SPSR_EL1_TASK
    stp     x0, x1, [sp, #-16]!

    ldr     x0, =os_current_task
    ldr     x0, [x0]
    mov     x2, sp
    str     x2, [x0, #TASK_stack_top]

    bl      os_schedule

    ldr     x0, =os_current_task
    ldr     x0, [x0]
    ldr     x1, [x0, #TASK_stack_top]
    mov     sp, x1
    b       restore_full_frame

/*
 * IRQ vector entry — save full preempted-task context on its stack.
 */
os_irq_handler_asm:
    stp     x0, x1, [sp, #-16]!
    stp     x2, x3, [sp, #-16]!
    stp     x4, x5, [sp, #-16]!
    stp     x6, x7, [sp, #-16]!
    stp     x8, x9, [sp, #-16]!
    stp     x10, x11, [sp, #-16]!
    stp     x12, x13, [sp, #-16]!
    stp     x14, x15, [sp, #-16]!
    stp     x16, x17, [sp, #-16]!
    stp     x18, xzr, [sp, #-16]!

    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    stp     x27, x28, [sp, #-16]!
    stp     x29, x30, [sp, #-16]!

    mrs     x0, elr_el1
    mrs     x1, spsr_el1
    stp     x0, x1, [sp, #-16]!

    ldr     x2, =os_current_task
    ldr     x2, [x2]
    mov     x3, sp
    str     x3, [x2, #TASK_stack_top]

    ldr     x3, =irq_stack_top
    mov     sp, x3

    bl      os_irq_handler

    ldr     x0, =os_current_task
    ldr     x0, [x0]
    ldr     x1, [x0, #TASK_stack_top]
    mov     sp, x1
    b       restore_full_frame

restore_full_frame:
    ldp     x0, x1, [sp], #16
    msr     spsr_el1, x1
    msr     elr_el1, x0

    ldp     x29, x30, [sp], #16
    ldp     x27, x28, [sp], #16
    ldp     x25, x26, [sp], #16
    ldp     x23, x24, [sp], #16
    ldp     x21, x22, [sp], #16
    ldp     x19, x20, [sp], #16

    ldp     x18, xzr, [sp], #16
    ldp     x16, x17, [sp], #16
    ldp     x14, x15, [sp], #16
    ldp     x12, x13, [sp], #16
    ldp     x10, x11, [sp], #16
    ldp     x8, x9, [sp], #16
    ldp     x6, x7, [sp], #16
    ldp     x4, x5, [sp], #16
    ldp     x2, x3, [sp], #16
    ldp     x0, x1, [sp], #16

    eret

os_sync_exception:
    stp     x0, x1, [sp, #-16]!
    adr     x0, msg_sync
    bl      os_panic

os_fiq_exception:
    stp     x0, x1, [sp, #-16]!
    adr     x0, msg_fiq
    bl      os_panic

os_serror_exception:
    stp     x0, x1, [sp, #-16]!
    adr     x0, msg_serror
    bl      os_panic

msg_sync:
    .asciz  "synchronous exception"
msg_fiq:
    .asciz  "FIQ exception"
msg_serror:
    .asciz  "SError exception"

    .align  4
    .global irq_stack
irq_stack:
    .space  4096
irq_stack_top:
