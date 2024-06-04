//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_gpio_mcp.c
 *    MCP Crash Dump GPIO functionality
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
/**
 * @brief ToDo: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484997
 *        Implement GPIO assert for CD in progress
 *
 * @param in_progress
 */
void cd_gpio_assert_cd_in_progress(bool in_progress)
{
    FPFW_UNUSED(in_progress);
}

/**
 * @brief ToDo: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484997
 *        Implement GPIO assert for CD available
 *
 * @param available
 */
void cd_gpio_assert_cd_available(bool available)
{
    FPFW_UNUSED(available);
}