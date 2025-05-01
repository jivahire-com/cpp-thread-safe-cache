//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_fuse_init.c
 * This file contains the config/initialization for power fuse library.
 */

/*------------- Includes -----------------*/

#include <fpfw_init.h>
#include <power_tlm_fuse.h>
#include <stddef.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(power_fuse, FPFW_INIT_DEPENDENCIES("fuse_en"))
{
    /* inform power fuse library that fuse serivice is up*/
    power_fuse_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
