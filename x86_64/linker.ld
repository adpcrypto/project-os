ENTRY(_start)

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

SECTIONS
{
    /* GRUB loads kernel starting at 1MB physical address */
    . = 1M;

    .text :
    {
        KEEP(*(.multiboot2))
        *(.text)
    }

    .rodata :   { *(.rodata) }
    .data   :   { *(.data) }
    .bss    :   { *(.bss) }
}