//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_main.cpp
 * Tests health monitor main functionality
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <accelip_id.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <hm_test.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
#define ICC_APCORE_TEST_POINT 0x1000
#define ICC_MSCP_TEST_POINT   0x2000
#define ICC_HSP_TEST_POINT    0x3000
#define ICC_SDM_TEST_POINT    0x4000
#define ICC_CDED_TEST_POINT   0x5000

/*-------- Function Prototypes -----------*/
extern int pre_ddr_setup(void** state);
extern hm_config_t hm_config_test;

extern acpi_ghes_t ghes_local[];
extern acpi_ghes_error_record_dual_die_t ghes_error_record_local[];

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
}

//
// Tests
//
TEST_FUNCTION(test_hm_pre_ddr_init, pre_ddr_setup, nullptr)
{
}

TEST_FUNCTION(test_get_hm_config, pre_ddr_setup, nullptr)
{
    assert_true(get_hm_config() == &hm_config_test);
}