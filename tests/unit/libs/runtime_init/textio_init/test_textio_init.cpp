//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hw_version_init.cpp
 * Tests the init of the hw version component
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwUtils.h>
#include <fpfw_init.h>
#include <textio_pl011.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_uart;
extern fpfw_init_component_t _fpfw_component_std_io;

fpfw_init_component_t _fpfw_component_dfwk = {};

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_textio_pl011_device_initialize(textio_pl011_device_t* device, const textio_pl011_config_t* config, DFWK_SCHEDULE* schedule)
{
    FPFW_UNUSED(device);
    FPFW_UNUSED(config);
    FPFW_UNUSED(schedule);
    function_called();
}

void __wrap_textio_pl011_device_interface_initialize(textio_pl011_device_t* device, textio_pl011_interface_t* pl011_interface)
{
    FPFW_UNUSED(device);
    FPFW_UNUSED(pl011_interface);
    function_called();
}

//
// Tests
//

TEST_FUNCTION(textio_init_uart, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_textio_pl011_device_initialize);

    // Call API under test
    _fpfw_component_uart.init_fn();
}

TEST_FUNCTION(textio_init_std_io, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_textio_pl011_device_interface_initialize);
    expect_function_call(stdio_textio_init);

    // Call API under test
    _fpfw_component_std_io.init_fn();
}
}
