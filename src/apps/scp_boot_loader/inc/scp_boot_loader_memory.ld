/*
 * This is the linker script used for building the base boot loader image.
 * It contains a mainimage section used to inject the compressed SCP run time firmware
*/

/*
 * The combined binary containing boot loader + firmware is placed in MSCP_EXP RAM0 with 128 bytes left at the start for boot info. 
 * The boot loader data is expected to be placed in the lower 32KB of the DTCM hence the address is calculated by subtracting 32KB from end of DTCM
 * See the address map: https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/SOC%20Top/Kingsgate%20SOC%20Address%20Map%20r1p04.xlsx?d=w1dbe548890594d609964d4ae084eee3a&csf=1&web=1&e=MXoeEK
*/
__EXP_RAM0_START__   = 0x1200080;
__EXP_RAM0_END__     = 0x12FFFFF;
__DTCMSTART__        = 0x20078000;
__DTCMEND__          = 0x2007FFFF;

/* 
 * CODE and DATA sizes map directly to the sizes of the respective TCM (plus one for size).
 * The STACKSIZE is set to 4KB, the stack is expected to be used as part of the decompression code
 *
 * The regions start address and sizes should adhere to the below MPU Region Recommendations:
 * - Size: The region size should be one of the approved MPU region sizes (smallest being 32 B, largest being 4 GB, following powers of 2).
 * - Start Address: The start address should align with a boundary matching it's size.
 * - For additional MPU Region details see: ARM DDI 0403E.e section B3.5.
*/

__CODE_START__ = __EXP_RAM0_START__;
__CODE_SIZE__  = (__EXP_RAM0_END__ - __EXP_RAM0_START__) + 1;
__DATA_START__ = __DTCMSTART__;
__DATA_SIZE__  = (__DTCMEND__ - __DTCMSTART__) + 1;
__STACKSIZE__  = 0x1000;

MEMORY
{
  CODE   (rx) : ORIGIN = __CODE_START__, LENGTH = __CODE_SIZE__
  DATA   (rw) : ORIGIN = __DATA_START__, LENGTH = __DATA_SIZE__
}

SECTIONS
{
    .vectors : 
    {
        KEEP(*(.vectors .vectors.*))
    } > CODE

    .text :
    {
        *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        *(.rodata*)

        KEEP(*(.eh_frame*))
    } > CODE

    .ARM.extab : 
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > CODE

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > CODE
    __exidx_end = .;

   .note.gnu.build-id (READONLY): ALIGN(4)
    {
        _build_id_msdata_start = ABSOLUTE(.);
        *(.note.gnu.build-id)
        . = ALIGN (4);
        _build_id_msdata_end = ABSOLUTE(.);
    } > CODE

    /* Main Image */
    .mainimage (READONLY) : {
        . = ALIGN(4);
        _mainimage_s = .;
        KEEP(*(.mainimage))
        _mainimage_e = .;
    } > CODE    

    .data : 
    {
        __data_load_start__ = LOADADDR(.data);
        __data_start__ = ABSOLUTE(.);

        *(.data*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);

        /* All data end */
        __data_end__ = .;
    } > DATA AT> CODE

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > DATA

    /* Use the remaining data allotment for the .heap section */
    /* The C runtime expects .heap to follow .bss */
    .heap (NOLOAD):
    {
        __heap_start__ = ALIGN(4);
        /* "end" indicates the end of .bss and beginning of the .heap */
        PROVIDE (end = .);
        PROVIDE (_end = .);
        FILL(0x00);
        *(.heap)
        /* Move the pointer to the desired beginning of the stack minus one byte
           then use the BYTE(0x00) command to insert a byte at the end of this section.
           This ensures that the section will contain data and will be appropriately filled
           in the output */
        . = ORIGIN(DATA) + LENGTH(DATA) - 1 - __STACKSIZE__;
        BYTE(0x00);
        __heap_end__ = ALIGN(4);
        PROVIDE(heap_end = .);
    } > DATA

    .stack ALIGN(4)(NOLOAD):
    {
        __stack_start__ = ALIGN(4);
        FILL(0x00)
        *(.stack)
        . = ALIGN(. + __STACKSIZE__, 4) - 1;
        BYTE(0)
        __stack_end__ = ALIGN(4);
        PROVIDE (__stack = .);
    } > DATA

    __HEAPSIZE__ = __heap_end__ - __heap_start__;

    __RAM_segment_used_end__ = .;
}