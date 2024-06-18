//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_gpio.h
 * APIs for GPIO interaction
 */
#pragma once

/*---------- Nested Includes -------------*/
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void cd_gpio_assert_cd_in_progress(bool in_progress);

void cd_gpio_assert_cd_available(bool available);