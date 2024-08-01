//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_init.c
 * Initialization of the crash dump.
 */

/*------------- Includes -----------------*/
#include <crash_dump.h> // for crash_dump_init
#include <fpfw_init.h>  // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <stddef.h>     // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(cd_init, FPFW_INIT_DEPENDENCIES("gpio_lib"))
{
    crash_dump_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
