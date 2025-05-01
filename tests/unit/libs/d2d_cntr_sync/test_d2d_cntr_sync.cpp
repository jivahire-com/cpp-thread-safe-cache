//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_d2d_cntr_sync.cpp
 * d2d_cntr_sync tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h>
#include <d2d_cntr_sync.h>    // for d2d_cnter_sync_init
#include <d2d_counter_sync.h> // for d2d_cnter_sync_init
#include <fpfw_cfg_mgr.h>
#include <fpfw_status.h> // for FPFW_SUCCESS
#include <setjmp.h>      // for setjmp, longjmp
#include <stdint.h>      // for uint32_t, uintptr_t

/*-- Symbolic Constant Macros (defines) --*/
#define BUG_CHECK_RETURN_VALUE 0x1
#define BUGCHECK_MOCK_RETURN   (setjmp(cd_mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
d2d_counter_sync_knobs_t __real_config_get_d2d_counter_sync_knobs(void);

/*-- Declarations (Statics and globals) --*/
jmp_buf cd_mock_jump_buf;

/*------------- Functions ----------------*/
//
// Mocks
//
KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
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

void __wrap_d2d_counter_sync_enable(DIE_INSTANCE die_num)
{
    function_called();
    check_expected(die_num);
}

_Noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Handle noreturn, allowing control to return to test
    longjmp(cd_mock_jump_buf, BUG_CHECK_RETURN_VALUE);
}

d2d_counter_sync_knobs_t __wrap_config_get_d2d_counter_sync_knobs(void)
{
    d2d_counter_sync_knobs_t* d2d_config = mock_ptr_type(d2d_counter_sync_knobs_t*);
    function_called();
    return *d2d_config;
}

//! Test Helper APIs
void expect_value_wrap_d2d_counter_sync_configure_d0_and_enable_sync(void)
{
    //! default values for d2d_counter_sync_d0_config_t
    //! network_delay = 187,
    //! sync_timeout = 4294967295,
    //! mstupdt_addr_offset = 1,
    //! turn_ar_time = 26,
    //! round_trip_delay = 26,
    //! use_prg_rndt_delay = true,

    d2d_counter_sync_knobs_t d2d_cfg_knobs = __real_config_get_d2d_counter_sync_knobs();
    will_return(__wrap_config_get_d2d_counter_sync_knobs, &d2d_cfg_knobs);
    expect_function_call(__wrap_config_get_d2d_counter_sync_knobs);
    d2d_counter_sync_d0_config_t test_counter_sync_d0_config = d2d_cfg_knobs.d2d_counter_sync_d0_cfg;

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
    //! default values for d2d_counter_sync_d1_config_t
    //! counter_offset_threshold = 268435456,
    //! sync_timeout = 4294967295,
    //! sync_interval = 196608,
    //! turn_ar_time = 26,
    //! retry_count = 16,
    //! exp_updtime_threshold = 100,

    d2d_counter_sync_knobs_t d2d_cfg_knobs = __real_config_get_d2d_counter_sync_knobs();
    will_return(__wrap_config_get_d2d_counter_sync_knobs, &d2d_cfg_knobs);
    expect_function_call(__wrap_config_get_d2d_counter_sync_knobs);
    d2d_counter_sync_d1_config_t test_counter_sync_d1_config = d2d_cfg_knobs.d2d_counter_sync_d1_cfg;

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

static int cleanup(void** state)
{
    FPFW_UNUSED(state);
    // Reset any resources needed for the tests
    d2d_cntr_sync_reset();
    return 0;
}
}

//
// Tests
//
TEST_FUNCTION(test_d2d_cntr_sync_init_die_0_success, nullptr, cleanup)
{
    //! Set expectations for die 0
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    expect_value_wrap_d2d_counter_sync_configure_d0_and_enable_sync(); //! die 0 config & enable

    //! FUT
    d2d_cntr_sync_init(DIE_0, CPU_SCP);
}

TEST_FUNCTION(test_d2d_cntr_sync_init_die_1_success, nullptr, cleanup)
{
    //! Set expectations for die 1
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    expect_value_wrap_d2d_counter_sync_configure_d1_params(); //! die 1 config only

    //! FUT
    d2d_cntr_sync_init(DIE_1, CPU_SCP);
}

TEST_FUNCTION(test_d2d_cntr_sync_enable_die1_fail, nullptr, nullptr)
{
    //! Set expectations for die 1
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON); //! anything but SVP
    //! FUT, expect fail, die 1 not configured yet
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        d2d_cntr_sync_enable();
    }
}

TEST_FUNCTION(test_d2d_cntr_sync_enable_die1_success, nullptr, cleanup)
{
    //! Setup expectations for die 1
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON); //! anything but SVP
    expect_value_wrap_d2d_counter_sync_configure_d1_params();                   //! die 1 config only
    d2d_cntr_sync_init(DIE_1, CPU_SCP);

    //! FUT, expect pass
    expect_value(__wrap_d2d_counter_sync_enable, die_num, DIE_1);
    expect_function_call(__wrap_d2d_counter_sync_enable);
    d2d_cntr_sync_enable();
}