//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_accel_intr_client_init.cpp
 * Tests the init of the Accel Interrupt Client
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h>              // for PDFWK_SCHEDULE
#include <DfwkThreadXHost.h>         // for DFWK_THREADX_HOST
#include <FpFwUtils.h>               // for FPFW_UNUSED
#include <accel_intr_service_dfwk.h> // for paccel_intr_service_t, accel_in...
#include <fpfw_init.h>               // for FPFW_INIT_STATUS_SUCCESS, fpfw_...

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_accel_intr_svc;
extern fpfw_init_component_t _fpfw_component_accel_intr_inf;

/*------------- Functions ----------------*/

/****************************
 * MOCKS
 ****************************/

/**
 * @brief Mock function for fpfw_init_get_handle
 *
 * @param[in] id : Component ID, this is unused in testcase
 */
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

/**
 * @brief Mock function for Accel Interrupt Service init
 *
 * @param[in] p_device : This value is passed to the function, so null check
 * @param[in] p_schedule : This value is retrieved with a fpfw_init_get_handle call and hence checking for ptr
 *
 */
void __wrap_accel_intr_service_init(paccel_intr_service_t p_device, PDFWK_SCHEDULE p_schedule)
{
    assert_non_null(p_device);
    check_expected_ptr(p_schedule);
}

/**
 * @brief Mock function for Accel Interrupt Service Interface init
 *
 * @param[in] p_device : This value is retrieved with a fpfw_init_get_handle call and hence checking for ptr
 * @param[in] p_interface : This value is passed to the function, so null check
 *
 */
void __wrap_accel_intr_service_interface_init(paccel_intr_service_t p_device, paccel_intr_service_interface_t p_interface)
{
    check_expected_ptr(p_device);
    assert_non_null(p_interface);
}
}

/****************************
 * TESTS
 ****************************/

/**
 * @brief Test accel_intr_svc init
 */
TEST_FUNCTION(test_accel_intr_svc_init, nullptr, nullptr)
{
    //! Set up expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_fpfw_init_get_handle, &test_host); //! driver fmwk host handle
    expect_value(__wrap_accel_intr_service_init, p_schedule, &(test_host.Schedule));

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_accel_intr_svc.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

/**
 * @brief Test accel_intr_int init
 */
TEST_FUNCTION(test_accel_intr_int_init, nullptr, nullptr)
{
    //! Set up expectations
    accel_intr_service_t accel_intr_device = {};

    will_return(__wrap_fpfw_init_get_handle, &accel_intr_device); //! driver fmwk host handle
    expect_value(__wrap_accel_intr_service_interface_init, p_device, &(accel_intr_device));

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_accel_intr_inf.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}
