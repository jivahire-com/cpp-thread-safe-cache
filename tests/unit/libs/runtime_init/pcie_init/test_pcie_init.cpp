//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Unit tests for PCIe component initialization layer
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h>
#include <boot_status.h>
#include <cmocka.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <scp_pcie_manager.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pcie;
extern fpfw_init_component_t _fpfw_component_pcie_cli;
extern fpfw_init_component_t _fpfw_component_cxl_chbcr;
extern fpfw_init_component_t _fpfw_component_pcie_config;

/*------------- Functions ----------------*/
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    uintptr_t host_ptr = 0xDEADBEEF;
    return (void*)(host_ptr);
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv()
{
    return mock_type(idsw_plat_id_t);
}

void __wrap_pcie_cli_init(pciess_device_t* pcie_dev_handles)
{
    assert_non_null(pcie_dev_handles);
}

void __wrap_scp_pcie_initialize(PDFWK_SCHEDULE schedule, uint16_t rpss_to_init, KNG_DIE_ID die_id)
{
    assert_non_null(schedule);
    check_expected(rpss_to_init);
    check_expected(die_id);
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

TEST_FUNCTION(test_die0_svp_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die1_svp_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS4) | (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_1);

    const auto test_die = (KNG_DIE_ID)1;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_emu_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_1D);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die1_emu_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_2D);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS4) | (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_1);

    const auto test_die = (KNG_DIE_ID)1;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_fpga_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS1) | (1 << RPSS2)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die1_fpga_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, (1 << RPSS5));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_1);

    const auto test_die = (KNG_DIE_ID)1;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_silicon_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die1_silicon_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS4) | (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_1);

    const auto test_die = (KNG_DIE_ID)1;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_unknown_plat_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, 0xFF);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, 0);
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_cli_init, NULL, NULL)
{
    _fpfw_component_pcie_cli.init_fn();
}

TEST_FUNCTION(test_cxl_init, NULL, NULL)
{
    will_return(__wrap_idsw_get_die_id, DIE_1);
    _fpfw_component_cxl_chbcr.init_fn();
}

TEST_FUNCTION(test_pcie_config_init, NULL, NULL)
{
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_any_always(__wrap__txe_thread_create, thread_ptr);
    expect_any_always(__wrap__txe_thread_create, name_ptr);
    expect_any_always(__wrap__txe_thread_create, entry_function);
    expect_any_always(__wrap__txe_thread_create, entry_input);
    expect_any_always(__wrap__txe_thread_create, stack_start);
    expect_any_always(__wrap__txe_thread_create, stack_size);
    expect_any_always(__wrap__txe_thread_create, priority);
    expect_any_always(__wrap__txe_thread_create, preempt_threshold);
    expect_any_always(__wrap__txe_thread_create, time_slice);
    expect_any_always(__wrap__txe_thread_create, auto_start);
    expect_any_always(__wrap__txe_thread_create, thread_control_block_size);
    will_return_always(__wrap__txe_thread_create, TX_SUCCESS);
    _fpfw_component_pcie_config.init_fn();
}

TEST_FUNCTION(test_pcie_config_min_config, NULL, NULL)
{
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_MIN_CONFIG_SIM);
    _fpfw_component_pcie_config.init_fn();
}
