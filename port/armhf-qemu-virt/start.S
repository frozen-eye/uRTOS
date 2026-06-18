/*
 * uRTOS — ARM32 (armhf) boot for QEMU virt (SMP).
 */

    .syntax unified
    .arch armv7-a
    .section ".text.boot", "ax"
    .global _start
    .global secondary_cpu_entry

    .extern os_boot_main
    .extern os_secondary_cpu_main
    .extern __bss_start
    .extern __bss_end
    .extern vector_table
    .extern os_bss_ready

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

    mrc     p15, 0, r10, c0, c0, 5
    and     r10, r10, #0xFF
    cmp     r10, #0
    bne     secondary_cpu_entry

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
    mov     r2, #1
    ldr     r0, =os_bss_ready
    str     r2, [r0]
    dsb
    sev
    bl      os_boot_main

hang:
    wfi
    b       hang

secondary_cpu_entry:
1:
    ldr     r0, =os_bss_ready
    ldr     r1, [r0]
    cmp     r1, #0
    bne     2f
    wfe
    b       1b
2:
    ldr     r0, =secondary_stacks
    sub     r1, r10, #1
    mov     r2, #4096
    mul     r1, r1, r2
    add     sp, r0, r1
    add     sp, sp, #4096

    bl      os_secondary_cpu_main

secondary_hang:
    wfi
    b       secondary_hang

    .align  4
    .skip   4096
svc_stack:
    .space  4096
svc_stack_top:

    .align  12
secondary_stacks:
    .space  4096 * 3

    .section ".data"
    .align  4
    .global os_bss_ready
os_bss_ready:
    .word   0
