//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_arsm_init.cpp
 * ARSM init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h>
#include <atu_api.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

void* __real_memset(void* __a, int __b, size_t __c);

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_arsm;
static bool s_test_enabled = false;

/*------------- Functions ----------------*/

//
// Mocks
//

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

bool __wrap_system_info_is_warm_start(void)
{
    return mock_type(bool);
}

void* __wrap_memset(void* __a, int __b, size_t __c)
{
    if (s_test_enabled)
    {
        check_expected(__a);
        check_expected(__b);
        check_expected(__c);
        return NULL;
    }

    return __real_memset(__a, __b, __c);
}

static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);

    s_test_enabled = true;

    return 0;
}

static int test_teardown(void** ctx)
{
    FPFW_UNUSED(ctx);

    s_test_enabled = false;

    return 0;
}
}

//
// Tests
//
TEST_FUNCTION(test_arsm_init_warm_boot, test_setup, test_teardown)
{
    // Set up expectations
    will_return(__wrap_system_info_is_warm_start, true);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_arsm.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_arsm_init_cold_boot_d0, test_setup, test_teardown)
{
    // Set up expectations
    will_return(__wrap_system_info_is_warm_start, false);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_memset, __a, (void*)MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR);
    expect_value(__wrap_memset, __b, 0);
    expect_value(__wrap_memset, __c, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_SIZE);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_arsm.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_arsm_init_cold_boot_d1, test_setup, test_teardown)
{
    // Set up expectations
    will_return(__wrap_system_info_is_warm_start, false);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    expect_value(__wrap_memset, __a, (void*)MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR);
    expect_value(__wrap_memset, __b, 0);
    expect_value(__wrap_memset, __c, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_SIZE);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_arsm.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}