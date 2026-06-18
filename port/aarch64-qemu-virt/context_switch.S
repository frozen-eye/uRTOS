/*
 * uRTOS — context save/restore (SMP via TPIDR_EL1 -> os_cpu_t).
 */

    .section ".text", "ax"

    .equ FRAME_REGS,     31
    .equ FRAME_EXTRA,    2
    .equ FRAME_SIZE,     ((FRAME_REGS + FRAME_EXTRA) * 8)
    .equ SPSR_EL1_TASK,   0x005

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
    .global os_fiq_exception
    .global os_serror_exception

    .equ CPU_current,    0
    .equ CPU_id,         8
    .equ CPU_irq_stack,  16
    .equ IRQ_STACK_SIZE, 4096

    .equ TASK_stack_top, 0
    .equ TASK_fn,        16
    .equ TASK_arg,       24
    .equ TASK_stack,     48

    .macro  get_cpu  reg
    mrs     \reg, tpidr_el1
    .endm

os_task_init_context:
    mov     x19, x0
    ldr     x0, [x19, #TASK_stack]
    add     x0, x0, #4096
    sub     x0, x0, #(17 * 16)
    and     x0, x0, #~15

    mov     x1, x0
    add     x2, x0, #(17 * 16)
1:
    cmp     x1, x2
    b.ge    2f
    str     xzr, [x1], #8
    b       1b
2:
    adr     x1, task_trampoline
    str     x1, [x0, #0]
    mov     x2, #SPSR_EL1_TASK
    str     x2, [x0, #8]
    str     x1, [x0, #24]
    str     x19, [x0, #256]
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

os_start_first_task:
    get_cpu x9
    ldr     w10, [x9, #CPU_id]
    cbnz    w10, 1f
    bl      port_timer_start
1:
    ldr     x0, [x9, #CPU_current]
    ldr     x1, [x0, #TASK_stack_top]
    mov     sp, x1
    msr     daifclr, #2
    b       restore_full_frame

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
    mov     x0, x30
    mov     x1, #SPSR_EL1_TASK
    stp     x0, x1, [sp, #-16]!

    get_cpu x9
    ldr     x0, [x9, #CPU_current]
    mov     x2, sp
    str     x2, [x0, #TASK_stack_top]

    bl      os_schedule

    get_cpu x9
    ldr     x0, [x9, #CPU_current]
    cbz     x0, yield_to_idle
    ldr     x1, [x0, #TASK_stack_top]
    mov     sp, x1
    b       restore_full_frame

yield_to_idle:
    bl      os_schedule
    get_cpu x9
    ldr     x0, [x9, #CPU_current]
    cbz     x0, 2f
    ldr     x1, [x0, #TASK_stack_top]
    mov     sp, x1
    b       restore_full_frame
2:
    add     x3, x9, #CPU_irq_stack
    add     sp, x3, #IRQ_STACK_SIZE
    wfi
    b       yield_to_idle

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

    get_cpu x9
    ldr     x2, [x9, #CPU_current]
    cbz     x2, 1f
    mov     x3, sp
    str     x3, [x2, #TASK_stack_top]
1:

    add     x3, x9, #CPU_irq_stack
    add     x3, x3, #IRQ_STACK_SIZE
    mov     sp, x3

    bl      os_irq_handler

    get_cpu x9
    ldr     x0, [x9, #CPU_current]
    cbz     x0, irq_return_idle
    ldr     x1, [x0, #TASK_stack_top]
    mov     sp, x1
    b       restore_full_frame

irq_return_idle:
    add     x3, x9, #CPU_irq_stack
    add     sp, x3, #IRQ_STACK_SIZE
    ldp     x0, x1, [sp], #16
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

os_cpu_wfi:
    get_cpu x9
    add     x3, x9, #CPU_irq_stack
    add     sp, x3, #IRQ_STACK_SIZE
    wfe
    ret

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
