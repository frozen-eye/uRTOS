/*
 * uRTOS — RISC-V M-mode trap vector (direct mode).
 */

    .section ".text", "ax"
    .align  4
    .global trap_vector

    .extern os_trap_handler_asm

trap_vector:
    j       os_trap_handler_asm
