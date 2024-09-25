/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file mock_dvfs.c
 * Provides the mocked version dvfs APIs necessary for power testing
 */

/*------------- Includes -----------------*/

#include "mock_dvfs.h"

// clang-format off
#include <setjmp.h>
// clang-format on
#include <cmocka.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/
#define UNUSED(a) ((void)a)
/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
static dvfs_config_t s_dvfs_cfg = {0};
static dvfs_vft_t s_dvfs_vft = {0};

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
void __wrap_dvfs_pll_dsm_toggle_start(const uintptr_t cluster_pex_base_addr)
{
    check_expected(cluster_pex_base_addr);
    return;
}

void __wrap_dvfs_pll_dsm_toggle_end(const uintptr_t cluster_pex_base_addr)
{
    check_expected(cluster_pex_base_addr);
    return;
}

int __wrap_wait_for_FLLCalDone(const uintptr_t cluster_pex_base_addr, uint32_t timeout_in_us)
{
    UNUSED(timeout_in_us);
    check_expected(cluster_pex_base_addr);
    return mock_type(int);
}

int __wrap_dvfs_init(const uintptr_t cluster_pex_base_addr, const dvfs_config_t* dvfs_cfg)
{
    UNUSED(cluster_pex_base_addr);
    memcpy_s(&s_dvfs_cfg, sizeof(s_dvfs_cfg), dvfs_cfg, sizeof(dvfs_config_t));
    memcpy_s(&s_dvfs_vft, sizeof(s_dvfs_vft), dvfs_cfg->fuse_cfg.vft, sizeof(dvfs_vft_t));

    return mock_type(int);
}

void __wrap_dvfs_telemetry_config(const uintptr_t cluster_pex_base_addr, const uint32_t pstate_telemetry_addr, const uint32_t scp_msg_addr)
{
    UNUSED(pstate_telemetry_addr);
    UNUSED(scp_msg_addr);
    check_expected(cluster_pex_base_addr);
    return;
}

void __wrap_dvfs_set_plimit(const uintptr_t cluster_pex_base_addr, uint8_t plimit_index, bool rack_power_cap)
{
    check_expected(cluster_pex_base_addr);
    check_expected(plimit_index);
    check_expected(rack_power_cap);
    return;
}

void __wrap_dvfs_config_plimit(const uintptr_t cluster_pex_base_addr, dvfs_plimit plimit)
{
    check_expected(cluster_pex_base_addr);
    uint32_t plimit_val = plimit.as_uint32;
    check_expected(plimit_val);
    return;
}

int __wrap_dvfs_get_adclk_droop_count(const uintptr_t cluster_pex_base_addr, uint32_t* droop_count)
{
    check_expected(cluster_pex_base_addr);
    *droop_count = mock_type(uint32_t);
    return mock_type(int);
}

void __wrap_dvfs_ns_set_cppc_desired(const uintptr_t cluster_pex_base_addr, uint8_t cppc_desired, uint8_t throttle_pri)
{
    check_expected(cluster_pex_base_addr);
    check_expected(cppc_desired);
    check_expected(throttle_pri);
    return;
}

const dvfs_config_t* mock_dvfs_last_config()
{
    return &s_dvfs_cfg;
}

const dvfs_vft_t* mock_dvfs_last_vft_config()
{
    return &s_dvfs_vft;
}
