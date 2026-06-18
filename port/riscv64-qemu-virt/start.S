/*
 * uRTOS — RISC-V (M-mode) boot for QEMU virt.
 */

    .section ".text.boot", "ax"
    .global _start

    .extern os_boot_main
    .extern __bss_start
    .extern __bss_end
    .extern trap_vector

_start:
    la      sp, boot_stack_top

    la      t0, trap_vector
    csrw    mtvec, t0

    la      t0, __bss_start
    la      t1, __bss_end
1:
    bge     t0, t1, 2f
    sd      zero, 0(t0)
    addi    t0, t0, 8
    j       1b
2:
    call    os_boot_main

hang:
    wfi
    j       hang

    .align  4
    .skip   4096
boot_stack:
    .space  4096
boot_stack_top:
