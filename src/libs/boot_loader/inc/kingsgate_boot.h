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

#pragma once

// TODO: The header file needs to have struct definitions for boot config and function prototypes added
// TODO: Add function definitions for unpacking and loading firmware image into ITCM along with decompression logic
// ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484413

/*-- Symbolic Constant Macros (defines) --*/

#define BITMASK_WARM_BOOT           0b10000000

/*-------------- Typedefs ----------------*/

// TODO: Investigate if its possible to share metadata struct across other IPs
// ADI: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1777866/?triage=true
typedef struct kingsgate_boot_metadata_s {
    //
    // Metadata version
    //
    int8_t MetadataVersion;

    //
    // ResetReason
    //
    int8_t ResetReason;

    //
    // Boot Mode (Test mode? Green/ Red mode? etc)
    //
    int8_t BootMode;

    //
    // Reserved
    //
    int8_t Reserved;
} kingsgate_boot_metadata_t;

/*--------- Function Prototypes ----------*/
