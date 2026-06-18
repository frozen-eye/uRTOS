OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x40000000, LENGTH = 128M
}

SECTIONS
{
    . = ORIGIN(RAM);

    .text : {
        KEEP(*(.text.boot))
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > RAM

    .data : {
        *(.data .data.*)
    } > RAM

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}
