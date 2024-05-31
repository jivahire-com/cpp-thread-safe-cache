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
#include <atu_lib.h>         // for atu_map_entry_t, atu_entry_attr_t
#include <fpfw_init.h>       // for fpfw_init_result_t, fpfw_init_component_t
#include <idsw.h>            // for idsw_get_die_id
#include <power_dfwk.h>      // for ppower_service_t, etc
#include <power_runconfig.h> // for power_service_config_t
#include <startup_shutdown.h>
#include <stdint.h>

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

void __wrap_power_init(ppower_service_t p_device, PDFWK_SCHEDULE p_schedule, const power_service_config_t* p_config)
{
    assert_non_null(p_device);
    assert_non_null(p_config);
    check_expected_ptr(p_schedule);
}

void __wrap_power_interface_init(ppower_service_t p_device, ppower_service_interface_t p_interface)
{
    check_expected_ptr(p_device);
    assert_non_null(p_interface);
}

// wrap idsw_get_die_id
DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(DIE_ID);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);
    check_expected_ptr(atu_map_entry);
    return mock_type(int);
}

int32_t __wrap_sos_register_ssi(PDFWK_INTERFACE_HEADER p_interface,
                                pstartup_ssi_registration_t p_registration,
                                PDFWK_INTERFACE_HEADER p_ssi_interface)
{
    check_expected_ptr(p_interface);
    check_expected_ptr(p_registration);
    check_expected_ptr(p_ssi_interface);
    return mock_type(int32_t);
}
}
//
// Tests
//

TEST_FUNCTION(power_init_pwr_svc, nullptr, nullptr)
{
    //! Set up expectations
    DFWK_THREADX_HOST test_host = {};

    // only doing basic tests on die_id and atu_map as the current implementation is temporary
    will_return(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    expect_any(__wrap_atu_map, atu_map_entry);
    will_return(__wrap_atu_map, 0);

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
    DFWK_INTERFACE_HEADER ssi_interface = {};

    will_return(__wrap_fpfw_init_get_handle, &power_device);
    expect_value(__wrap_power_interface_init, p_device, &power_device);
    // interface init is called twice
    will_return(__wrap_fpfw_init_get_handle, &power_device);
    expect_value(__wrap_power_interface_init, p_device, &power_device);

    will_return(__wrap_fpfw_init_get_handle, &ssi_interface);
    expect_value(__wrap_sos_register_ssi, p_interface, &ssi_interface);
    // p_registration and p_ssi_interface come from static allocations in the function
    expect_any(__wrap_sos_register_ssi, p_registration);
    expect_any(__wrap_sos_register_ssi, p_ssi_interface);
    will_return(__wrap_sos_register_ssi, FPFW_INIT_STATUS_SUCCESS);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_pwr_int.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}
