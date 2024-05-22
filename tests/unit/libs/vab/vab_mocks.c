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
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);

    /* Keep mscp base zero to allow checking base address in UTs */
    atu_map_entry->mscp_start_address = 0x0;

    return mock_type(int);
}

void __wrap_tower_configure_vab_apu(uint64_t vab_base_addr, uintptr_t tower_base_addr)
{
    check_expected(vab_base_addr);
    check_expected(tower_base_addr);

    return;
}

void __wrap_configure_vab_system_addr_map(uint64_t vab_base_addr, uint64_t tower_base_addr)
{
    check_expected(vab_base_addr);
    check_expected(tower_base_addr);

    return;
}

void __wrap_deassert_pcr_reset(uintptr_t vab_pcr_base_addr)
{
    check_expected(vab_pcr_base_addr);
    return;
}

int __wrap_vab_pcr_init(uintptr_t vab_pcr_base_addr)
{
    check_expected(vab_pcr_base_addr);
    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);
    FPFW_UNUSED(atu_map_entry);

    return mock_type(int);
}
