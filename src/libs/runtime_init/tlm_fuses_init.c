//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_fuses_init.c
 * This file contains the config/initialization for the telemetry fuses library.
 */

/*------------- Includes -----------------*/

#include <fpfw_init.h>
#include <stddef.h>
#include <tlm_fuses.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

PLACED_CODE FPFW_INIT_COMPONENT(tlm_fuses, FPFW_INIT_DEPENDENCIES("fuse_en"))
{
    // Initialize the telemetry fuses library
    tlm_fuses_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
