//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_platform.c
 * This file contains APIs for the DDR Manager platform-specific compatibility.
 * This file is being taken over to also provide accessor functions to aid in unit testing.
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"
#include "ddr_memory_map.h"

#include <idhw.h>
#include <idsw.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
bool ddr_manager_platform_is_polling_supported()
{
    return idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON;
}

ddrss_memory_region_t* ddr_manager_get_outgoing_memory_map()
{
    return &outgoing_memory_map[0];
}