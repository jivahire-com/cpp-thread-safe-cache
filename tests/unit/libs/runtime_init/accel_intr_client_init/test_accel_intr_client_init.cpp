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
#include <DfwkDriver.h>              // for DFWK_SCHEDULE
#include <DfwkThreadXHost.h>         // for DFWK_THREADX_HOST
#include <FpFwUtils.h>               // for FPFW_UNUSED
#include <accel_intr_service_dfwk.h> // for paccel_intr_service_t, etc
#include <fpfw_init.h>               // for fpfw_init_result_t, fpfw_init_component_t
#include <stdint.h>
#include <string.h> // for memcpy

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_accel_intr_clnt;

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
 * @brief Mock function for Accel Interrupt Client init
 *
 * @param[in] p_interface : This value is retrieved with a fpfw_init_get_handle call and hence checking for ptr
 */
void __wrap_accel_intr_client_init(paccel_intr_service_interface_t p_interface)
{
    check_expected_ptr(p_interface);
}
}

/****************************
 * TESTS
 ****************************/

/**
 * @brief Test accel_intr_c init
 */
TEST_FUNCTION(test_accel_intr_client_init, nullptr, nullptr)
{
    //! Set up expectations
    accel_intr_service_interface_t accel_interface = {};

    will_return(__wrap_fpfw_init_get_handle, &accel_interface); //! driver fmwk host handle
    expect_value(__wrap_accel_intr_client_init, p_interface, &accel_interface);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_accel_intr_clnt.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
