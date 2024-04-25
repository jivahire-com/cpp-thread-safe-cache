/*
* This is the linker script to be used for building the final MCP binary image.
*  This file is currently a copy of scp_fw.ld and is currently a work in progress
*/

/*
 * The combined binary containing boot loader + firmware is placed in MSCP_EXP RAM0 with 128 bytes left at the start for boot info.
 * The boot loader data is expected to be placed in the lower 32KB of the DTCM hence the address is calculated by subtracting 32KB from end of DTCM
 * See the address map: https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/SOC%20Top/Kingsgate%20SOC%20Address%20Map%20r1p04.xlsx?d=w1dbe548890594d609964d4ae084eee3a&csf=1&web=1&e=MXoeEK
*/
__EXP_RAM0_START__   = 0x1200080;
__EXP_RAM0_END__     = 0x0007FFFF;
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

/* Include the common kernel linker file
*  TODO: The kernel linker script is used as a placeholder for now and will be replaced with final bootloader sections.
*  ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484399
*/
INCLUDE "kernel.ld"