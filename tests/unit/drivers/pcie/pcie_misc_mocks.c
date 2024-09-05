//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Misc. mock functions used by PCIe driver unit tests
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h>     // IWYU pragma: keep
#include <cmocka.h>         // IWYU pragma: keep
#include <pcie_ss_common.h> // for pcie_ss_entity_t, ss_bases_t
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

void __wrap_plat_overrides_post_rp_ready(pcie_ss_entity_t* rpss)
{
    check_expected(rpss);
}