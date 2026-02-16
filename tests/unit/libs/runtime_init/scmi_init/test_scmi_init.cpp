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
#include <boot_status.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <idsw.h>      // for idsw_get_die_id
#include <idsw_kng.h>  // for KNG_DIE_ID, KNG_PLAT_ID
#include <scmi_init.h> // for scmi_drv_init, scmi_set_apcore_interface

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

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
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

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_POWER_SCMI_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_scmi_drv.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
