//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ioss_mocks.c
 * IOSS mock functions
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <atu_lib.h>    // for atu_id_t, atu_map_entry_t
#include <cmocka.h>     // for mock_type
#include <ioss_init.h>
#include <stdint.h> // for uint64_t, uintptr_t, uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    FPFW_UNUSED(atu_id);
    FPFW_UNUSED(atu_map_entry);

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    FPFW_UNUSED(atu_id);
    FPFW_UNUSED(atu_map_entry);

    return mock_type(int);
}

void __wrap_ioss_init(IOSS_INSTANCE ioss_id, ioss_init_t* init)
{
    FPFW_UNUSED(ioss_id);
    FPFW_UNUSED(init);
    function_called();
}
