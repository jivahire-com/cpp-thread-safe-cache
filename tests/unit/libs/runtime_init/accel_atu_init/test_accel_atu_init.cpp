//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_accel_atu_init.cpp
 * Accel ATU init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h>
#include <accelip_id.h>
#include <atu_init.h>
#include <atu_lib.h>
#include <fpfw_init.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_accel_atu;

/*------------- Functions ----------------*/
//
// Mocks
//

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    FPFW_UNUSED(atu_id);

    atu_map_entry->mscp_start_address = 0xDEADEED;

    return mock_type(int);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    function_called();
}
}

//
// Tests
//

TEST_FUNCTION(test_accel_atu_die0, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, 0);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_accel_atu.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_accel_atu_die1, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, 1);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_accel_atu.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_accel_atu_fail1, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, 2);
    expect_value(__wrap_FpFwAssert, expression, false);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_accel_atu.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_accel_atu_fail2, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, 1);
    will_return_always(__wrap_atu_map, SILIBS_E_PARAM);
    expect_function_call_any(__wrap_crash_dump_bug_check);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_accel_atu.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_atu_svc_accel_atu_addr_die0, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, 0);

    // Perform necessary assertions on result
    assert_non_null(atu_svc_accel_atu_addr(ACCEL_ID_SDM));
}

TEST_FUNCTION(test_atu_svc_accel_atu_addr_die1, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, 1);

    // Perform necessary assertions on result
    assert_non_null(atu_svc_accel_atu_addr(ACCEL_ID_CDED));
}

TEST_FUNCTION(test_atu_svc_accel_atu_addr_fail1, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, 0);
    expect_value(__wrap_FpFwAssert, expression, false);

    // Perform necessary assertions on result
    assert_null(atu_svc_accel_atu_addr(NUM_VALID_ACCEL_ID));
}

TEST_FUNCTION(test_atu_svc_accel_atu_addr_fail2, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, 2);
    expect_value(__wrap_FpFwAssert, expression, false);

    // Perform necessary assertions on result
    assert_null(atu_svc_accel_atu_addr(ACCEL_ID_SDM));
}
