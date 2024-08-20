//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mcp_mpu_init.cpp
 * MCP MPU Init Test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include "cmsis_m7.h" // for ARM_MPU_AP_PRIV, ARM_MPU_RASR, ARM_...

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT...
#include <mpu.h>       // for DISABLE_SUBREGION, NON_CACHEABLE
#include <stddef.h>    // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_mpu;

/*------------- Functions ----------------*/
//
// Mocks
//
void __real_ARM_MPU_Load(ARM_MPU_Region_t const* table, uint32_t cnt);
void __wrap_ARM_MPU_Load(ARM_MPU_Region_t const* table, uint32_t cnt)
{
    check_expected_ptr(table);
    check_expected(cnt);

    __real_ARM_MPU_Load(table, cnt);
}

//
// Tests
//
TEST_FUNCTION(test_mcp_mpu_init, nullptr, nullptr)
{
    // Set expectations to mock the following calls if mpu_init is successful.
    expect_function_call(ARM_MPU_Disable);
    expect_function_call(ARM_MPU_Load);
    expect_function_call(ARM_MPU_Enable);

    // Set expectations to have valid region table and its count.
    expect_not_value(__wrap_ARM_MPU_Load, table, NULL);
    expect_not_value(__wrap_ARM_MPU_Load, cnt, 0);

    // Call API under test
    fpfw_init_result_t result = _fpfw_component_mpu.init_fn();

    // Check expected return value
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
}
}
