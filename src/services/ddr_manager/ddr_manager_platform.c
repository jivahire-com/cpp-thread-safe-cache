//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_platform.c
 * This file contains APIs for the DDR Manager platform-specific compatibility.
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"

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