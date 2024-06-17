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
#include <ioss_init.h> // for ioss_init
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/*------------- Functions ----------------*/
//
// Tests
//
TEST_FUNCTION(test_invalid_die_num, NULL, NULL)
{
    int die_num = 2;
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        ioss_init(die_num);
    }
}

TEST_FUNCTION(test_valid_die_0, NULL, NULL)
{
    int die_num = 0;
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_program_ioss_pcr_usb_reset, pcr_base_addr, IOSS_TOP_IOSS_PCR_ADDRESS);
    will_return(__wrap_program_ioss_pcr_clock_mux, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    ioss_init(die_num);
}

TEST_FUNCTION(test_valid_die_1, NULL, NULL)
{
    int die_num = 1;
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_program_ioss_pcr_usb_reset, pcr_base_addr, IOSS_TOP_IOSS_PCR_ADDRESS);
    will_return(__wrap_program_ioss_pcr_clock_mux, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    ioss_init(die_num);
}

TEST_FUNCTION(test_atu_map_fail, NULL, NULL)
{
    int die_num = 0;
    will_return(__wrap_atu_map, SILIBS_E_INIT);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        ioss_init(die_num);
    }
}

TEST_FUNCTION(test_program_ioss_pcr_clock_fail, NULL, NULL)
{
    int die_num = 0;
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_program_ioss_pcr_usb_reset, pcr_base_addr, IOSS_TOP_IOSS_PCR_ADDRESS);
    will_return(__wrap_program_ioss_pcr_clock_mux, SILIBS_E_PARAM);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        ioss_init(die_num);
    }
}

TEST_FUNCTION(test_atu_unmap_fail, NULL, NULL)
{
    int die_num = 0;
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_program_ioss_pcr_usb_reset, pcr_base_addr, IOSS_TOP_IOSS_PCR_ADDRESS);
    will_return(__wrap_program_ioss_pcr_clock_mux, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_E_INIT);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        ioss_init(die_num);
    }
}