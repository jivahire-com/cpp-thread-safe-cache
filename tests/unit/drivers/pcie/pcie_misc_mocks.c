//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Misc. mock functions used by PCIe driver unit tests
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <cmocka.h>     // IWYU pragma: keep
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __wrap_enable_vab_isrs(uint16_t vab_instances_to_init)
{
    check_expected(vab_instances_to_init);
}
