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
#include <accelip_id.h> // for NUM_VALID_ACCEL_ID
#include <fpfw_timer.h> // for fpfw_timer_t
#include <stdint.h>     // for uint
#include <stdint.h>     // for UINT64_MAX

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static fpfw_timer_callback s_crashdump_timeout_cb[NUM_VALID_ACCEL_ID] = {nullptr};
static void* s_crashdump_timeout_ctx[NUM_VALID_ACCEL_ID] = {nullptr};
static ACCEL_ID s_active_accel_type = NUM_VALID_ACCEL_ID;

/*------------- Functions ----------------*/

fpfw_timer_callback fpfw_mock_get_timer_cb(ACCEL_ID accel_type)
{
    assert_in_range(accel_type, 0, NUM_VALID_ACCEL_ID - 1);
    return s_crashdump_timeout_cb[accel_type];
}

void* fpfw_mock_get_timer_ctx(ACCEL_ID accel_type)
{
    assert_in_range(accel_type, 0, NUM_VALID_ACCEL_ID - 1);
    return s_crashdump_timeout_ctx[accel_type];
}

void fpfw_mock_set_active_accel_type(ACCEL_ID accel_type)
{
    assert_in_range(accel_type, 0, NUM_VALID_ACCEL_ID - 1);
    s_active_accel_type = accel_type;
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
    check_expected_ptr(timer);
    check_expected_ptr(cb);
    assert_in_range(variant, FPFW_TIMER_ONESHOT, FPFW_TIMER_PERIODIC);
    assert_in_range(period, 0, UINT64_MAX);
    if (s_crashdump_timeout_cb[s_active_accel_type] == nullptr)
    {
        s_crashdump_timeout_cb[s_active_accel_type] = cb;
        s_crashdump_timeout_ctx[s_active_accel_type] = ctx;
    }

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

uint32_t __wrap_FPFwCoreInterruptEnableVector(uint32_t irqnum)
{
    check_expected(irqnum);
    return mock_type(uint32_t);
}

} // extern "C"