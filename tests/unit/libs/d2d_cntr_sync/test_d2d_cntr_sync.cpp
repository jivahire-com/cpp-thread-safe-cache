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
#include <fpfw_status.h>      // for FPFW_SUCCESS
#include <stdint.h>           // for uint32_t, uintptr_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_d2d_counter_sync_configure_d0_params(d2d_counter_sync_d0_config_t* counter_sync_config)
{
    check_expected(counter_sync_config->network_delay);
    check_expected(counter_sync_config->sync_timeout);
    check_expected(counter_sync_config->mstupdt_addr_offset);
    check_expected(counter_sync_config->turn_ar_time);
    check_expected(counter_sync_config->round_trip_delay);
    check_expected(counter_sync_config->use_prg_rndt_delay);
}

void __wrap_d2d_counter_sync_configure_d1_params(d2d_counter_sync_d1_config_t* counter_sync_config)
{
    check_expected(counter_sync_config->counter_offset_threshold);
    check_expected(counter_sync_config->sync_timeout);
    check_expected(counter_sync_config->sync_interval);
    check_expected(counter_sync_config->turn_ar_time);
    check_expected(counter_sync_config->retry_count);
    check_expected(counter_sync_config->exp_updtime_threshold);
}

void __wrap_d2d_counter_sync_enable(DIE_INSTANCE die_num)
{
    check_expected(die_num);
}
}

//
// Tests
//
TEST_FUNCTION(test_d2d_cntr_sync_init_die_0, nullptr, nullptr)
{
    expect_value(__wrap_d2d_counter_sync_configure_d0_params, counter_sync_config->network_delay, 187);
    expect_value(__wrap_d2d_counter_sync_configure_d0_params, counter_sync_config->sync_timeout, 4294967295);
    expect_value(__wrap_d2d_counter_sync_configure_d0_params, counter_sync_config->mstupdt_addr_offset, 1);
    expect_value(__wrap_d2d_counter_sync_configure_d0_params, counter_sync_config->turn_ar_time, 26);
    expect_value(__wrap_d2d_counter_sync_configure_d0_params, counter_sync_config->round_trip_delay, 26);
    expect_value(__wrap_d2d_counter_sync_configure_d0_params, counter_sync_config->use_prg_rndt_delay, true);

    expect_value(__wrap_d2d_counter_sync_enable, die_num, SOC_D0);

    d2d_cntr_sync_init(DIE_0);
}

TEST_FUNCTION(test_d2d_cntr_sync_init_die_1, nullptr, nullptr)
{
    expect_value(__wrap_d2d_counter_sync_configure_d1_params, counter_sync_config->counter_offset_threshold, 268435456);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params, counter_sync_config->sync_timeout, 4294967295);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params, counter_sync_config->sync_interval, 196608);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params, counter_sync_config->turn_ar_time, 26);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params, counter_sync_config->retry_count, 16);
    expect_value(__wrap_d2d_counter_sync_configure_d1_params, counter_sync_config->exp_updtime_threshold, 100);

    expect_value(__wrap_d2d_counter_sync_enable, die_num, SOC_D1);

    d2d_cntr_sync_init(DIE_1);
}
