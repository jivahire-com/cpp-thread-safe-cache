//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_scmi_init.cpp
 * Tests the init of the scmi driver
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h> // for DFWK_SCHEDULE
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <fpfw_init.h>  // for fpfw_init_result_t, fpfw_init_component_t
#include <scmi_init.h>  // for scmi_drv_init, scmi_set_apcore_interface

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_scmi_drv;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_scmi_set_apcore_interface(DFWK_INTERFACE_HEADER* p_interface)
{
    check_expected_ptr(p_interface);
}

void __wrap_scmi_drv_init(DFWK_INTERFACE_HEADER* p_scp_tfa_interface)
{
    check_expected_ptr(p_scp_tfa_interface);
}
}
//
// Tests
//

TEST_FUNCTION(scmi_init, nullptr, nullptr)
{
#define TEST_INTERFACE 0x23456780
    //! Set up expectations

    will_return_count(__wrap_fpfw_init_get_handle, (void*)TEST_INTERFACE, 2); //! driver fmwk host handle
    expect_value(__wrap_scmi_set_apcore_interface, p_interface, TEST_INTERFACE);
    expect_value(__wrap_scmi_drv_init, p_scp_tfa_interface, TEST_INTERFACE);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_scmi_drv.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
