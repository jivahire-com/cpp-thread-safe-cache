//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_health_monitor_init.cpp
 * Tests the init of the health monitor
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <accelip_id.h> // for ACCEL_ID_SDM, ACCEL_ID_CDED
#include <fpfw_init.h>  // for fpfw_init_result_t, fpfw_init_component_t
#include <fpfw_pldm_service.h>
#include <health_monitor.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_hm_svc;
extern fpfw_init_component_t _fpfw_component_hm_cli_init;
extern fpfw_init_component_t _fpfw_component_hm_pldm;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_hm_pre_ddr_init(hm_config_t* hm_config)
{
    assert_non_null(hm_config->mscp_ghes_base);
    assert_non_null(hm_config->mscp_ghes_error_record_addr_table_base);
    assert_non_null(hm_config->mscp_ghes_ack_addr_table_base);
    assert_non_null(hm_config->mscp_ghes_error_record_addr_base);
    assert_null(hm_config->mscp_error_injection_addr_base);
    function_called();
}

void __wrap_hm_post_ddr_init()
{
    function_called();
}

void __wrap_hm_post_intercore_init(hm_intercore_type_t intercore_type, fpfw_icc_base_ctx_t* icc_ctx)
{
    check_expected(intercore_type);
    assert_non_null(icc_ctx);
    function_called();
}

void __wrap_hm_cli_init()
{
    function_called();
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

PlatformEventReadyNotificationCb __wrap_pldm_platform_event_ready_callback = NULL;

fpfw_status_t __wrap_fpfw_pldm_service_register_platform_event_ready_notification(pldm_platform_event_ready_notification* p_notification)
{
    assert_non_null(p_notification);
    assert_non_null(p_notification->CallBack);

    __wrap_pldm_platform_event_ready_callback = p_notification->CallBack;

    function_called();

    return FPFW_STATUS_SUCCESS;
}

void __wrap_hm_set_pldm_ready_status()
{
    function_called();
}
}

//
// Tests
//
TEST_FUNCTION(hm_svc, nullptr, nullptr)
{
    will_return_always(__wrap_fpfw_init_get_handle, (void*)1234);
    will_return_always(__wrap_idsw_get_die_id, (KNG_DIE_ID)1);
    expect_function_call(__wrap_hm_pre_ddr_init);
    expect_function_call(__wrap_hm_post_ddr_init);
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_value(__wrap_hm_post_intercore_init, intercore_type, HM_INTERCORE_REMOTE);
    expect_function_call(__wrap_hm_post_intercore_init);
    expect_value(__wrap_hm_post_intercore_init, intercore_type, HM_INTERCORE_LOCAL);
    expect_function_call(__wrap_hm_post_intercore_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(hm_cli_init, nullptr, nullptr)
{
    expect_function_call_any(__wrap_hm_cli_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_cli_init.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_pldm, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);

    expect_function_call(__wrap_fpfw_pldm_service_register_platform_event_ready_notification);
    _fpfw_component_hm_pldm.init_fn();

    expect_function_call(__wrap_hm_set_pldm_ready_status);
    __wrap_pldm_platform_event_ready_callback(0, NULL);
}
