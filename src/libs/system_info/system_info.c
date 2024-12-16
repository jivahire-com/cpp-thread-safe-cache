//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file system_info.c
 * Implements system info APIs
 */

/*------------- Includes -----------------*/
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#define __NO_CSR_TYPEDEFS__
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <stdint.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
#define MASK_WARM_START (0b1000)
/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bool is_hsp_present = false;
static bool is_warm_start = false;

/*------------- Functions ----------------*/
bool system_info_is_hsp_present()
{
    return is_hsp_present;
}
bool system_info_is_warm_start()
{
    return is_warm_start;
}
void system_info_init()
{
    HSP_BOOT_METADATA* boot_meta_data = (HSP_BOOT_METADATA*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS);
    // Temporary until HSP writes actual metadata instead of arbitrary values
    if (boot_meta_data->MetadataVersion == 0x1 && boot_meta_data->ResetReason == 0x7)
    {
        is_hsp_present = true;
    }
    if (boot_meta_data->MetadataVersion == 0x1 && (boot_meta_data->ResetReason & MASK_WARM_START))
    {
        is_warm_start = true;
    }
}