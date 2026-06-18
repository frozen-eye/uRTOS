/*
 * uRTOS — AArch64 EL1 exception vectors.
 *
 * Layout: 4 groups x 4 entries (Sync, IRQ, FIQ, SError).
 * Kernel runs at EL1 using SP_EL1, so Current EL with SPx is used.
 */

    .section ".text", "ax"
    .align  11
    .global vector_table

    .extern os_sync_exception
    .extern os_irq_handler_asm
    .extern os_fiq_exception
    .extern os_serror_exception

    .macro  vector_entry handler
    .align  7
    b       \handler
    .endm

vector_table:
    /* Current EL with SP0 — unused in this port. */
    vector_entry unexpected_vector
    vector_entry unexpected_vector
    vector_entry unexpected_vector
    vector_entry unexpected_vector

    /* Current EL with SPx — kernel IRQs and exceptions. */
    vector_entry os_sync_exception
    vector_entry os_irq_handler_asm
    vector_entry os_fiq_exception
    vector_entry os_serror_exception

    /* Lower EL AArch64 — unused (no user tasks yet). */
    vector_entry unexpected_vector
    vector_entry unexpected_vector
    vector_entry unexpected_vector
    vector_entry unexpected_vector

    /* Lower EL AArch32 — unused. */
    vector_entry unexpected_vector
    vector_entry unexpected_vector
    vector_entry unexpected_vector
    vector_entry unexpected_vector

    .section ".text", "ax"
    .align  4

unexpected_vector:
    stp     x0, x1, [sp, #-16]!
    adr     x0, msg_unexpected
    bl      os_panic
    /* not reached */

msg_unexpected:
    .asciz  "unexpected exception vector"
