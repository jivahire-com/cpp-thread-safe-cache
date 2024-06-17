//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tower_mocks.c
 * Mock functions for tower sequence
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <cmocka.h> // IWYU pragma: keep
#include <ddrss_lib.h>
#include <silibs_status.h>
#include <stddef.h>

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
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }
    // Keep mscp base non-zero to allow checking base address in UTs
    atu_map_entry->mscp_start_address = 0xffffffff;

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(int);
}

int __wrap_ddrss_init(ddrss_cfg_knobs_t* cfg_knobs)
{
    FPFW_UNUSED(cfg_knobs);
    return mock_type(int);
}