//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_sds_init.cpp
 * Tests the init of the sds service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <FpFwUtils.h>
#include <fpfw_init.h>
#include <sds_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_sds_svc;
extern fpfw_init_component_t _fpfw_component_sds_int;

/*------------- Functions ----------------*/

//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_sds_init(psds_service_t p_device, PDFWK_SCHEDULE p_schedule)
{
    assert_non_null(p_device);
    check_expected_ptr(p_schedule);
}

void __wrap_sds_interface_init(psds_service_t p_device, psds_service_interface_t p_interface)
{
    check_expected_ptr(p_device);
    assert_non_null(p_interface);
}
}
//
// Tests
//

TEST_FUNCTION(test_sds_svc_init, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_value(__wrap_sds_init, p_schedule, &(test_host.Schedule));

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sds_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_sds_interface_init, nullptr, nullptr)
{
    // Set up expectations
    sds_service_t sds_device = {};
    will_return(__wrap_fpfw_init_get_handle, &sds_device);
    expect_value(__wrap_sds_interface_init, p_device, &sds_device);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sds_int.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}
