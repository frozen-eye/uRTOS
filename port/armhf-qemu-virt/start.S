/*
 * uRTOS — ARM32 (armhf) boot for QEMU virt.
 */

    .syntax unified
    .arch armv7-a
    .section ".text.boot", "ax"
    .global _start

    .extern os_boot_main
    .extern __bss_start
    .extern __bss_end
    .extern vector_table

_start:
    /* SVC mode, IRQ/FIQ masked. */
    mov     r0, #0xD3
    msr     cpsr_c, r0

    /* MMU off. */
    mrc     p15, 0, r0, c1, c0, 0
    bic     r0, r0, #1
    mcr     p15, 0, r0, c1, c0, 0

    /* Vector base. */
    ldr     r0, =vector_table
    mcr     p15, 0, r0, c12, c0, 0

    ldr     sp, =svc_stack_top

    mov     r2, #0
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
1:
    cmp     r0, r1
    bge     2f
    str     r2, [r0], #4
    b       1b
2:
    bl      os_boot_main

hang:
    wfi
    b       hang

    .align  4
    .skip   4096
svc_stack:
    .space  4096
svc_stack_top:
