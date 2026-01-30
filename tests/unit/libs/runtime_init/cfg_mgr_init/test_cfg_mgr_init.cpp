//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cfg_mgr_init.cpp
 * Config Manger Init test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
// #include "debug.h" // for UNUSED

#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <atu_api.h>
#include <boot_status.h> // for boot_status_notify_extd
#include <fpfw_cfg_mgr.h>
#include <fpfw_cfg_mgr_init.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <mscp_exp_rmss_memory_map.h>
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_cfg_mgr;
extern fpfw_init_component_t _fpfw_component_cfg_mgr_cli;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_cfg_mgr_init(fpfw_cfg_mgr_config_t* cfg_mgr_config, var_service_shared_mem_t* var_svc_mem_ctx)
{
    assert_non_null(cfg_mgr_config->read_knob_fn);
    assert_non_null(cfg_mgr_config->write_knob_fn);
    assert_true(var_svc_mem_ctx->max_payload_size == MSCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_SIZE);
    assert_true(var_svc_mem_ctx->payload_base == (uintptr_t)MSCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_BASE);

    check_expected(cfg_mgr_config->profile_id);

    function_called();
}

void __wrap_cfg_mgr_cli_init()
{
    function_called();
}

uint8_t __wrap_system_info_get_bmc_profile(void)
{
    return mock_type(uint8_t);
    ;
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
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

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}
}

//
// Tests
//
TEST_FUNCTION(test_cfg_mgr_init_platform, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    expect_function_call(__wrap_cfg_mgr_init);
    will_return(__wrap_system_info_get_bmc_profile, 0);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_value(__wrap_cfg_mgr_init, cfg_mgr_config->profile_id, 0);

    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_CFGMGR_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_cfg_mgr.init_fn();
}

TEST_FUNCTION(test_cfg_mgr_init_fpga, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    expect_function_call(__wrap_cfg_mgr_init);
    will_return(__wrap_system_info_get_bmc_profile, 0);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    expect_value(__wrap_cfg_mgr_init, cfg_mgr_config->profile_id, 1);

    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_CFGMGR_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_cfg_mgr.init_fn();
}

TEST_FUNCTION(test_cfg_mgr_init_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    expect_function_call(__wrap_cfg_mgr_init);
    will_return(__wrap_system_info_get_bmc_profile, 0);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_value(__wrap_cfg_mgr_init, cfg_mgr_config->profile_id, 2);

    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_CFGMGR_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_cfg_mgr.init_fn();
}

TEST_FUNCTION(test_cfg_mgr_cli_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_cfg_mgr_cli_init);

    // Check dependencies
    assert_string_equal("cfg_mgr", _fpfw_component_cfg_mgr_cli.children[0]);
    assert_string_equal("cli", _fpfw_component_cfg_mgr_cli.children[1]);

    // Call API under test
    _fpfw_component_cfg_mgr_cli.init_fn();
}

TEST_FUNCTION(test_cfg_mgr_init_bmc_compute_ovl2, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    expect_function_call(__wrap_cfg_mgr_init);
    will_return(__wrap_system_info_get_bmc_profile, 1);
    expect_value(__wrap_cfg_mgr_init, cfg_mgr_config->profile_id, 5);

    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_CFGMGR_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_cfg_mgr.init_fn();
}

TEST_FUNCTION(test_cfg_mgr_init_bmc_compute_ovl3, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    expect_function_call(__wrap_cfg_mgr_init);
    will_return(__wrap_system_info_get_bmc_profile, 2);
    expect_value(__wrap_cfg_mgr_init, cfg_mgr_config->profile_id, 6);

    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_CFGMGR_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_cfg_mgr.init_fn();
}

TEST_FUNCTION(test_cfg_mgr_init_init_tree, nullptr, nullptr)
{
    constexpr const char* expected_children[] =
        {"dfwk", "atu_svc", "var_serv", "hw_ver", "spi_bridge", "sysinfo", "icc_d2dmbx", "systick_upd", "gtimer"};

    bool found[_countof(expected_children)] = {false};

    for (size_t i = 0;; ++i)
    {
        const char* child = _fpfw_component_cfg_mgr.children[i];
        if (child[0] == '\0')
        {
            break;
        }

        for (size_t j = 0; j < _countof(expected_children); ++j)
        {
            if (strncmp(child, expected_children[j], FPFW_INIT_NODE_ID_LEN) == 0)
            {
                found[j] = true;
                break;
            }
        }
    }

    for (size_t j = 0; j < _countof(expected_children); ++j)
    {
        assert_true(found[j]);
    }
}