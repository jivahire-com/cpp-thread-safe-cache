//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_ppu_test.cpp
 * APcore service ppu tests
 */

/*------------- Includes -----------------*/
#include "ap_core_test.h"

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

#include <CMockaWrapper.h> // for expect_value, check_expected_ptr, Cmo...
#include <DfwkCommon.h>    // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUE...
#include <ap_core.h>
#include <ap_core_i.h>
#include <core_cluster_with_pvt_regs.h> // for CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS
#include <corebits.h>
#include <idsw.h>     // for platform ID declarations
#include <idsw_kng.h> // for KNG_PLAT_ID
#include <pik_clock_lib.h>
#include <ppu_v1.h>
#include <voyager_dsu_cluster_regs.h> // for VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS, ...

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {

void __wrap_ppu_v1_init(uintptr_t ppu_addr)
{
    check_expected(ppu_addr);
}

int __wrap_pik_clock_power_transition(uint32_t dev_id, MOD_PD_STATE state)
{
    check_expected(dev_id);
    check_expected(state);

    return SILIBS_SUCCESS;
}

// Mock for ppu_v1_set_power_mode_with_timeout
int __wrap_ppu_v1_set_power_mode_with_timeout(uintptr_t ppu_v1_base_addr, PPU_V1_MODE ppu_mode, PPU_V1_OPMODE op_policy, uint32_t timeout_us)
{
    check_expected(ppu_v1_base_addr);
    check_expected(ppu_mode);
    check_expected(op_policy);
    check_expected(timeout_us);

    return SILIBS_SUCCESS;
}

// Mock for ppu_dynamic_enable
int __wrap_ppu_dynamic_enable(uintptr_t ppu_v1_base_addr, PPU_V1_MODE min_dyn_state)
{
    check_expected(ppu_v1_base_addr);
    check_expected(min_dyn_state);

    return SILIBS_SUCCESS;
}

} // extern "C"

//
// Tests
//
AP_CORE_TEST(ap_core_ppu_init, NULL, NULL)
{
    should_mock_ap_core_ppu_init = false;

    // Arrange
    ap_core_service_context_t context = {};
    ap_core_service_config_t test_config = {.platform_die_core_count = 1};
    context.p_config = &test_config;
    corebits_set_bit(&context.enabled_cores, 0);

    // Assert
    expect_value(__wrap_FpFwAssert, expression, 1);
    expect_value(__wrap_ppu_v1_init, ppu_addr, CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);
    expect_value(__wrap_ppu_v1_init, ppu_addr, CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CORE0_PPU_ADDRESS);

    // Act
    ap_core_ppu_init(&context);
}

AP_CORE_TEST(ap_core_ppu_clusters_on, NULL, NULL)
{
    // Arrange
    ap_core_service_context_t context = {};
    ap_core_service_config_t test_config = {
        .cluster_pex_base = 0x1000,
        .cluster_stride = 0x100,
        .platform_die_core_count = 3, // Assume 3 cores for this test
    };
    context.p_config = &test_config;

    // Mock idsw_get_platform_sdv to return PLATFORM_RVP_EVT_SILICON
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    // Enable cores 0 and 2
    corebits_set_bit(&context.enabled_cores, 0);
    corebits_set_bit(&context.enabled_cores, 2);

    uint32_t timeout_ms = 100;

    expect_value(__wrap_FpFwAssert, expression, 1);

    // Expect pik_clock_power_transition to be called for cluster clocks of cores 0 and 2
    for (int clk_dev_id = PIK_DEV_ID_CLUS_CORECLK; clk_dev_id <= PIK_DEV_ID_CLUS_PPUCLK; clk_dev_id++)
    {
        expect_value(__wrap_pik_clock_power_transition, dev_id, PIK_CLUS_PIK_DEV_ID(0, clk_dev_id));
        expect_value(__wrap_pik_clock_power_transition, state, MOD_PD_STATE_ON);
        expect_value(__wrap_FpFwAssert, expression, 1);
    }

    for (int clk_dev_id = PIK_DEV_ID_CLUS_CORECLK; clk_dev_id <= PIK_DEV_ID_CLUS_PPUCLK; clk_dev_id++)
    {
        expect_value(__wrap_pik_clock_power_transition, dev_id, PIK_CLUS_PIK_DEV_ID(2, clk_dev_id));
        expect_value(__wrap_pik_clock_power_transition, state, MOD_PD_STATE_ON);
        expect_value(__wrap_FpFwAssert, expression, 1);
    }

    // Expect ppu_v1_set_power_mode_with_timeout to be called for cores 0 and 2
    expect_value(__wrap_ppu_v1_set_power_mode_with_timeout,
                 ppu_v1_base_addr,
                 test_config.cluster_pex_base + CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS +
                     VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);
    expect_value(__wrap_ppu_v1_set_power_mode_with_timeout, ppu_mode, PPU_V1_MODE_ON);
    expect_value(__wrap_ppu_v1_set_power_mode_with_timeout, op_policy, PPU_V1_OPMODE_00);
    expect_value(__wrap_ppu_v1_set_power_mode_with_timeout, timeout_us, timeout_ms);

    expect_value(__wrap_ppu_v1_set_power_mode_with_timeout,
                 ppu_v1_base_addr,
                 test_config.cluster_pex_base + (2 * test_config.cluster_stride) +
                     CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);
    expect_value(__wrap_ppu_v1_set_power_mode_with_timeout, ppu_mode, PPU_V1_MODE_ON);
    expect_value(__wrap_ppu_v1_set_power_mode_with_timeout, op_policy, PPU_V1_OPMODE_00);
    expect_value(__wrap_ppu_v1_set_power_mode_with_timeout, timeout_us, timeout_ms);

    // Expect ppu_dynamic_enable to be called for cores 0 and 2
    expect_value(__wrap_ppu_dynamic_enable,
                 ppu_v1_base_addr,
                 test_config.cluster_pex_base + CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS +
                     VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);
    expect_value(__wrap_ppu_dynamic_enable, min_dyn_state, PPU_V1_MODE_OFF);

    expect_value(__wrap_ppu_dynamic_enable,
                 ppu_v1_base_addr,
                 test_config.cluster_pex_base + (2 * test_config.cluster_stride) +
                     CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);
    expect_value(__wrap_ppu_dynamic_enable, min_dyn_state, PPU_V1_MODE_OFF);

    // Always return PLATFORM_RVP_EVT_SILICON for __wrap_idsw_get_platform_sdv
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    // Act
    __real_ap_core_ppu_clusters_on(&context, timeout_ms);
}
