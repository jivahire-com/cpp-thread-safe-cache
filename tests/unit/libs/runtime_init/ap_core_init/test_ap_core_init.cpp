//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ap_core_init.cpp
 * Tests the init of the power service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h>      // for DFWK_SCHEDULE
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <FpFwUtils.h>       // for FPFW_UNUSED
#include <ap_core_init.h>    // for ap_core_service_config_t
#include <atu_lib.h>         // for atu_map_entry_t, atu_entry_attr_t
#include <boot_status.h>
#include <corebits.h> // for corebits_is_bit_set, corebits_is_clear
#include <fpfw_icc_base.h>
#include <fpfw_init.h>         // for fpfw_init_result_t, fpfw_init_component_t
#include <idsw.h>              // for idsw_get_die_id
#include <idsw_kng.h>          // for KNG_DIE_ID, KNG_PLAT_ID
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE
#include <startup_shutdown.h>
#include <stdint.h>
#include <string.h> // for memcpy

/*-- Symbolic Constant Macros (defines) --*/
#define DEFAULT_BOOT_CORE_RVBARADDR (0x00010000ull)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_ap_core_svc;
extern fpfw_init_component_t _fpfw_component_ap_core_int;

ap_core_service_config_t s_saved_config = {};

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_ap_core_init(pap_core_service_t p_device,
                         PDFWK_SCHEDULE p_schedule,
                         fpfw_icc_base_ctx_t* icc_base_ctx,
                         const ap_core_service_config_t* p_config)
{
    assert_non_null(p_device);
    assert_non_null(p_config);
    assert_non_null(p_config->platform_cores_in_die);
    assert_non_null(icc_base_ctx);
    check_expected_ptr(p_schedule);

    // save config for some tests
    memcpy(&s_saved_config, p_config, sizeof(ap_core_service_config_t));
}

void __wrap_ap_core_interface_init(pap_core_service_t p_device, pap_core_interface_t p_interface)
{
    check_expected_ptr(p_device);
    assert_non_null(p_interface);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

int __wrap_atu_translate_address(atu_id_t atu_id, uint64_t ap_addr, uint32_t* mscp_addr)
{
    check_expected(atu_id);
    check_expected(ap_addr);
    assert_non_null(mscp_addr);
    // return a value
    *mscp_addr = mock_type(uint32_t);
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

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
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
}
//
// Tests
//

TEST_FUNCTION(ap_core_init_ap_core_svc, nullptr, nullptr)
{
    //! Set up expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_fpfw_init_get_handle, (void*)0x2cfed84); // icc_hspmbx
    will_return(__wrap_fpfw_init_get_handle, &test_host);       // dfwk
    expect_value(__wrap_ap_core_init, p_schedule, &(test_host.Schedule));

    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_ap_core_svc.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);

    assert_int_equal(s_saved_config.platform_die_core_count, NUM_AP_CORES_PER_DIE);
    assert_true(s_saved_config.primary_boot_die);
}

TEST_FUNCTION(ap_core_init_ap_core_svc__svp, nullptr, nullptr)
{
    //! Set up expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_fpfw_init_get_handle, (void*)0x2cfed84); // icc_hspmbx
    will_return(__wrap_fpfw_init_get_handle, &test_host);       // dfwk
    expect_value(__wrap_ap_core_init, p_schedule, &(test_host.Schedule));

    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_ap_core_svc.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);

    assert_int_equal(s_saved_config.platform_die_core_count, NUM_AP_CORES_PER_DIE);
    assert_true(s_saved_config.primary_boot_die);
}

TEST_FUNCTION(ap_core_init_ap_core_svc__bigfpga, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};
    uint32_t rvbar_value = 0;

    will_return(__wrap_fpfw_init_get_handle, (void*)0x2cfed84); // icc_hspmbx
    will_return(__wrap_fpfw_init_get_handle, &test_host);       // dfwk
    expect_value(__wrap_ap_core_init, p_schedule, &(test_host.Schedule));

    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE_RVP);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    // on FPGA we also expect a call that will write forever loop; it uses atu_translate_address call
    expect_value(__wrap_atu_translate_address, atu_id, ATU_ID_MSCP);
    expect_value(__wrap_atu_translate_address, ap_addr, DEFAULT_BOOT_CORE_RVBARADDR);
    // return for mscp_addr - only works if our data address is in lower 32GB
    will_return(__wrap_atu_translate_address, (uint32_t)&rvbar_value);
    will_return(__wrap_atu_translate_address, 0);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_ap_core_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
    assert_int_not_equal(rvbar_value, 0);

    // bigfpga has no core 0
    assert_false(corebits_is_bit_set(s_saved_config.platform_cores_in_die, 0));
    assert_int_equal(s_saved_config.platform_die_core_count, NUM_AP_CORES_PER_DIE);
    assert_true(s_saved_config.primary_boot_die);
}

TEST_FUNCTION(ap_core_init_ap_core_int, nullptr, nullptr)
{
    // Set up expectations
    ap_core_service_t ap_core_device = {};
    DFWK_INTERFACE_HEADER ssi_interface = {};

    will_return(__wrap_fpfw_init_get_handle, &ap_core_device);
    expect_value(__wrap_ap_core_interface_init, p_device, &ap_core_device);
    // interface init is called twice
    will_return(__wrap_fpfw_init_get_handle, &ap_core_device);
    expect_value(__wrap_ap_core_interface_init, p_device, &ap_core_device);

    will_return(__wrap_fpfw_init_get_handle, &ssi_interface);
    expect_value(__wrap_sos_register_ssi, p_interface, &ssi_interface);
    // p_registration and p_ssi_interface come from static allocations in the function
    expect_any(__wrap_sos_register_ssi, p_registration);
    expect_any(__wrap_sos_register_ssi, p_ssi_interface);
    will_return(__wrap_sos_register_ssi, FPFW_INIT_STATUS_SUCCESS);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_APCORE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_ap_core_int.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}
