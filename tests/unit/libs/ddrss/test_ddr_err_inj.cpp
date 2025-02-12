//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cli_ddr.cpp
 * Tests the init of cli ddr component
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_non_null, CmockaWrapperTest, TEST_...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <ddr_err_inj.h>
#include <ddrmctop_regs.h>
#include <ddrss_lib.h>
#include <kng_soc_constants.h>
#include <silibs_ap_top_regs.h>
#include <silibs_platform.h>
#include <stddef.h> // for NULL, size_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

//
// Mocks
//
int __wrap_ddrss_inject_media_data_err(uint32_t mc, const ddrss_media_data_err_inj_info_t* media_err_inj)
{
    check_expected(mc);
    check_expected(media_err_inj->err_rs_sym);
    check_expected(media_err_inj->err_inj_rw);
    check_expected(media_err_inj->err_inj_beat);
    check_expected(media_err_inj->err_inj_cnt);
    return 0;
}

/*------------- Functions ----------------*/

//
// Tests
//

TEST_FUNCTION(test_ecc_ce_error_injection, NULL, NULL)
{
    // Set up pre-conditions / expectations
    const uint32_t p_addr = 0x0;
    const uint16_t Bit_Value = BIT1;
    const uint32_t expected_mc = 0;
    const int32_t expected_die_num = 0;
    ddrss_media_data_err_inj_info_t expected_err_inj_data = {};
    expected_err_inj_data.err_rs_sym = 2; // Bit_Value; // some_expected_value
    expected_err_inj_data.err_inj_rw = 1;
    expected_err_inj_data.err_inj_beat = 1;
    expected_err_inj_data.err_inj_cnt = 1;
    expect_value(__wrap_ddrss_inject_media_data_err, mc, expected_mc);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_rs_sym, expected_err_inj_data.err_rs_sym);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_rw, expected_err_inj_data.err_inj_rw);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_beat, expected_err_inj_data.err_inj_beat);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_cnt, expected_err_inj_data.err_inj_cnt);
    will_return(__wrap_atu_map, 0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, 0);

    // Call the function under test
    ddrss_ue_ce_error_injection(expected_die_num, expected_mc, p_addr, Bit_Value);
}

TEST_FUNCTION(test_ecc_ue_error_injection, NULL, NULL)
{
    // Set up pre-conditions / expectations
    const uint32_t p_addr = 0x0;
    const uint16_t Bit_Value = BIT1;
    const uint32_t expected_mc = 0;
    const int32_t expected_die_num = 0;
    ddrss_media_data_err_inj_info_t expected_err_inj_data = {};
    expected_err_inj_data.err_rs_sym = 2; // Bit_Value; // some_expected_value
    expected_err_inj_data.err_inj_rw = 1;
    expected_err_inj_data.err_inj_beat = 1;
    expected_err_inj_data.err_inj_cnt = 1;
    expect_value(__wrap_ddrss_inject_media_data_err, mc, expected_mc);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_rs_sym, expected_err_inj_data.err_rs_sym);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_rw, expected_err_inj_data.err_inj_rw);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_beat, expected_err_inj_data.err_inj_beat);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_cnt, expected_err_inj_data.err_inj_cnt);
    will_return(__wrap_atu_map, 0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, 0);

    // Call the function under test
    ddrss_ue_ce_error_injection(expected_die_num, expected_mc, p_addr, Bit_Value);
}
}