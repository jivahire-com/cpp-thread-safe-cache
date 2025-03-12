//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file kingsgate_boot.h
 * Header file containing macro and structure definitions to be used as part of the boot loader flow
 */

#define __NO_CSR_TYPEDEFS__
#define __NO_ADDRMAP_TYPEDEFS__

/*----------- Nested includes ------------*/

#include <stdint.h>
#include <stddef.h>

#pragma once

// TODO: The header file needs to have struct definitions for boot config and function prototypes added
// TODO: Add function definitions for unpacking and loading firmware image into ITCM along with decompression logic
// ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484413

/*-- Symbolic Constant Macros (defines) --*/

#define BITMASK_WARM_BOOT          (0b1000)

/*-------------- Typedefs ----------------*/

typedef enum {
    MSCP_CPU_START = 0x90,	
    MSCP_CPU_SCP,
    MSCP_CPU_MCP,
    MSCP_CPU_INVALID
} mscp_cpu_t;

typedef struct kingsgate_boot_config_s {
    size_t      data_src_base;
    size_t      data_src_end;

    size_t      itc_ram_base;
    uint32_t    itc_ram_size;
    size_t      dtc_ram_base;
    uint32_t    dtc_ram_size;
	size_t      rmss_data_base;
    uint32_t    rmss_data_size;
    
	size_t      boot_meta_base;

    mscp_cpu_t  cpu_type;
} kingsgate_boot_config_t;

// TODO: Investigate if its possible to share metadata struct across other IPs
// ADI: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1777866/?triage=true
typedef struct kingsgate_boot_metadata_s {
    //
    // Metadata version
    //
    int8_t meta_data_version;

    //
    // ResetReason
    //
    int8_t reset_reason;

    //
    // Boot Mode (Test mode? Green/ Red mode? etc)
    //
    int8_t boot_mode;

    //
    // Reserved
    //
    int8_t reserved;
} kingsgate_boot_metadata_t;

/*--------- Function Prototypes ----------*/

/**
 *  @brief This function validates the boot config, decompresses the firmware image
 *         and returns the boot entry address.
 *
 *  @note This is a boot loader function and is expected to run with interrupts disabled
 * 
 *  @param[in] boot_config
 *  Pointer of type kingsgate_boot_config_t, contains info needed to validate and unpack
 *  firmware image
 *
 *  @return
 *      returns pointer to boot address - is NULL if it is a failure
 */
void* load_image(kingsgate_boot_config_t* boot_config);
