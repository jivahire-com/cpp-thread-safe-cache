//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pex_rng.cpp
 * pex_rng tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <pex_rng.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <core_cluster_top_regs.h>
#include <corebits.h>
#include <pex_regs.h>
#include <rng.h>
#include <silibs_ap_top_regs.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

void __wrap_rng_enable_r(uint32_t base, uint8_t div)
{
    check_expected(base);
    check_expected(div);
}

//
// Tests
//
TEST_FUNCTION(test_init_pex_rng, nullptr, nullptr)
{
    const corebits_t test_platform_cores = (corebits_t)COREBITS_INIT_UINT32(0x00000001, 0x00000000, 0x0);
    pex_rng_config_t test_config = {.cluster_pex_base = 0, .platform_cores_in_die = &test_platform_cores, .core_count = 1};
    expect_value(__wrap_FpFwAssert, expression, 1);
    expect_value(__wrap_rng_enable_r, base, (PEX_RNG_ADDRESS));
    expect_value(__wrap_rng_enable_r, div, 1);

    init_pex_rng(&test_config);
}
}