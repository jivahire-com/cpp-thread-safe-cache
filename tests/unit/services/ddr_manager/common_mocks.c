//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file common_mocks.c
 * Implementation of common mocks
 */

/*------------- Includes -----------------*/

#include "common_mocks.h"

#include <setjmp.h>
#include <cmocka.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __wrap_ddr_manager_set_thermal_trip_gpio()
{
    function_called();
}