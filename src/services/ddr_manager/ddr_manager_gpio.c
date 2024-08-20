//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_gpio.c
 * This file contains the implementation for the DDR Manager GPIO APIs
 *
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"

#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void ddr_manager_set_thermal_trip_gpio()
{
    // Assert the GPIO pin
    // This will trigger the thermal trip
    // This is a stub implementation
    // Replace with actual implementation
    // ADO: #1983318
    printf("Setting DDR thermal trip GPIO\n");
}