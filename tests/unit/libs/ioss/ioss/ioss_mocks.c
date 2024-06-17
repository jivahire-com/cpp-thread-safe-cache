//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ioss_mocks.c
 * IOSS mock functions
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h>       // IWYU pragma: keep
#include <FpFwUtils.h>        // for FPFW_UNUSED
#include <atu_lib.h>          // for atu_id_t, atu_map_entry_t
#include <cmocka.h>           // for mock_type
#include <pcr_clock_config.h> // for PCR_CLOCK_SELECT
#include <pcr_ioss.h>         // for IOSS_GFMUX_FAM
#include <stdint.h>           // for uint64_t, uintptr_t, uint32_t

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

void __wrap_program_ioss_pcr_usb_reset(uintptr_t pcr_base_addr)
{
    check_expected(pcr_base_addr);

    return;
}

int __wrap_program_ioss_pcr_clock_mux(uintptr_t pcr_base_addr, IOSS_GFMUX_FAM gfmux_fam, PCR_CLOCK_SELECT clock_select, uint32_t mux_config_timeout)
{
    FPFW_UNUSED(pcr_base_addr);
    FPFW_UNUSED(gfmux_fam);
    FPFW_UNUSED(clock_select);
    FPFW_UNUSED(mux_config_timeout);

    return mock_type(int);
}
