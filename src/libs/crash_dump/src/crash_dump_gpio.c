//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_gpio.c
 * Crash Dump GPIO functionality
 */

/*------------- Includes -----------------*/
#include "crash_dump_gpio.h" // for cd_gpio_assert_cd_available, cd_gpio_as...

#include <crash_dump.h> // for CRASH_DUMP_CORE_MCP, GetCrashDumpConfig
#include <gpio_lib.h>   // for gpio_set_output, GPIO_CTRL_PIN_ID, MSCP...
#include <stdbool.h>    // for bool

/*-- Symbolic Constant Macros (defines) --*/
#define SAFE_MODE_REQ       2
#define CD_AVAILABLE        3
#define GPIO_CD_IN_PROGRESS GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_6, SAFE_MODE_REQ)
#define GPIO_CD_AVAILABLE   GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_6, CD_AVAILABLE)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief GPIO assert for Crash Dump in progress
 *
 * @param in_progress true if crash dump is in progress, otherwise false.
 */
void cd_gpio_assert_cd_in_progress(bool in_progress)
{
    if (crash_dump_context()->core_index == CRASH_DUMP_CORE_MCP)
    {
        // NB: this presumes that the GPIO pin and pad have been correctly configured by SCP
        // This signal is active-low
        gpio_set_output(GPIO_CD_IN_PROGRESS, !in_progress);
    }
}

/**
 * @brief GPIO assert for Crash Dump is available
 *
 * @param available true if crash dump is available, otherwise false.
 */
void cd_gpio_assert_cd_available(bool available)
{
    if (crash_dump_context()->core_index == CRASH_DUMP_CORE_MCP)
    {
        // NB: this presumes that the GPIO pin and pad have been correctly configured by SCP
        // This signal is active-low
        gpio_set_output(GPIO_CD_AVAILABLE, !available);
    }
}