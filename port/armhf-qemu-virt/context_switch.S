/*
 * uRTOS — ARM32 context switch and IRQ preemption (SMP via TPIDRPRW).
 */

    .syntax unified
    .arch armv7-a
    .section ".text", "ax"

    .equ FRAME_WORDS, 15
    .equ FRAME_BYTES, (FRAME_WORDS * 4)
    .equ CPSR_TASK,    0x13

    .extern os_schedule
    .extern os_irq_handler
    .extern os_panic
    .extern port_timer_start

    .global os_cpu_wfi
    .global os_yield
    .global os_task_init_context
    .global os_start_first_task
    .global os_sync_exception
    .global os_irq_handler_asm

    .equ CPU_current,    0
    .equ CPU_id,         4
    .equ CPU_irq_stack,  8
    .equ IRQ_STACK_SIZE, 4096

    .equ TASK_stack_top, 0
    .equ TASK_fn,        8
    .equ TASK_arg,       12
    .equ TASK_stack,     28

    .macro  get_cpu  reg
    mrc     p15, 0, \reg, c13, c0, 4
    .endm

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
    str     r2, [r0, #0]
    str     r1, [r0, #4]
    str     r4, [r0, #20]

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
    get_cpu r9
    ldr     r10, [r9, #CPU_id]
    cmp     r10, #0
    bne     1f
    bl      port_timer_start
1:
    ldr     r0, [r9, #CPU_current]
    ldr     sp, [r0, #TASK_stack_top]
    cpsie   i
    b       restore_full_frame

os_yield:
    mov     r0, lr
    stmfd   sp!, {r0-r12, lr}
    mov     r0, #CPSR_TASK
    stmfd   sp!, {r0}

    get_cpu r9
    ldr     r0, [r9, #CPU_current]
    mov     r1, sp
    str     r1, [r0, #TASK_stack_top]

    bl      os_schedule

    get_cpu r9
    ldr     r0, [r9, #CPU_current]
    cmp     r0, #0
    beq     yield_to_idle
    ldr     sp, [r0, #TASK_stack_top]
    b       restore_full_frame

yield_to_idle:
    bl      os_schedule
    get_cpu r9
    ldr     r0, [r9, #CPU_current]
    cmp     r0, #0
    beq     2f
    ldr     sp, [r0, #TASK_stack_top]
    b       restore_full_frame
2:
    add     r3, r9, #CPU_irq_stack
    add     sp, r3, #IRQ_STACK_SIZE
    wfe
    b       yield_to_idle

os_irq_handler_asm:
    sub     r0, lr, #4
    stmfd   sp!, {r0-r12, lr}
    mrs     r0, spsr
    stmfd   sp!, {r0}

    get_cpu r9
    ldr     r0, [r9, #CPU_current]
    cmp     r0, #0
    beq     1f
    mov     r1, sp
    str     r1, [r0, #TASK_stack_top]
1:
    add     r3, r9, #CPU_irq_stack
    add     sp, r3, #IRQ_STACK_SIZE

    bl      os_irq_handler

    get_cpu r9
    ldr     r0, [r9, #CPU_current]
    cmp     r0, #0
    beq     irq_return_idle
    ldr     sp, [r0, #TASK_stack_top]
    b       restore_full_frame

irq_return_idle:
    add     r3, r9, #CPU_irq_stack
    add     sp, r3, #IRQ_STACK_SIZE
    ldmfd   sp!, {r0}
    msr     spsr_cxsf, r0
    ldmfd   sp!, {r0-r12, lr}
    movs    pc, lr

restore_full_frame:
    ldmfd   sp!, {r0}
    msr     spsr_cxsf, r0
    ldmfd   sp!, {r0-r12, lr}
    mov     pc, r0

os_cpu_wfi:
    get_cpu r9
    add     r3, r9, #CPU_irq_stack
    add     sp, r3, #IRQ_STACK_SIZE
    wfe
    bx      lr

os_sync_exception:
    push    {r0, lr}
    ldr     r0, =msg_sync
    bl      os_panic

msg_sync:
    .asciz  "synchronous exception"
