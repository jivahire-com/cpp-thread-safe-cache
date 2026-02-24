//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Unit tests for gtimer initialization.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <boot_status.h>
#include <fpfw_init.h>
#include <gtimer_prodfw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_gtimer;

/*------------- Functions ----------------*/
/* Mocks */
KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

void __wrap_gtimer_prodfw_init(gtimer_prodfw_init_config_t* config)
{
    check_expected_ptr(config);
}

uint32_t __wrap_systick_get_emcpu_clock(void)
{
    // Mocked to return a fixed clock frequency for testing purposes
    return mock_type(uint32_t);
}

void __wrap_delay_init(uint32_t cpu_freq_hz, uint64_t (*get_counter)(void))
{
    check_expected(cpu_freq_hz);
    (void)get_counter;
}

uint64_t __wrap_gtimer_prodfw_get_counter(void)
{
    return mock_type(uint64_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

uint8_t __wrap_idsw_get_cpu_type()
{
    return mock_type(uint8_t);
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

/* Tests */
TEST_FUNCTION(test_gtimer_init_soc, NULL, NULL)
{
    gtimer_prodfw_init_config_t test_config = {.counter_control_base = SCP_TOP_GEN_CNTR_CTRL_ADDRESS,
                                               .timer_control_base = SCP_TOP_SCP_TIMER_CTRL_ADDRESS,
                                               .timer_base_address = SCP_TOP_SCP_TIMER_BASE_ADDRESS,
                                               .frequency_hz = (1 * 1000 * 1000 * 1000), /* 1GHz */
                                               .scaling_factor = 4,
                                               .timer_irq = HW_INT_SCP_GENERIC_TIMER_INT,
                                               .d2d_sync_point_required = false};
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    expect_memory(__wrap_gtimer_prodfw_init, config, &test_config, sizeof(gtimer_prodfw_init_config_t));

    will_return(__wrap_systick_get_emcpu_clock, (uint32_t)(1000000000U));
    expect_value(__wrap_delay_init, cpu_freq_hz, (uint32_t)(1000000000U));

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_PRE_GTIMER_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_gtimer.init_fn();
}

TEST_FUNCTION(test_gtimer_init_fpga, NULL, NULL)
{
    gtimer_prodfw_init_config_t test_config = {.counter_control_base = SCP_TOP_GEN_CNTR_CTRL_ADDRESS,
                                               .timer_control_base = SCP_TOP_SCP_TIMER_CTRL_ADDRESS,
                                               .timer_base_address = SCP_TOP_SCP_TIMER_BASE_ADDRESS,
                                               .frequency_hz = (10 * 1000 * 1000), /* 10 MHz */
                                               .scaling_factor = 1,
                                               .timer_irq = HW_INT_SCP_GENERIC_TIMER_INT,
                                               .d2d_sync_point_required = false};
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);

    expect_memory(__wrap_gtimer_prodfw_init, config, &test_config, sizeof(gtimer_prodfw_init_config_t));

    will_return(__wrap_systick_get_emcpu_clock, (uint32_t)(10000000U));
    expect_value(__wrap_delay_init, cpu_freq_hz, (uint32_t)(10000000U));

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_PRE_GTIMER_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_gtimer.init_fn();
}

TEST_FUNCTION(test_gtimer_init_svp, NULL, NULL)
{
    gtimer_prodfw_init_config_t test_config = {.counter_control_base = SCP_TOP_GEN_CNTR_CTRL_ADDRESS,
                                               .timer_control_base = SCP_TOP_SCP_TIMER_CTRL_ADDRESS,
                                               .timer_base_address = SCP_TOP_SCP_TIMER_BASE_ADDRESS,
                                               .frequency_hz = (125 * 1000 * 1000), /* 125 MHz */
                                               .scaling_factor = 1,
                                               .timer_irq = HW_INT_SCP_GENERIC_TIMER_INT,
                                               .d2d_sync_point_required = false};

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    expect_memory(__wrap_gtimer_prodfw_init, config, &test_config, sizeof(gtimer_prodfw_init_config_t));

    will_return(__wrap_systick_get_emcpu_clock, (uint32_t)(125000000U));
    expect_value(__wrap_delay_init, cpu_freq_hz, (uint32_t)(125000000U));

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_PRE_GTIMER_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_gtimer.init_fn();
}

TEST_FUNCTION(test_gtimer_init_svp_min_config, NULL, NULL)
{
    gtimer_prodfw_init_config_t test_config = {.counter_control_base = SCP_TOP_GEN_CNTR_CTRL_ADDRESS,
                                               .timer_control_base = SCP_TOP_SCP_TIMER_CTRL_ADDRESS,
                                               .timer_base_address = SCP_TOP_SCP_TIMER_BASE_ADDRESS,
                                               .frequency_hz = (125 * 1000 * 1000), /* 125 MHz */
                                               .scaling_factor = 1,
                                               .timer_irq = HW_INT_SCP_GENERIC_TIMER_INT,
                                               .d2d_sync_point_required = false};

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_MIN_CONFIG_SIM);

    expect_memory(__wrap_gtimer_prodfw_init, config, &test_config, sizeof(gtimer_prodfw_init_config_t));

    will_return(__wrap_systick_get_emcpu_clock, (uint32_t)(125000000U));
    expect_value(__wrap_delay_init, cpu_freq_hz, (uint32_t)(125000000U));

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_PRE_GTIMER_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_gtimer.init_fn();
}

} /* extern C */
