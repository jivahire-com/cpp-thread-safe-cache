//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_usb.cpp
 * USBSS tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include "idsw.h"

#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <cmocka.h>     // for mock_type
#include <error_handler.h>
#include <silibs_status.h> // for SILIBS_SUCCESS, SILIBS_E_INIT, SILIBS_E_P...
#include <usb.h>           // for usb_init
#include <usb_init.h>      // for usbss_cfg_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_usbss_init(uint32_t init_flag, usbss_cfg_t* usbss_cfg)
{
    check_expected(init_flag);
    FPFW_UNUSED(usbss_cfg);
    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_invalid_usb_init_block, nullptr, nullptr)
{
    int usb_init_block = 3;
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        usb_init(usb_init_block);
    }
}

TEST_FUNCTION(test_valid_usb_init1, nullptr, nullptr)
{
    expect_value(__wrap_usbss_init, init_flag, USBSS_INIT_USB2_0);
    expect_function_call(__wrap_usbss_init);
    usb_init(USBSS_INIT_USB2_0);
}

TEST_FUNCTION(test_valid_usb_init2, nullptr, nullptr)
{
    expect_value(__wrap_usbss_init, init_flag, USBSS_INIT_USB2_1);
    expect_function_call(__wrap_usbss_init);
    usb_init(USBSS_INIT_USB2_1);
}
}