//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_gpio_scp.c
 *    SCP Crash Dump GPIO functionality
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>       // for FPFW_UNUSED
#include <crash_dump_gpio.h> // for cd_gpio_assert_cd_available, cd_gpio_as...
#include <stdbool.h>         // for bool

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void cd_gpio_assert_cd_in_progress(bool in_progress)
{
    // MCP controls GPIO. No-op.
    FPFW_UNUSED(in_progress);
}

void cd_gpio_assert_cd_available(bool available)
{
    // MCP controls GPIO. No-op.
    FPFW_UNUSED(available);
}