//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_init_scp.c
 * SCP specific initialization of the crash dump.
 */

/*------------- Includes -----------------*/
#include <crash_dump_scp.h> // for crash_dump_init_post_mesh
#include <fpfw_init.h>      // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <stddef.h>         // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(cd_init_pomesh, FPFW_INIT_DEPENDENCIES("cd_init", "mesh"))
{
    crash_dump_init_post_mesh();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
