/* AtomicOS Kernel Linker Script */
/* Deterministic memory layout for real-time guarantees */

ENTRY(_start)

SECTIONS
{
    /* Kernel starts at 0x10000 (64KB) */
    . = 0x10000;
    
    /* Code section - read-only, executable */
    .text ALIGN(4K) : 
    {
        *(.text)
        *(.text.*)
    }
    
    /* Read-only data */
    .rodata ALIGN(4K) :
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Initialized data */
    .data ALIGN(4K) :
    {
        *(.data)
        *(.data.*)
    }
    
    /* BSS section - uninitialized data */
    .bss ALIGN(4K) :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }
    
    /* End of kernel */
    __kernel_end = .;
    
    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.GNU-stack)
    }
}