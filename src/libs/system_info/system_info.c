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

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
bool system_info_is_hsp_present()
{
    HSP_BOOT_METADATA* boot_meta_data = (HSP_BOOT_METADATA*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS);
    // Temporary until HSP writes actual metadata instead of arbitrary values
    return boot_meta_data->MetadataVersion == 0x1 && boot_meta_data->ResetReason == 0x7;
}
