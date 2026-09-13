OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    /* QEMU multiboot загружает образ с 1 МБ — ниже лежит VGA/BIOS. */
    . = 1M;

    .text : ALIGN(4K)
    {
        /* multiboot-заголовок должен попадать в первые 8 КБ образа */
        KEEP(*(.multiboot))
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .init_array : ALIGN(4)
    {
        __init_array_start = .;
        KEEP(*(.init_array))
        KEEP(*(.init_array.*))
        KEEP(*(.ctors))
        KEEP(*(.ctors.*))
        __init_array_end = .;
    }

    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    kernel_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}
