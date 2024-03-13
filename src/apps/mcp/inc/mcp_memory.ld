/*
This is the main memory map definition for the application firmware

This file defines the usage of the TCM memory that the firmware is loaded into.

TCM memory begins with the firmware code section, which includes the reset vectors. Firmware data
comes after firmware code.

The beginning and end of each of these sections must be on aligned boundaries for the MMU to properly
grant permissions. The alignment requirements are somewhat complex; the alignment of the start of the
region is dependent on the region's size. See the mmu library for more details on how the registers are
programmed.

For simplicity, the following rule can be used.
The MMU rules will be satisfied if all regions:
   * Start at an address that is a multiple of 64kB
   * Have a size that is a multiple of 64kB
   * Are not larger than 512kB

Other alignments are possible. Adjusting a size by an alignment smaller than 64kB may be done, but will
have the possibility of a runtime failure when the MMU register values are computed.
Successfully computing the MMU register values with the MMU library will indicate the value is good.

At present the MMU library uses only a single MMU region to describe a given code or data region.
It could be made to cover more flexible alignments needed by allocating more MMU regions.

*/

/* This common .ld file will be included by the application firmware and other specific ld files */


/* Adjust the required sizes of each of the regions here
   region sizes should be in multiples of 0x10000 (64kB).
   
   These multiples allow the MPU to protect each region with
   the appropriate restrictions */
__CODE_SIZE__ = 0x30000;
__DATA_SIZE__ = 0x20000;
__STACKSIZE__ = 0x8000;

/* This is the address of the start of the TCM */
__ITCMSTART__        = 0x00000000;
__ITCMEND__          = 0x0007FFFF;
__DTCMSTART__        = 0x20000000;
__DTCMEND__          = 0x2007FFFF;

/* Code and data regions for each of the executables follow each other from the beginning of TCM */
__CODE_START__ = __ITCMSTART__;
__CODE_END__   = __CODE_START__ + __CODE_SIZE__;
__DATA_START__ = __DTCMSTART__;
__DATA_END__   = __DATA_START__ + __DATA_SIZE__;

ASSERT(__CODE_END__ <= __ITCMEND__, "Instruction Memory allocation exceeds size of TCM")
ASSERT(__DATA_END__ <= __DTCMEND__, "Data Memory allocation exceeds size of TCM")

/* Include the common kernel linker file */
INCLUDE "kernel.ld"