/*
 * uRTOS — AArch64 boot for QEMU virt (SMP).
 */

    .section ".text.boot", "ax"
    .global _start
    .global secondary_cpu_entry

    .extern os_boot_main
    .extern os_secondary_cpu_main
    .extern __bss_start
    .extern __bss_end
    .extern vector_table
    .extern os_bss_ready

    .equ SPSR_EL1_KERNEL, 0x3C5

_start:
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #3
    b.eq    from_el3
    cmp     x0, #2
    b.eq    from_el2
    cmp     x0, #1
    b.eq    el1_entry
    b       unexpected_el

from_el3:
    mov     x0, #0x3c9
    msr     scr_el3, x0
    mov     x0, #0x3cd
    msr     spsr_el3, x0
    adr     x0, from_el2
    msr     elr_el3, x0
    eret

from_el2:
    mov     x0, #3
    msr     cnthctl_el2, x0
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
    mov     x0, #0
    msr     sctlr_el1, x0

    ldr     x0, =vector_table
    msr     vbar_el1, x0

    mrs     x19, mpidr_el1
    and     x19, x19, #0xFF
    cbnz    x19, secondary_cpu_entry

    ldr     x20, =el1_stack_top
    mov     sp, x20

    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
1:
    cmp     x0, x1
    b.ge    2f
    str     xzr, [x0], #8
    b       1b
2:
    mov     w0, #1
    ldr     x1, =os_bss_ready
    str     w0, [x1]
    sev
    bl      os_boot_main

hang:
    wfi
    b       hang

secondary_cpu_entry:
1:
    ldr     x0, =os_bss_ready
    ldr     w1, [x0]
    cbnz    w1, 2f
    wfe
    b       1b
2:
    ldr     x0, =secondary_stacks
    sub     x1, x19, #1
    add     x0, x0, x1, lsl #12
    add     sp, x0, #4096

    bl      os_secondary_cpu_main

secondary_hang:
    wfi
    b       secondary_hang

unexpected_el:
    b       unexpected_el

    .align  4
    .skip   4096
el1_stack:
    .space  4096
el1_stack_top:

    .align  12
secondary_stacks:
    .space  4096 * 3

    .section ".data"
    .align  4
    .global os_bss_ready
os_bss_ready:
    .word   0
