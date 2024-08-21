//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file fpfw_mock.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include "fpfw_status.h"      // for fpfw_status_t
#include "fpfw_timer_types.h" // for _fpfw_timer_variant_t, fpfw_timer_call...

#include <FpFwUtils.h>  // for fpfw_dur_t, FPFW_UNUSED
#include <fpfw_timer.h> // for fpfw_timer_t
#include <stdint.h>     // for UINT64_MAX

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static fpfw_timer_callback s_cur_cb = NULL;

/*------------- Functions ----------------*/

fpfw_timer_callback fpfw_mock_get_timer_cb(void)
{
    return s_cur_cb;
}

/*****************************************
 * MOCKS
 *****************************************/

/**
 * @brief fpfw_timer_create - Mock function
 * Also caches fpfw_timer_callback, for other testcases
 */
fpfw_status_t __wrap_fpfw_timer_create(fpfw_timer_t* timer,
                                       fpfw_timer_variant_t variant,
                                       fpfw_dur_t period,
                                       fpfw_timer_callback cb,
                                       void* ctx)
{
    FPFW_UNUSED(ctx);
    check_expected_ptr(timer);
    check_expected_ptr(cb);
    assert_in_range(variant, FPFW_TIMER_ONESHOT, FPFW_TIMER_PERIODIC);
    assert_in_range(period, 0, UINT64_MAX);
    s_cur_cb = cb;
    return mock_type(fpfw_status_t);
}

/**
 * @brief fpfw_timer_enable - Mock function
 */
fpfw_status_t __wrap_fpfw_timer_enable(fpfw_timer_t* timer, fpfw_dur_t delay)
{
    check_expected_ptr(timer);
    assert_in_range(delay, 1, UINT64_MAX);
    return mock_type(fpfw_status_t);
}

/**
 * @brief fpfw_timer_reset - Mock function
 */
fpfw_status_t __wrap_fpfw_timer_reset(fpfw_timer_t* timer)
{
    check_expected_ptr(timer);
    return mock_type(fpfw_status_t);
}

} // extern "C"