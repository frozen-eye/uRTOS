/*
 * uRTOS — AArch64 boot for QEMU virt.
 *
 * Ensures execution at EL1 with MMU disabled, sets VBAR_EL1, and
 * establishes EL1 stack before entering C code.
 */

    .section ".text.boot", "ax"
    .global _start

    .extern os_boot_main
    .extern __bss_start
    .extern __bss_end
    .extern vector_table

    .equ SPSR_EL1_KERNEL, 0x3C5   /* EL1h, DAIF masked */

_start:
    /* Determine current exception level. */
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #2
    b.eq    from_el2
    cmp     x0, #1
    b.eq    el1_entry
    b       unexpected_el

from_el2:
    /* Allow EL1 access to the physical timer and counter. */
    mov     x0, #3
    msr     cnthctl_el2, x0

    /* EL1 non-secure, AArch64. */
    mov     x0, #(1 << 31)
    msr     hcr_el2, x0

    mov     x0, #0
    msr     sctlr_el1, x0

    adr     x0, el1_entry
    msr     elr_el2, x0
    mov     x0, #SPSR_EL1_KERNEL
    msr     spsr_el2, x0
    eret

el1_entry:
    /* Configure EL1: MMU off, caches off. */
    mov     x0, #0
    msr     sctlr_el1, x0

    /* Install exception vector table. */
    ldr     x0, =vector_table
    msr     vbar_el1, x0

    /* EL1 stack (SP_EL1). */
    ldr     x0, =el1_stack_top
    mov     sp, x0

    /* Zero BSS. */
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
1:
    cmp     x0, x1
    b.ge    2f
    str     xzr, [x0], #8
    b       1b
2:
    bl      os_boot_main

    /* Should never return. */
hang:
    wfi
    b       hang

unexpected_el:
    /* Unsupported exception level — spin. */
    b       unexpected_el

    .align  4
    .skip   4096
el1_stack:
    .space  4096
el1_stack_top:
