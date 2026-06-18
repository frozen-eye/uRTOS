/*
 * uRTOS — RISC-V context switch and timer preemption.
 *
 * Frame at stack_top (33 x 8 bytes):
 *   mepc, mstatus, x1 .. x31
 */

    .section ".text", "ax"

    .equ FRAME_BYTES, (33 * 8)
    .equ MSTATUS_TASK, 0x00001888   /* MPP=M, MPIE=1, MIE=1 on mret */

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
    .equ TASK_fn,        16
    .equ TASK_arg,       24
    .equ TASK_stack,     48

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
    sd      zero, 0(t1)
    addi    t1, t1, 8
    j       1b
2:
    la      t1, task_trampoline
    sd      t1, 0(a0)               /* mepc */
    li      t2, MSTATUS_TASK
    sd      t2, 8(a0)               /* mstatus */
    sd      t0, 136(a0)             /* x16 = task pointer */

    sd      a0, TASK_stack_top(t0)
    ret

task_trampoline:
    mv      t0, a6
    ld      a0, TASK_arg(t0)
    ld      t1, TASK_fn(t0)
    jalr    t1
1:
    wfi
    j       1b

os_start_first_task:
    call    port_timer_start

    la      t0, os_current_task
    ld      t0, 0(t0)
    ld      sp, TASK_stack_top(t0)

    li      t1, 0x80                /* MTIE in mie */
    csrs    mie, t1
    csrs    mstatus, 0x8            /* MIE */

    j       restore_full_frame

os_yield:
    addi    sp, sp, -FRAME_BYTES

    sd      ra, 0(sp)
    li      t0, MSTATUS_TASK
    sd      t0, 8(sp)

    sd      x1, 16(sp)
    sd      x2, 24(sp)
    sd      x3, 32(sp)
    sd      x4, 40(sp)
    sd      x5, 48(sp)
    sd      x6, 56(sp)
    sd      x7, 64(sp)
    sd      x8, 72(sp)
    sd      x9, 80(sp)
    sd      x10, 88(sp)
    sd      x11, 96(sp)
    sd      x12, 104(sp)
    sd      x13, 112(sp)
    sd      x14, 120(sp)
    sd      x15, 128(sp)
    sd      x16, 136(sp)
    sd      x17, 144(sp)
    sd      x18, 152(sp)
    sd      x19, 160(sp)
    sd      x20, 168(sp)
    sd      x21, 176(sp)
    sd      x22, 184(sp)
    sd      x23, 192(sp)
    sd      x24, 200(sp)
    sd      x25, 208(sp)
    sd      x26, 216(sp)
    sd      x27, 224(sp)
    sd      x28, 232(sp)
    sd      x29, 240(sp)
    sd      x30, 248(sp)
    sd      x31, 256(sp)

    la      t0, os_current_task
    ld      t0, 0(t0)
    sd      sp, TASK_stack_top(t0)

    call    os_schedule

    la      t0, os_current_task
    ld      t0, 0(t0)
    ld      sp, TASK_stack_top(t0)
    j       restore_full_frame

os_trap_handler_asm:
    addi    sp, sp, -FRAME_BYTES

    csrr    t0, mepc
    csrr    t1, mstatus
    sd      t0, 0(sp)
    sd      t1, 8(sp)

    sd      x1, 16(sp)
    sd      x2, 24(sp)
    sd      x3, 32(sp)
    sd      x4, 40(sp)
    sd      x5, 48(sp)
    sd      x6, 56(sp)
    sd      x7, 64(sp)
    sd      x8, 72(sp)
    sd      x9, 80(sp)
    sd      x10, 88(sp)
    sd      x11, 96(sp)
    sd      x12, 104(sp)
    sd      x13, 112(sp)
    sd      x14, 120(sp)
    sd      x15, 128(sp)
    sd      x16, 136(sp)
    sd      x17, 144(sp)
    sd      x18, 152(sp)
    sd      x19, 160(sp)
    sd      x20, 168(sp)
    sd      x21, 176(sp)
    sd      x22, 184(sp)
    sd      x23, 192(sp)
    sd      x24, 200(sp)
    sd      x25, 208(sp)
    sd      x26, 216(sp)
    sd      x27, 224(sp)
    sd      x28, 232(sp)
    sd      x29, 240(sp)
    sd      x30, 248(sp)
    sd      x31, 256(sp)

    la      t0, os_current_task
    ld      t0, 0(t0)
    sd      sp, TASK_stack_top(t0)

    la      sp, irq_stack_top

    call    os_irq_handler

    la      t0, os_current_task
    ld      t0, 0(t0)
    ld      sp, TASK_stack_top(t0)
    j       restore_full_frame

restore_full_frame:
    ld      t0, 0(sp)
    ld      t1, 8(sp)
    csrw    mepc, t0
    csrw    mstatus, t1

    ld      x1, 16(sp)
    ld      x2, 24(sp)
    ld      x3, 32(sp)
    ld      x4, 40(sp)
    ld      x5, 48(sp)
    ld      x6, 56(sp)
    ld      x7, 64(sp)
    ld      x8, 72(sp)
    ld      x9, 80(sp)
    ld      x10, 88(sp)
    ld      x11, 96(sp)
    ld      x12, 104(sp)
    ld      x13, 112(sp)
    ld      x14, 120(sp)
    ld      x15, 128(sp)
    ld      x16, 136(sp)
    ld      x17, 144(sp)
    ld      x18, 152(sp)
    ld      x19, 160(sp)
    ld      x20, 168(sp)
    ld      x21, 176(sp)
    ld      x22, 184(sp)
    ld      x23, 192(sp)
    ld      x24, 200(sp)
    ld      x25, 208(sp)
    ld      x26, 216(sp)
    ld      x27, 224(sp)
    ld      x28, 232(sp)
    ld      x29, 240(sp)
    ld      x30, 248(sp)
    ld      x31, 256(sp)

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
