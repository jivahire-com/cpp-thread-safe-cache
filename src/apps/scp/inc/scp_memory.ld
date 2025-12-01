/**
 *
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * This is the main linker script for the SCP. It defines the expected memory layout
 * and sets up necessary components for the common linker script.
*/

/*
 * These are the addresses of ITCM and DTCM.
 * See the address map: https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/SOC%20Top/Kingsgate%20SOC%20Address%20Map%20r1p04.xlsx?d=w1dbe548890594d609964d4ae084eee3a&csf=1&web=1&e=MXoeEK
 * The __ITCMSTART__ has been moved to 0x80 as the M7 VTOR register only accepts addresses with 128 byte offset (https://developer.arm.com/documentation/dui0646/b/Cortex-M7-Peripherals/System-control-block/Vector-Table-Offset-Register?lang=en)
 * The __DTCMEND__ is reduced to 0x20077FFF, this is because the last 32KB is used by the boot loader for decompression and hence the firmware image needs to leave the region untouched.
 * RMSS Data size for SCP is 332kb
 * The __RESERVED_CODE_SIZE is used to create a 32KB region to reserve ITCM for future use.
*/
__ITCMSTART__        = 0x00000080;
__ITCMEND__          = 0x0007FFFF;
__DTCMSTART__        = 0x20000000;
__DTCMEND__          = 0x20077FFF;
__RMSSDATASTART__    = 0x0134D000;
__RMSSDATAEND__      = 0x0139BFFF;

/*
 * ADO 2809728 - Rollback original reserved space
 * after create a new section in RMSS ro region for functions,
 */
__RESERVED_CODE_SIZE = 0;

/*
 * CODE and DATA sizes map directly to the sizes of the respective TCM (plus one for size).
 * The STACKSIZE maps to the System Stack used by ThreadX, set to 4 KB.
 *
 * The regions start address and sizes should adhere to the below MPU Region Recommendations:
 * - Size: The region size should be one of the approved MPU region sizes (smallest being 32 B, largest being 4 GB, following powers of 2).
 * - Start Address: The start address should align with a boundary matching it's size.
 * - For additional MPU Region details see: ARM DDI 0403E.e section B3.5.
*/

__CODE_START__ = __ITCMSTART__;
__CODE_SIZE__  = (__ITCMEND__ - __ITCMSTART__) - (__RESERVED_CODE_SIZE) + 1;
__DATA_START__ = __DTCMSTART__;
__DATA_SIZE__  = (__DTCMEND__ - __DTCMSTART__) + 1;
__STACKSIZE__  = 0x1000;
__RMSS_DATA_START__ = __RMSSDATASTART__;
__RMSS_DATA_SIZE__ = (__RMSSDATAEND__ - __RMSSDATASTART__) + 1;

/* Include the common kernel linker file */
INCLUDE "kernel.ld"