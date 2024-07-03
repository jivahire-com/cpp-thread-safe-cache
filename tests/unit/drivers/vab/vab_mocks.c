//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Mock functions for vab initialization.
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <cmocka.h> // IWYU pragma: keep
#include <vab_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);

    /* Keep mscp base zero to allow checking base address in UTs */
    atu_map_entry->mscp_start_address = 0xffffffff;

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);
    FPFW_UNUSED(atu_map_entry);

    return mock_type(int);
}

int __wrap_vab_init(vab_init_t* vab_init_params)
{
    check_expected(vab_init_params->security_state);
    check_expected(vab_init_params->vab_smmu_gbpa_cfg->sh_cfg);
    check_expected(vab_init_params->system_counter_delay);
    check_expected(vab_init_params->vab_resolved_base_addr);

    function_called();
    return 0;
}