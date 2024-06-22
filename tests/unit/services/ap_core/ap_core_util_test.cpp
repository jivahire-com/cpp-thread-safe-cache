//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_util_test.cpp
 * APcore service utility tests
 */

/*------------- Includes -----------------*/
#include "ap_core_test.h"

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

#include <CMockaWrapper.h> // for expect_value, check_expected_ptr, Cmo...
#include <ap_core_i.h>
#include <ap_core_init.h>
#include <core_cluster_with_pvt_regs.h>
#include <core_manager_and_clock_control_registers_regs.h>
#include <corebits.h>

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
// we wrap all these functions for main service testing, so we need to declare them here
extern "C" {
unsigned int __real_ap_core_util_boot_core(ap_core_service_context_t* p_context);
void __real_ap_core_util_set_rvbaraddr(ap_core_service_context_t* p_context, unsigned core_idx, uint64_t rvbaraddr);
void __real_ap_core_util_set_all_rvbaraddr(ap_core_service_context_t* p_context, uint64_t rvbaraddr);
void __real_ap_core_util_get_fuse_enabled_cores(corebits_t* enabled_cores);
}
/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {

} // extern "C"

//
// Tests
//
#define TEST_RVBARADDR  0x123456789abcdef0
#define TEST_CORE_COUNT 4
#define TEST_CORE       2
AP_CORE_TEST(util_get_boot_core, NULL, NULL)
{

    ap_core_service_context_t test_context;
    ap_core_service_config_t test_config;
    test_context.p_config = &test_config;

    test_config.platform_die_core_count = TEST_CORE_COUNT;

    // set up the enabled cores
    corebits_t test_enabled_cores = {0};
    corebits_set_bit(&test_enabled_cores, TEST_CORE_COUNT - 1);
    test_context.enabled_cores = test_enabled_cores;

    // expect null check
    expect_value(__wrap_FpFwAssert, expression, true);

    // expect the boot core to be last core
    unsigned int boot_core = __real_ap_core_util_boot_core(&test_context);
    assert_int_equal(boot_core, TEST_CORE_COUNT - 1);
}

AP_CORE_TEST(util_set_rvbaraddr, NULL, NULL)
{
    ap_core_service_context_t test_context;
    ap_core_service_config_t test_config;
    // we need a fake register region to write to
    core_manager_and_clock_control_registers_reg test_cm_ccr_reg = {};

    test_context.p_config = &test_config;

    // set the cluster pex start to match with our fake registers
    test_config.cluster_pex_base = ((uintptr_t)&test_cm_ccr_reg) - CORE_CLUSTER_WITH_PVT_CORE_MANAGER_ADDRESS;

    // expect null check
    expect_value(__wrap_FpFwAssert, expression, true);

    __real_ap_core_util_set_rvbaraddr(&test_context, 0, TEST_RVBARADDR);

    assert_int_equal(test_cm_ccr_reg.pe_rvbaraddr_lw.as_uint32, (uint32_t)TEST_RVBARADDR);
    assert_int_equal(test_cm_ccr_reg.pe_rvbaraddr_up.as_uint32, (uint32_t)(TEST_RVBARADDR >> 32));
}

AP_CORE_TEST(util_set_all_rvbaraddr, NULL, NULL)
{
    ap_core_service_context_t test_context;
    ap_core_service_config_t test_config;

    test_context.p_config = &test_config;

    // this API being tested requires core count and enabled cores
    test_config.platform_die_core_count = TEST_CORE_COUNT;

    // set up the enabled cores
    corebits_t test_enabled_cores = {0};
    // only enable one core
    corebits_set_bit(&test_enabled_cores, TEST_CORE);
    test_context.enabled_cores = test_enabled_cores;

    // expect null check
    expect_value(__wrap_FpFwAssert, expression, true);

    // rvbar expectations
    expect_value(__wrap_ap_core_util_set_rvbaraddr, p_context, &test_context);
    expect_value(__wrap_ap_core_util_set_rvbaraddr, core_idx, TEST_CORE);
    expect_value(__wrap_ap_core_util_set_rvbaraddr, rvbaraddr, TEST_RVBARADDR);

    __real_ap_core_util_set_all_rvbaraddr(&test_context, TEST_RVBARADDR);
}

AP_CORE_TEST(util_get_fuse_enabled_cores, NULL, NULL)
{
    corebits_t test_enabled_cores = {0};

    // expect null check
    expect_value(__wrap_FpFwAssert, expression, true);

    // no other expectations for now
    __real_ap_core_util_get_fuse_enabled_cores(&test_enabled_cores);
}