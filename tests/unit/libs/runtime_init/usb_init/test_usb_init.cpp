//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_usb_init.cpp
 * USBSS init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_component_t
#include <idsw_kng.h>  // for KNG_DIE_ID, KNG_PLAT_ID
#include <stddef.h>    // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_usb;

/*------------- Functions ----------------*/
//
// Mocks
//

int __wrap_usb_init(uint32_t usb_init_block)
{
    FPFW_UNUSED(usb_init_block);
    function_called();

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
TEST_FUNCTION(test_usb_init_success, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idhw_get_die_id, test_die);
    expect_function_calls(__wrap_usb_init, 2);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_svp, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_fpga, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_fpga_large, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_fpga_tiny, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_TINY);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_fpga_small, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_SMALL);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_fpga_large_rvp, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE_RVP);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_zebu, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_zebu_1D, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_1D);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_zebu_2D, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_2D);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_zebu_1D_8C, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_1D_8C);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_bypass_zebu_2D_8C, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_2D_8C);

    // Call API under test
    _fpfw_component_usb.init_fn();
}

TEST_FUNCTION(test_usb_init_die1, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idhw_get_die_id, test_die);

    // Call API under test
    _fpfw_component_usb.init_fn();
}
}
