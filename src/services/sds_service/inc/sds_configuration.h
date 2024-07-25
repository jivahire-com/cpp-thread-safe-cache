//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sds_configuration.h
 * Header containing configuration of SDS service
 */

#pragma once
#include "sds_api.h"

/*----------- Nested includes ------------*/
/*-------------- Typedefs ----------------*/
enum sds_region_index {
    PLATFORM_SDS_REGION_SECURE,
    PLATFORM_SDS_REGION_NONSECURE,
    PLATFORM_SDS_REGION_COUNT,
};

// TBD: This is a placeholder for the actual SDS region configuration
#define SCP_SDS_SECURE_BASE 0x100
#define SCP_SDS_SECURE_SIZE 0x100
#define SCP_SDS_NON_SECURE_BASE 0x100
#define SCP_SDS_NON_SECURE_SIZE 0x100

/*--------- Function Prototypes ----------*/
