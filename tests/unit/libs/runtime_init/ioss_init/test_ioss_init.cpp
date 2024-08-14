//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ioss_init.cpp
 * IOSS init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_component_t
#include <idsw_kng.h>  // for KNG_DIE_ID, KNG_PLAT_ID

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_ioss;

/*------------- Functions ----------------*/
//
// Mocks
//
int __wrap_ioss_init(uint8_t die_num)
{
    check_expected(die_num);

    return 0;
}

KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

//
// Tests
//
TEST_FUNCTION(test_ioss_init_silicon, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idhw_get_die_id, test_die);
    expect_value(__wrap_ioss_init, die_num, test_die);

    // Call API under test
    _fpfw_component_ioss.init_fn();
}

TEST_FUNCTION(test_ioss_init_svp, NULL, NULL)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_idhw_get_die_id, test_die);
    expect_value(__wrap_ioss_init, die_num, test_die);

    // Call API under test
    _fpfw_component_ioss.init_fn();
}

TEST_FUNCTION(test_ioss_init_bypass_fpga, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    // Call API under test
    _fpfw_component_ioss.init_fn();
}

TEST_FUNCTION(test_ioss_init_bypass_fpga_large, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);

    // Call API under test
    _fpfw_component_ioss.init_fn();
}

TEST_FUNCTION(test_ioss_init_die1, NULL, NULL)
{
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idhw_get_die_id, test_die);

    // Call API under test
    _fpfw_component_ioss.init_fn();
}
}
