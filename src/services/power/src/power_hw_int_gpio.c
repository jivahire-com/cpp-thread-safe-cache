//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_gpio.c
 * Implementation of power hw gpio functions.
 */

/*------------- Includes -----------------*/
#include "power_hw_int_i.h" // for power_hw_gpio_connected, power_hw_gpio_r...

#include <gpio_lib.h>
#include <idsw.h> // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <stdbool.h> // for bool, false

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define GPIO_RACK_LIM_ASSERTED GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_4, 5)

static bool state_rack_limit_override = false;

/*------------- Functions ----------------*/

void power_hw_gpio_set_rack_limit_override(bool state)
{
    // set the rack limit override
    state_rack_limit_override = state;
}

bool power_hw_gpio_connected()
{
    // Return true for PLATFORM_FPGA_LARGE_RVP and PLATFORM_RVP_EVT_SILICON platforms
    if ((idsw_get_platform_sdv() == PLATFORM_FPGA_LARGE_RVP) || (idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON))
    {
        return true;
    }
    // Return false for any other platform
    return false;
}

// function returns true if rack power limit (VR_HOT_SOC) is asserted
bool power_hw_gpio_rack_limit_asserted()
{
    uint32_t state_rack_limit_gpio;
    if (state_rack_limit_override)
    {
        state_rack_limit_gpio = 0;
    }
    else
    {
        gpio_get_input(GPIO_RACK_LIM_ASSERTED, &state_rack_limit_gpio);
    }
    // Active low signal, so return inverted value of state_rack_limit_gpio
    return state_rack_limit_gpio == 0;
}
