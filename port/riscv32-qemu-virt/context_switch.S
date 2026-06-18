/*
 * uRTOS — RISC-V 32-bit context switch and timer preemption.
 *
 * Frame at stack_top (33 x 4 bytes):
 *   mepc, mstatus, x1 .. x31
 */

    .section ".text", "ax"

    .equ FRAME_BYTES, (33 * 4)
    .equ MSTATUS_TASK, 0x00001888

    .extern os_current_task
    .extern os_schedule
    .extern os_irq_handler
    .extern os_panic
    .extern port_timer_start

    .global os_yield
    .global os_task_init_context
    .global os_start_first_task
    .global os_sync_exception
    .global os_trap_handler_asm
    .global irq_stack_top

    .equ TASK_stack_top, 0
    .equ TASK_fn,        8
    .equ TASK_arg,       12
    .equ TASK_stack,     28

os_task_init_context:
    mv      t0, a0

    lw      a0, TASK_stack(t0)
    addi    a0, a0, 4096
    addi    a0, a0, -FRAME_BYTES
    andi    a0, a0, ~15

    mv      t1, a0
    addi    t2, a0, FRAME_BYTES
1:
    bge     t1, t2, 2f
    sw      zero, 0(t1)
    addi    t1, t1, 4
    j       1b
2:
    la      t1, task_trampoline
    sw      t1, 0(a0)               /* mepc */
    li      t2, MSTATUS_TASK
    sw      t2, 4(a0)               /* mstatus */
    sw      t0, 68(a0)               /* x16 = task pointer */

    sw      a0, TASK_stack_top(t0)
    ret

task_trampoline:
    mv      t0, a6
    lw      a0, TASK_arg(t0)
    lw      t1, TASK_fn(t0)
    jalr    t1
1:
    wfi
    j       1b

os_start_first_task:
    call    port_timer_start

    la      t0, os_current_task
    lw      t0, 0(t0)
    lw      sp, TASK_stack_top(t0)

    li      t1, 0x80                /* MTIE */
    csrs    mie, t1
    csrs    mstatus, 0x8            /* MIE */

    j       restore_full_frame

os_yield:
    addi    sp, sp, -FRAME_BYTES

    sw      ra, 0(sp)
    li      t0, MSTATUS_TASK
    sw      t0, 4(sp)

    sw      x1, 8(sp)
    sw      x2, 12(sp)
    sw      x3, 16(sp)
    sw      x4, 20(sp)
    sw      x5, 24(sp)
    sw      x6, 28(sp)
    sw      x7, 32(sp)
    sw      x8, 36(sp)
    sw      x9, 40(sp)
    sw      x10, 44(sp)
    sw      x11, 48(sp)
    sw      x12, 52(sp)
    sw      x13, 56(sp)
    sw      x14, 60(sp)
    sw      x15, 64(sp)
    sw      x16, 68(sp)
    sw      x17, 72(sp)
    sw      x18, 76(sp)
    sw      x19, 80(sp)
    sw      x20, 84(sp)
    sw      x21, 88(sp)
    sw      x22, 92(sp)
    sw      x23, 96(sp)
    sw      x24, 100(sp)
    sw      x25, 104(sp)
    sw      x26, 108(sp)
    sw      x27, 112(sp)
    sw      x28, 116(sp)
    sw      x29, 120(sp)
    sw      x30, 124(sp)
    sw      x31, 128(sp)

    la      t0, os_current_task
    lw      t0, 0(t0)
    sw      sp, TASK_stack_top(t0)

    call    os_schedule

    la      t0, os_current_task
    lw      t0, 0(t0)
    lw      sp, TASK_stack_top(t0)
    j       restore_full_frame

os_trap_handler_asm:
    addi    sp, sp, -FRAME_BYTES

    csrr    t0, mepc
    csrr    t1, mstatus
    sw      t0, 0(sp)
    sw      t1, 4(sp)

    sw      x1, 8(sp)
    sw      x2, 12(sp)
    sw      x3, 16(sp)
    sw      x4, 20(sp)
    sw      x5, 24(sp)
    sw      x6, 28(sp)
    sw      x7, 32(sp)
    sw      x8, 36(sp)
    sw      x9, 40(sp)
    sw      x10, 44(sp)
    sw      x11, 48(sp)
    sw      x12, 52(sp)
    sw      x13, 56(sp)
    sw      x14, 60(sp)
    sw      x15, 64(sp)
    sw      x16, 68(sp)
    sw      x17, 72(sp)
    sw      x18, 76(sp)
    sw      x19, 80(sp)
    sw      x20, 84(sp)
    sw      x21, 88(sp)
    sw      x22, 92(sp)
    sw      x23, 96(sp)
    sw      x24, 100(sp)
    sw      x25, 104(sp)
    sw      x26, 108(sp)
    sw      x27, 112(sp)
    sw      x28, 116(sp)
    sw      x29, 120(sp)
    sw      x30, 124(sp)
    sw      x31, 128(sp)

    la      t0, os_current_task
    lw      t0, 0(t0)
    sw      sp, TASK_stack_top(t0)

    la      sp, irq_stack_top

    call    os_irq_handler

    la      t0, os_current_task
    lw      t0, 0(t0)
    lw      sp, TASK_stack_top(t0)
    j       restore_full_frame

restore_full_frame:
    lw      t0, 0(sp)
    lw      t1, 4(sp)
    csrw    mepc, t0
    csrw    mstatus, t1

    lw      x1, 8(sp)
    lw      x2, 12(sp)
    lw      x3, 16(sp)
    lw      x4, 20(sp)
    lw      x5, 24(sp)
    lw      x6, 28(sp)
    lw      x7, 32(sp)
    lw      x8, 36(sp)
    lw      x9, 40(sp)
    lw      x10, 44(sp)
    lw      x11, 48(sp)
    lw      x12, 52(sp)
    lw      x13, 56(sp)
    lw      x14, 60(sp)
    lw      x15, 64(sp)
    lw      x16, 68(sp)
    lw      x17, 72(sp)
    lw      x18, 76(sp)
    lw      x19, 80(sp)
    lw      x20, 84(sp)
    lw      x21, 88(sp)
    lw      x22, 92(sp)
    lw      x23, 96(sp)
    lw      x24, 100(sp)
    lw      x25, 104(sp)
    lw      x26, 108(sp)
    lw      x27, 112(sp)
    lw      x28, 116(sp)
    lw      x29, 120(sp)
    lw      x30, 124(sp)
    lw      x31, 128(sp)

    addi    sp, sp, FRAME_BYTES
    mret

os_sync_exception:
    la      a0, msg_sync
    call    os_panic

msg_sync:
    .asciz  "synchronous exception"

    .align  4
    .global irq_stack
irq_stack:
    .space  4096
irq_stack_top:
