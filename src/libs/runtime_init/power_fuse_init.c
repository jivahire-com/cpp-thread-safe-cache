//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_fuse_init.c
 * This file contains the config/initialization for power fuse library.
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>  // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <power_tlm_fuse.h>
#include <stddef.h> // for NULL
/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

#ifdef SCP_RUNTIME_INIT
FPFW_INIT_COMPONENT(power_fuse, FPFW_INIT_DEPENDENCIES("fuse_post_mesh"))
{   
#else
FPFW_INIT_COMPONENT(power_fuse, FPFW_INIT_DEPENDENCIES(""))
{
#endif
    /* inform power fuse library that fuse serivice is up*/
    power_fuse_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
