//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_gtimer.cpp
 * gtimer tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <gtimer.h>                          // for gtimer_init
#include <refclk_counter_cnt_control_regs.h> // for refclk_counter_cnt_cont...
#include <stdint.h>                          // for uint32_t, uintptr_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Tests
//
TEST_FUNCTION(test_gtimer_init, nullptr, nullptr)
{
    const uint32_t REFCLK_FREQUENCY_HZ = 250000000;
    const uint32_t REFCLK_SCALING_FACTOR = 4;

    // Set up expectations
    refclk_counter_cnt_control_reg test_cntr_ctrl_reg = {};

    // Call API under test
    gtimer_init((uintptr_t)&test_cntr_ctrl_reg, REFCLK_FREQUENCY_HZ, REFCLK_SCALING_FACTOR);

    assert_int_equal(test_cntr_ctrl_reg.cntcr.en, 1);
    assert_int_equal(test_cntr_ctrl_reg.cntcr.fcreq, 1);
    assert_int_equal(test_cntr_ctrl_reg.cntfid0.frequency, REFCLK_FREQUENCY_HZ);
    assert_int_equal(test_cntr_ctrl_reg.cntincr.incr_value, REFCLK_SCALING_FACTOR);
}
}