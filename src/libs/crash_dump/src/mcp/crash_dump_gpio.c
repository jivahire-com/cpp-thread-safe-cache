//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_gpio.c
 *    MCP Crash Dump GPIO functionality
 */

/*------------- Includes -----------------*/
#include "../crash_dump_gpio.h" // for cd_gpio_assert_cd_available, cd_gpio_as...

#include <gpio_lib.h> // for gpio_set_output, GPIO_CTRL_PIN_ID, MSCP...
#include <stdbool.h>  // for bool

/*-- Symbolic Constant Macros (defines) --*/
#define SAFE_MODE_REQ       2
#define CD_AVAILABLE        3
#define GPIO_CD_IN_PROGRESS GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_6, SAFE_MODE_REQ)
#define GPIO_CD_AVAILABLE   GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_6, CD_AVAILABLE)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
#ifndef GPIO_DRIVER_AVAILABLE // Stub of gpio until driver is available : https://azurecsi.visualstudio.com/Dev/_workitems/edit/1828509
    #include "FpFwUtils.h" // for FPFW_UNUSED

    #include <stdint.h> // for uint32_t

int gpio_set_output(uint32_t gpio_pin_id, uint32_t level)
{
    FPFW_UNUSED(gpio_pin_id);
    FPFW_UNUSED(level);

    return 0;
}
#endif

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief GPIO assert for Crash Dump in progress
 *
 * @param in_progress true if crash dump is in progress, otherwise false.
 */
void cd_gpio_assert_cd_in_progress(bool in_progress)
{
    // NB: this presumes that the GPIO pin and pad have been correctly configured by SCP
    // This signal is active-low
    gpio_set_output(GPIO_CD_IN_PROGRESS, !in_progress);
}

/**
 * @brief GPIO assert for Crash Dump is available
 *
 * @param available true if crash dump is available, otherwise false.
 */
void cd_gpio_assert_cd_available(bool available)
{
    // NB: this presumes that the GPIO pin and pad have been correctly configured by SCP
    // This signal is active-low
    gpio_set_output(GPIO_CD_AVAILABLE, !available);
}