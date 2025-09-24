//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ift_init.cpp
 * ift init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h>
#include <idsw_kng.h> // for KNG_CPU_TYPE
#include <ift_fw.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_ift;
extern fpfw_init_component_t _fpfw_component_ift_drv;

/*------------- Functions ----------------*/
//
// Mocks
//

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_ift_dfwk_set_interface(pift_interface_t p_interface)
{
    assert_non_null(p_interface);
}

void __wrap_ift_dfwk_device_initialize(pift_device_t device, PDFWK_SCHEDULE schedule)
{
    assert_non_null(device);
    assert_non_null(schedule);
}

void __wrap_ift_dfwk_interface_initialize(pift_interface_t intf, pift_device_t device)
{
    assert_non_null(intf);
    assert_non_null(device);
}

void __wrap_ift_setup_tests(void)
{
    function_called();
}

void __wrap_ift_init(fpfw_icc_base_ctx_t* hsp_icc_ctx)
{
    assert_non_null(hsp_icc_ctx);
}

bool __wrap_ift_is_enabled(void)
{
    return mock_type(bool);
}

//
// Tests
//

TEST_FUNCTION(test_ift_init, nullptr, nullptr)
{
    // Set Expectations
    will_return(__wrap_fpfw_init_get_handle, 0x12345678);

    // Call API under test
    fpfw_init_result_t result = _fpfw_component_ift.init_fn();

    // Validate Results
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_ift_drv_init, nullptr, nullptr)
{
    // Set Expectations
    will_return(__wrap_ift_is_enabled, true);
    will_return(__wrap_fpfw_init_get_handle, 0x12345678);
    expect_function_call(__wrap_ift_setup_tests);

    // Call API under test
    fpfw_init_result_t result = _fpfw_component_ift_drv.init_fn();

    // Validate Results
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_ift_drv_init_ift_disabled, nullptr, nullptr)
{
    // Set Expectations
    will_return(__wrap_ift_is_enabled, false);

    // Call API under test
    fpfw_init_result_t result = _fpfw_component_ift_drv.init_fn();

    // Validate Results
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
}
