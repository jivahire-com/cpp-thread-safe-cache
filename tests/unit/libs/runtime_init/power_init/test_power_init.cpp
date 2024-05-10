//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_power_init.cpp
 * Tests the init of the power service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h>      // for DFWK_SCHEDULE
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <FpFwUtils.h>       // for FPFW_UNUSED
#include <fpfw_init.h>       // for fpfw_init_result_t, fpfw_init_component_t
#include <power_dfwk.h>      // for ppower_service_t, etc

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pwr_svc;
extern fpfw_init_component_t _fpfw_component_pwr_int;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_power_init(ppower_service_t p_device, PDFWK_SCHEDULE p_schedule)
{
    assert_non_null(p_device);
    check_expected_ptr(p_schedule);
}

void __wrap_power_interface_init(ppower_service_t p_device, ppower_service_interface_t p_interface)
{
    check_expected_ptr(p_device);
    assert_non_null(p_interface);
}

//
// Tests
//

TEST_FUNCTION(power_init_pwr_svc, nullptr, nullptr)
{
    //! Set up expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_fpfw_init_get_handle, &test_host); //! driver fmwk host handle
    expect_value(__wrap_power_init, p_schedule, &(test_host.Schedule));

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_pwr_svc.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(power_init_pwr_int, nullptr, nullptr)
{
    // Set up expectations
    power_service_t power_device = {};
    will_return(__wrap_fpfw_init_get_handle, &power_device); //! uart device handle
    expect_value(__wrap_power_interface_init, p_device, &power_device);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_pwr_int.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}
}
