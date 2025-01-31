//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ioss_unit_tests.cpp
 * IOSS tests
 */

/*------------- Includes -----------------*/
#include "silibs_common.h"
#include "silibs_platform.h"

#include <CMockaWrapper.h>
#include <cstdint>
#include <ioss_top_regs.h>          // for IOSS_TOP_IOSS_PCR_ADDRESS, IOSS_...
#include <kng_soc_constants.h>      // for ATU_PAGE_SIZE, NUM_IOSS_INSTANCES
#include <silibs_ap_top_regs.h>     // for AP_TOP_D0_VAB_CDED_IOSS_ADDRESS
#include <stddef.h>                 // for NULL
#include <vab_cded_ioss_top_regs.h> // for VAB_CDED_IOSS_TOP_IOSS_ADDRESS

extern "C" {
#include <error_handler.h>
#include <ioss_ini.h> // for ioss_ini
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/*------------- Functions ----------------*/
//
// Tests
//
TEST_FUNCTION(test_valid_die_0, NULL, NULL)
{
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_function_call(__wrap_ioss_init);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    ioss_ini();
}

TEST_FUNCTION(test_atu_map_fail, NULL, NULL)
{
    will_return(__wrap_atu_map, SILIBS_E_INIT);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        ioss_ini();
    }
}

TEST_FUNCTION(test_atu_unmap_fail, NULL, NULL)
{
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_function_call(__wrap_ioss_init);
    will_return(__wrap_atu_unmap, SILIBS_E_INIT);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        ioss_ini();
    }
}