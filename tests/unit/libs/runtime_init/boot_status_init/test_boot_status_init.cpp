//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_boot_status_init.cpp
 * boot status init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_icc_base.h>
#include <fpfw_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_boot_stat;
/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_boot_status_init(fpfw_icc_base_ctx_t* icc_base_ctx)
{
    check_expected_ptr(icc_base_ctx);
    function_called();
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    function_called();
    return mock_type(void*);
}
//
// Tests
//
TEST_FUNCTION(test_boot_status_init, nullptr, nullptr)
{
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* dummy_icc_hspmbx_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    // Set up expectations
    will_return(__wrap_fpfw_init_get_handle, dummy_icc_hspmbx_ctx);
    expect_function_call(__wrap_fpfw_init_get_handle);
    expect_value(__wrap_boot_status_init, icc_base_ctx, dummy_icc_hspmbx_ctx);
    expect_function_call(__wrap_boot_status_init);

    // Call API under test
    _fpfw_component_boot_stat.init_fn();
}
}
