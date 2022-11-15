ENTRY(_start);
PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    /* Definition for the MS-DOS memory allocator */
    . = 0;
    low_mem = .;

    text_start = .;
    .text : ALIGN(4K) {
        . = 0x500; /* Place multiboot on discardable lower memory */
        *(.multiboot .multiboot.*);
        . = 0xF000; /* Highmemory for the DOS part of the OS */
        *(.text.dos);
        . = 1M;
        *(.text .text.*)
    } :text
    text_end = .;

    data_start = .;
    .rodata : ALIGN(4K) {
        *(.rodata .rodata.*)
    } :rodata

    .data : ALIGN(4K) {
        *(.data .data.*)
    } :data

    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss .bss.*)
        . = ALIGN(4K);
        stack_bottom = .;
        . += 32K;
        g_KernStackTop = .;
    } :data
    data_end = .;

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
