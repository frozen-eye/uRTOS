/*
 * uRTOS — ARM32 exception vectors for QEMU virt.
 */

    .syntax unified
    .arch armv7-a
    .section ".text", "ax"
    .align  5
    .global vector_table

    .extern os_sync_exception
    .extern os_irq_handler_asm
    .extern os_panic

vector_table:
    ldr     pc, =_start
    ldr     pc, =os_sync_exception
    ldr     pc, =os_sync_exception
    ldr     pc, =os_sync_exception
    ldr     pc, =os_sync_exception
    ldr     pc, =unexpected_vector
    ldr     pc, =os_irq_handler_asm
    ldr     pc, =unexpected_vector

unexpected_vector:
    push    {r0, lr}
    ldr     r0, =msg_unexpected
    bl      os_panic

msg_unexpected:
    .asciz  "unexpected exception vector"
