OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
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
