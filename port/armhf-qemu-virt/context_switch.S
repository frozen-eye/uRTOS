/*
 * uRTOS — ARM32 context switch and IRQ preemption.
 *
 * Frame at stack_top (15 words):
 *   [0]  spsr
 *   [4]  r0  (holds interrupted pc)
 *   [8]  r1
 *   ...
 *   [52] lr
 */

    .syntax unified
    .arch armv7-a
    .section ".text", "ax"

    .equ FRAME_WORDS, 15
    .equ FRAME_BYTES, (FRAME_WORDS * 4)
    .equ CPSR_TASK,    0x13        /* SVC, IRQ enabled */

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
    .global irq_stack_top

    .equ TASK_stack_top, 0
    .equ TASK_fn,        8
    .equ TASK_arg,       12
    .equ TASK_stack,     28

os_task_init_context:
    mov     r4, r0

    ldr     r0, [r4, #TASK_stack]
    add     r0, r0, #4096
    sub     r0, r0, #FRAME_BYTES
    bic     r0, r0, #7

    mov     r1, r0
    add     r2, r0, #FRAME_BYTES
    mov     r3, #0
1:
    cmp     r1, r2
    bge     2f
    str     r3, [r1], #4
    b       1b
2:
    ldr     r1, =task_trampoline
    mov     r2, #CPSR_TASK
    str     r2, [r0, #0]            @ spsr
    str     r1, [r0, #4]            @ r0/pc
    str     r4, [r0, #20]           @ r4 = task pointer

    str     r0, [r4, #TASK_stack_top]
    bx      lr

task_trampoline:
    mov     r5, r4
    ldr     r0, [r5, #TASK_arg]
    ldr     r1, [r5, #TASK_fn]
    blx     r1
1:
    wfi
    b       1b

os_start_first_task:
    bl      port_timer_start

    ldr     r0, =os_current_task
    ldr     r0, [r0]
    ldr     sp, [r0, #TASK_stack_top]

    cpsie   i
    b       restore_full_frame

os_yield:
    mov     r0, lr
    stmfd   sp!, {r0-r12, lr}
    mov     r0, #CPSR_TASK
    stmfd   sp!, {r0}

    ldr     r0, =os_current_task
    ldr     r0, [r0]
    mov     r1, sp
    str     r1, [r0, #TASK_stack_top]

    bl      os_schedule

    ldr     r0, =os_current_task
    ldr     r0, [r0]
    ldr     sp, [r0, #TASK_stack_top]
    b       restore_full_frame

os_irq_handler_asm:
    sub     r0, lr, #4
    stmfd   sp!, {r0-r12, lr}
    mrs     r0, spsr
    stmfd   sp!, {r0}

    ldr     r0, =os_current_task
    ldr     r0, [r0]
    mov     r1, sp
    str     r1, [r0, #TASK_stack_top]

    ldr     sp, =irq_stack_top

    bl      os_irq_handler

    ldr     r0, =os_current_task
    ldr     r0, [r0]
    ldr     sp, [r0, #TASK_stack_top]
    b       restore_full_frame

restore_full_frame:
    ldmfd   sp!, {r0}
    msr     spsr_cxsf, r0
    ldmfd   sp!, {r0-r12, lr}
    mov     pc, r0

os_sync_exception:
    push    {r0, lr}
    ldr     r0, =msg_sync
    bl      os_panic

msg_sync:
    .asciz  "synchronous exception"

    .align  4
    .global irq_stack
irq_stack:
    .space  4096
irq_stack_top:
