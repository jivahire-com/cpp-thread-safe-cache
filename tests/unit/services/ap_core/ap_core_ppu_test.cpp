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
