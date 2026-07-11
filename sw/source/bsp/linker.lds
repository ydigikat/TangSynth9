/*------------------------------------------------------------------------------
 * Jason Wilden 2026
 *-----------------------------------------------------------------------------*/

MEMORY
{
    /* 16KB SRAM starting at address 0 */
    ram (rwx) : ORIGIN = 0x00000000, LENGTH = 16K
    voiceram (rw) : ORIGIN = 0x00010000, LENGTH = 1K
}

/* Although this will get rid of RWX warning, the picorv32 has no MPU so ignores these which means
   it is enirely possible for the program to accidentally write over its own code (text) 
   and constants (rodata) 
*/
PHDRS
{
    text PT_LOAD FLAGS(5);   /* R+X */
    rodata PT_LOAD FLAGS(4); /* R */
    data PT_LOAD FLAGS(6);   /* R+W */
}

SECTIONS
{
    /* Code section */
    .text : {
        . = ALIGN(4);
        KEEP(*(.text.start))      /* Reset vector  */
        KEEP(*(.text.irq))        /* IRQ vector */
        *(.text*)                 /* Rest of the code */
        . = ALIGN(4);
    } > ram :text

    /* Read-only data */
    .rodata : {
        . = ALIGN(4);
        *(.rodata*)
        . = ALIGN(4);
    } > ram :rodata

    /* Initialized data */
    .data : {
        . = ALIGN(4);
        __data_start = .;
        *(.data*)
        . = ALIGN(4);
        __data_end = .;
    } > ram :data

    /* Uninitialized data (BSS) cleared in startup.s*/
    .bss : {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > ram :data

    /* Stack grows down from end of RAM */
    /* Stack pointer will point to top of RAM, first push goes to top-4 */
    .stack (NOLOAD) : {
        . = ALIGN(16);
        __stack_bottom = .;
        . += 0x100;  /* Reserve 256 bytes for stack checking */
        . = ALIGN(16);
         __ram_end = .;  /* stash for use in overflow assert.  voiceram moves . */
    } > ram

    /* Voice RAM - separate dual ported BRAM for voice data exchange */
    .voice_params (NOLOAD) : {
      *(.voice_params)
    } > voiceram

    /* Stack top is at the very end of RAM */
    __stack_top = ORIGIN(ram) + LENGTH(ram);  

    /* Check memory structure for integrity */
    ASSERT(__stack_bottom < __stack_top, "ERROR: Stack overlaps with .bss")
    ASSERT(__ram_end <= __stack_top, "ERROR: Program too large for RAM")    
    ASSERT(SIZEOF(.voice_params) <= 0x400, "ERROR: voice_params exceeds voiceram size")

    /* Discard unwanted sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}

ENTRY(_start)