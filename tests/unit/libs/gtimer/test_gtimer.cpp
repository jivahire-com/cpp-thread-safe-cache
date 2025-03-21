//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_gtimer.cpp
 * gtimer tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FPFwInterrupts.h>
#include <FpFwUtils.h>
#include <d2d_counter_sync.h>
#include <fpfw_status.h>   // for FPFW_SUCCESS
#include <gtimer_prodfw.h> // for gtimer_init
#include <idsw.h>
#include <idsw_kng.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <refclk_counter_cnt_control_regs.h> // for refclk_counter_cnt_cont...
#include <stdint.h>                          // for uint32_t, uintptr_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
uint8_t __wrap_idsw_get_cpu_type()
{
    return mock_type(uint8_t);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

KNG_DIE_ID __wrap_idhw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

void __wrap_system_counter_init(const uintptr_t gen_counter_ctrl_base, uint32_t frequency_hz, uint8_t increment_val)
{
    check_expected(gen_counter_ctrl_base);
    check_expected(frequency_hz);
    check_expected(increment_val);
}

void __wrap_gtimer_init(const uintptr_t timer_cntrl_addr)
{
    check_expected(timer_cntrl_addr);
}

void __wrap_gtimer_enable_timer(const uintptr_t timer_base_addr)
{
    check_expected(timer_base_addr);
}

uint32_t __wrap_FPFwCoreInterruptEnableVector(uint32_t irqnum)
{
    check_expected(irqnum);
    return 0;
}

uint32_t __wrap_FPFwCoreInterruptDisableVector(uint32_t irqnum)
{
    check_expected(irqnum);
    return 0;
}

uint32_t __wrap_FPFwCoreInterruptRegisterCallback(uint32_t irqnum, FPFwCoreInterruptHandler handler, void* arg)
{
    check_expected(irqnum);
    FPFW_UNUSED(handler);
    FPFW_UNUSED(arg);

    return 0;
}

fpfw_status_t __wrap_fpfw_tmr_queue_add_oneshot(fpfw_tmr_queue_t* queue,
                                                fpfw_tmr_entry_t* tmr,
                                                uint64_t tick_interval,
                                                void (*cb)(void*, uint64_t, uint64_t),
                                                void* ctx)
{
    assert_non_null(queue);
    check_expected_ptr(tmr);
    check_expected(tick_interval);
    check_expected_ptr(cb);
    check_expected_ptr(ctx);

    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t __wrap_fpfw_tmr_queue_add_periodic(fpfw_tmr_queue_t* queue,
                                                 fpfw_tmr_entry_t* tmr,
                                                 uint64_t init_tick,
                                                 uint64_t tick_interval,
                                                 void (*cb)(void*, uint64_t, uint64_t),
                                                 void* ctx)
{
    assert_non_null(queue);
    check_expected_ptr(tmr);
    check_expected(init_tick);
    check_expected(tick_interval);
    check_expected_ptr(cb);
    check_expected_ptr(ctx);

    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t __wrap_fpfw_tmr_queue_expire(fpfw_tmr_queue_t* queue, uint64_t* next_tick)
{
    assert_non_null(queue);
    assert_non_null(next_tick);
    *next_tick = mock_type(uint64_t);

    return FPFW_STATUS_SUCCESS;
}

void __wrap_gtimer_set_timer(const uintptr_t timer_base_addr, uint64_t timestamp)
{
    FPFW_UNUSED(timer_base_addr);
    check_expected(timestamp);
}

uint64_t __wrap_gtimer_get_counter(const uintptr_t timer_base_addr)
{
    FPFW_UNUSED(timer_base_addr);
    return mock_type(uint64_t);
}

void __wrap_d2d_counter_sync_configure_d0_and_enable_sync(d2d_counter_sync_d0_config_t* counter_sync_config)
{
    assert_non_null(counter_sync_config);
    check_expected(counter_sync_config->network_delay);
    check_expected(counter_sync_config->sync_timeout);
    check_expected(counter_sync_config->mstupdt_addr_offset);
    check_expected(counter_sync_config->turn_ar_time);
    check_expected(counter_sync_config->round_trip_delay);
    check_expected(counter_sync_config->use_prg_rndt_delay);
}

void __wrap_d2d_counter_sync_configure_d1_params(d2d_counter_sync_d1_config_t* counter_sync_config)
{
    assert_non_null(counter_sync_config);
    check_expected(counter_sync_config->counter_offset_threshold);
    check_expected(counter_sync_config->sync_timeout);
    check_expected(counter_sync_config->sync_interval);
    check_expected(counter_sync_config->turn_ar_time);
    check_expected(counter_sync_config->retry_count);
    check_expected(counter_sync_config->exp_updtime_threshold);
}

int __wrap_mscp_exp_spi_synchronize_dies(mscp_exp_spi_sync_point_t sync_point, int die_id)
{
    FPFW_UNUSED(die_id); //! unused parameter
    assert_int_equal(sync_point.value, D2D_SYS_CNT_START);
    return mock_type(int);
}

//! Test Helper APIs
void expect_value_wrap_d2d_counter_sync_configure_d0_and_enable_sync(void)
{
    d2d_counter_sync_d0_config_t test_counter_sync_d0_config = {
        .network_delay = 187,
        .sync_timeout = 4294967295,
        .mstupdt_addr_offset = 1,
        .turn_ar_time = 26,
        .round_trip_delay = 26,
        .use_prg_rndt_delay = true,
    };

    expect_value(__wrap_d2d_counter_sync_configure_d0_and_enable_sync,
                 counter_sync_config->network_delay,
                 test_counter_sync_d0_config.network_delay);
    expect_value(__wrap_d2d_counter_sync_configure_d0_and_enable_sync,
                 counter_sync_config->sync_timeout,
                 test_counter_sync_d0_config.sync_timeout);
    expect_value(__wrap_d2d_counter_sync_configure_d0_and_enable_sync,
                 counter_sync_config->mstupdt_addr_offset,
                 test_counter_sync_d0_config.mstupdt_addr_offset);
    expect_value(__wrap_d2d_counter_sync_configure_d0_and_enable_sync,
                 counter_sync_config->turn_ar_time,
                 test_counter_sync_d0_config.turn_ar_time);
    expect_value(__wrap_d2d_counter_sync_configure_d0_and_enable_sync,
                 counter_sync_config->round_trip_delay,
                 test_counter_sync_d0_config.round_trip_delay);
    expect_value(__wrap_d2d_counter_sync_configure_d0_and_enable_sync,
                 counter_sync_config->use_prg_rndt_delay,
                 test_counter_sync_d0_config.use_prg_rndt_delay);
}

void expect_value_wrap_d2d_counter_sync_configure_d1_params(void)
{
    d2d_counter_sync_d1_config_t test_counter_sync_d1_config = {
        .counter_offset_threshold = 268435456,
        .sync_timeout = 4294967295,
        .sync_interval = 196608,
        .turn_ar_time = 26,
        .retry_count = 16,
        .exp_updtime_threshold = 100,
    };

    expect_value(__wrap_d2d_counter_sync_configure_d1_params,
                 counter_sync_config->counter_offset_threshold,
                 test_counter_sync_d1_config.counter_offset_threshold);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params,
                 counter_sync_config->sync_timeout,
                 test_counter_sync_d1_config.sync_timeout);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params,
                 counter_sync_config->sync_interval,
                 test_counter_sync_d1_config.sync_interval);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params,
                 counter_sync_config->turn_ar_time,
                 test_counter_sync_d1_config.turn_ar_time);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params,
                 counter_sync_config->retry_count,
                 test_counter_sync_d1_config.retry_count);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params,
                 counter_sync_config->exp_updtime_threshold,
                 test_counter_sync_d1_config.exp_updtime_threshold);
}
}

//
// Tests
//
TEST_FUNCTION(test_gtimer_init_single_die_scp, nullptr, nullptr)
{
    const uint32_t REFCLK_FREQUENCY_HZ = 250000000;
    const uint32_t REFCLK_SCALING_FACTOR = 4;

    // Set up expectations
    gtimer_prodfw_init_config_t test_config = {
        .counter_control_base = 42,
        .timer_control_base = 43,
        .timer_base_address = 44,
        .frequency_hz = REFCLK_FREQUENCY_HZ * REFCLK_SCALING_FACTOR,
        .scaling_factor = REFCLK_SCALING_FACTOR,
        .timer_irq = 45,
    };

    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    //! no d2d counter sync SPI synchronization for single die boot

    expect_value(__wrap_system_counter_init, gen_counter_ctrl_base, test_config.counter_control_base);
    expect_value(__wrap_system_counter_init, frequency_hz, test_config.frequency_hz);
    expect_value(__wrap_system_counter_init, increment_val, test_config.scaling_factor);

    expect_value(__wrap_gtimer_init, timer_cntrl_addr, test_config.timer_control_base);
    expect_value(__wrap_gtimer_enable_timer, timer_base_addr, test_config.timer_base_address);

    will_return_maybe(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    //! No d2d counter sync logic for single die regardless of platform

    expect_value(__wrap_FPFwCoreInterruptRegisterCallback, irqnum, 45);
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, 45);

    // Call API under test
    gtimer_prodfw_init(&test_config);
}

TEST_FUNCTION(test_gtimer_init_mcp, nullptr, nullptr)
{
    const uint32_t REFCLK_FREQUENCY_HZ = 250000000;
    const uint32_t REFCLK_SCALING_FACTOR = 4;

    // Set up expectations
    gtimer_prodfw_init_config_t test_config = {
        .counter_control_base = 42,
        .timer_control_base = 43,
        .timer_base_address = 44,
        .frequency_hz = REFCLK_FREQUENCY_HZ * REFCLK_SCALING_FACTOR,
        .scaling_factor = REFCLK_SCALING_FACTOR,
        .timer_irq = 45,
    };

    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);
    //! no sys counter init & d2d counter sync SPI synchronization for MCP

    expect_value(__wrap_gtimer_init, timer_cntrl_addr, test_config.timer_control_base);
    expect_value(__wrap_gtimer_enable_timer, timer_base_addr, test_config.timer_base_address);

    //! No d2d counter sync logic for mcp regardless of platform or single/dual die
    will_return_maybe(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    expect_value(__wrap_FPFwCoreInterruptRegisterCallback, irqnum, 45);
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, 45);

    // Call API under test
    gtimer_prodfw_init(&test_config);
}

TEST_FUNCTION(test_gtimer_init_svp_dual_die_scp, nullptr, nullptr)
{
    const uint32_t REFCLK_FREQUENCY_HZ = 250000000;
    const uint32_t REFCLK_SCALING_FACTOR = 4;

    // Set up expectations
    gtimer_prodfw_init_config_t test_config = {
        .counter_control_base = 42,
        .timer_control_base = 43,
        .timer_base_address = 44,
        .frequency_hz = REFCLK_FREQUENCY_HZ * REFCLK_SCALING_FACTOR,
        .scaling_factor = REFCLK_SCALING_FACTOR,
        .timer_irq = 45,
    };

    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    expect_value(__wrap_system_counter_init, gen_counter_ctrl_base, test_config.counter_control_base);
    expect_value(__wrap_system_counter_init, frequency_hz, test_config.frequency_hz);
    expect_value(__wrap_system_counter_init, increment_val, test_config.scaling_factor);

    expect_value(__wrap_gtimer_init, timer_cntrl_addr, test_config.timer_control_base);
    expect_value(__wrap_gtimer_enable_timer, timer_base_addr, test_config.timer_base_address);

    //! No d2d counter sync logic for SVP platform
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_value(__wrap_FPFwCoreInterruptRegisterCallback, irqnum, 45);
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, 45);

    // Call API under test
    gtimer_prodfw_init(&test_config);
}

TEST_FUNCTION(test_gtimer_init_soc_dual_die_scp_d0, nullptr, nullptr)
{
    const uint32_t REFCLK_FREQUENCY_HZ = 250000000;
    const uint32_t REFCLK_SCALING_FACTOR = 4;

    // Set up expectations
    gtimer_prodfw_init_config_t test_config = {
        .counter_control_base = 42,
        .timer_control_base = 43,
        .timer_base_address = 44,
        .frequency_hz = REFCLK_FREQUENCY_HZ * REFCLK_SCALING_FACTOR,
        .scaling_factor = REFCLK_SCALING_FACTOR,
        .timer_irq = 45,
    };

    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    expect_value(__wrap_system_counter_init, gen_counter_ctrl_base, test_config.counter_control_base);
    expect_value(__wrap_system_counter_init, frequency_hz, test_config.frequency_hz);
    expect_value(__wrap_system_counter_init, increment_val, test_config.scaling_factor);

    expect_value(__wrap_gtimer_init, timer_cntrl_addr, test_config.timer_control_base);
    expect_value(__wrap_gtimer_enable_timer, timer_base_addr, test_config.timer_base_address);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON); //! anything but SVP
    will_return(__wrap_idhw_get_die_id, DIE_0);
    expect_value_wrap_d2d_counter_sync_configure_d0_and_enable_sync(); //! die 0 config & enable

    expect_value(__wrap_FPFwCoreInterruptRegisterCallback, irqnum, 45);
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, 45);

    // Call API under test
    gtimer_prodfw_init(&test_config);
}

TEST_FUNCTION(test_gtimer_init_soc_dual_die_scp_d1, nullptr, nullptr)
{
    const uint32_t REFCLK_FREQUENCY_HZ = 250000000;
    const uint32_t REFCLK_SCALING_FACTOR = 4;

    // Set up expectations
    gtimer_prodfw_init_config_t test_config = {
        .counter_control_base = 42,
        .timer_control_base = 43,
        .timer_base_address = 44,
        .frequency_hz = REFCLK_FREQUENCY_HZ * REFCLK_SCALING_FACTOR,
        .scaling_factor = REFCLK_SCALING_FACTOR,
        .timer_irq = 45,
    };

    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    expect_value(__wrap_system_counter_init, gen_counter_ctrl_base, test_config.counter_control_base);
    expect_value(__wrap_system_counter_init, frequency_hz, test_config.frequency_hz);
    expect_value(__wrap_system_counter_init, increment_val, test_config.scaling_factor);

    expect_value(__wrap_gtimer_init, timer_cntrl_addr, test_config.timer_control_base);
    expect_value(__wrap_gtimer_enable_timer, timer_base_addr, test_config.timer_base_address);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON); //! anything but SVP
    will_return(__wrap_idhw_get_die_id, DIE_1);
    expect_value_wrap_d2d_counter_sync_configure_d1_params(); //! die 1 only config

    expect_value(__wrap_FPFwCoreInterruptRegisterCallback, irqnum, 45);
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, 45);

    // Call API under test
    gtimer_prodfw_init(&test_config);
}

TEST_FUNCTION(test_gtimer_add_oneshot, nullptr, nullptr)
{
    // Set up expectations
    fpfw_tmr_entry_t tmr = {};
    uint64_t tick_interval = 1000;
    void (*cb)(void*, uint64_t, uint64_t) = [](void*, uint64_t, uint64_t) {};
    void* ctx = nullptr;

    expect_value(__wrap_fpfw_tmr_queue_add_oneshot, tmr, &tmr);
    expect_value(__wrap_fpfw_tmr_queue_add_oneshot, tick_interval, tick_interval);
    expect_value(__wrap_fpfw_tmr_queue_add_oneshot, cb, cb);
    expect_value(__wrap_fpfw_tmr_queue_add_oneshot, ctx, ctx);

    will_return(__wrap_fpfw_tmr_queue_expire, 2000);

    expect_value(__wrap_FPFwCoreInterruptDisableVector, irqnum, 45);
    expect_value(__wrap_gtimer_set_timer, timestamp, 2000);
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, 45);

    // Call API under test
    gtimer_add_oneshot(&tmr, tick_interval, cb, ctx);
}

TEST_FUNCTION(test_gtimer_add_periodic, nullptr, nullptr)
{
    // Set up expectations
    fpfw_tmr_entry_t tmr = {};
    uint64_t init_tick = 1000;
    uint64_t tick_interval = 2000;
    void (*cb)(void*, uint64_t, uint64_t) = [](void*, uint64_t, uint64_t) {};
    void* ctx = nullptr;

    expect_value(__wrap_fpfw_tmr_queue_add_periodic, tmr, &tmr);
    expect_value(__wrap_fpfw_tmr_queue_add_periodic, init_tick, init_tick);
    expect_value(__wrap_fpfw_tmr_queue_add_periodic, tick_interval, tick_interval);
    expect_value(__wrap_fpfw_tmr_queue_add_periodic, cb, cb);
    expect_value(__wrap_fpfw_tmr_queue_add_periodic, ctx, ctx);

    will_return(__wrap_fpfw_tmr_queue_expire, 3000);
    will_return(__wrap_gtimer_get_counter, 1000);

    expect_value(__wrap_FPFwCoreInterruptDisableVector, irqnum, 45);
    expect_value(__wrap_gtimer_set_timer, timestamp, 3000);
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, 45);

    // Call API under test
    gtimer_add_periodic(&tmr, tick_interval, cb, ctx);
}