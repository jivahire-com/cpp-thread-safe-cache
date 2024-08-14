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
#include <atu_lib.h>    // for atu_id_t, atu_map_entry_t
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
int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    FPFW_UNUSED(atu_id);
    FPFW_UNUSED(atu_map_entry);

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    FPFW_UNUSED(atu_id);
    FPFW_UNUSED(atu_map_entry);

    return mock_type(int);
}

int __wrap_set_mscp_ioss_base_addr(uintptr_t mscp_resolved_base)
{
    FPFW_UNUSED(mscp_resolved_base);
    return mock_type(int);
}

void __wrap_usbss_init(uint32_t init_flag, usbss_cfg_t* usbss_cfg)
{
    FPFW_UNUSED(init_flag);
    FPFW_UNUSED(usbss_cfg);
    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_invalid_usb_init_block, nullptr, nullptr)
{
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        usb_init((USBSS_INIT_USB2_0));
    }
}

TEST_FUNCTION(test_valid_usb_init1, nullptr, nullptr)
{
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_set_mscp_ioss_base_addr, SILIBS_SUCCESS);
    expect_function_call(__wrap_usbss_init);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    usb_init((USBSS_INIT_USB2_0 | USBSS_INIT_USB2_1));
}

TEST_FUNCTION(test_valid_usb_init2, nullptr, nullptr)
{
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_set_mscp_ioss_base_addr, SILIBS_SUCCESS);
    expect_function_call(__wrap_usbss_init);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    usb_init((USBSS_INIT_USB2_0 | USBSS_INIT_USB2_1));
}
}