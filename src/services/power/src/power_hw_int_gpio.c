//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_gpio.c
 * Implementation of power hw gpio functions.
 */

/*------------- Includes -----------------*/
#include "power_hw_int_i.h" // for power_hw_gpio_connected, power_hw_gpio_r...

#include <stdbool.h> // for bool, false

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// TODO:  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491041/
// these will be needed for control loop implementation

bool power_hw_gpio_connected()
{
    // return true only if the platform has functioning GPIO;
    // used to enable SW control of input on dev platforms
    return false;
}

// function returns true if rack power limit (VR_HOT_SOC) is asserted
bool power_hw_gpio_rack_limit_asserted()
{
    return false;
}
