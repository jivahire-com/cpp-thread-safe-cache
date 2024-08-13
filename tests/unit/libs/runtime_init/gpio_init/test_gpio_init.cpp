//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_gpio_init.cpp
 * GPIO driver init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include "DfwkPtrTypes.h" // for PDFWK_SCHEDULE

#include <DfwkThreadXHost.h>
#include <cstdint>         // for uint32_t
#include <fpfw_init.h>     // for fpfw_init_component_t
#include <gpio.h>          // for pgpio_device_t, gpio_device_t, pgpio_in...
#include <gpio_lib.h>      // for gpio_afm_entry_t, gpio_config_entry_t
#include <silibs_status.h> // for SILIBS_SUCCESS
#include <string.h>        // for strcmp

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_gpio_lib;
extern fpfw_init_component_t _fpfw_component_gpio_dev;
extern fpfw_init_component_t _fpfw_component_gpio_int;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const char* id)
{
    check_expected_ptr(id);

    function_called();

    if (strcmp(id, "dfwk") == 0)
    {
        static DFWK_THREADX_HOST dfwk_host;

        return &dfwk_host;
    }
    else if (strcmp(id, "gpio_dev") == 0)
    {
        static gpio_device_t gpio_dev;

        return &gpio_dev;
    }

    return nullptr;
}

int __wrap_gpio_afm_init(const gpio_afm_entry_t* gpio_afm_tbl, uint32_t num)
{
    assert_null(gpio_afm_tbl); // Expecting nullptr to use default configuration
    assert_true(num == 0);     // Expecting 0 entries

    function_called();

    return SILIBS_SUCCESS;
}

int __wrap_gpio_init(const gpio_config_entry_t* gpio_cfg_tbl, uint32_t num)
{
    assert_null(gpio_cfg_tbl); // Expecting nullptr to use default configuration
    assert_true(num == 0);     // Expecting 0 entries

    function_called();

    return SILIBS_SUCCESS;
}

void __wrap_gpio_device_init(pgpio_device_t dev, PDFWK_SCHEDULE schedule)
{
    assert_non_null(dev);
    assert_non_null(schedule);

    function_called();
}

void __wrap_gpio_interface_init(pgpio_interface_t iface, pgpio_device_t dev)
{
    assert_non_null(iface);
    assert_non_null(dev);

    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_gpio_lib_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_gpio_afm_init);
    expect_function_call(__wrap_gpio_init);


    // Check dependencies
    assert_string_equal("mpu", _fpfw_component_gpio_lib.children[0]);

    // Call API under test
    _fpfw_component_gpio_lib.init_fn();
}

TEST_FUNCTION(test_gpio_device_init, nullptr, nullptr)
{
    // Set up expectations
    expect_string(__wrap_fpfw_init_get_handle, id, "dfwk");
    expect_function_call(__wrap_fpfw_init_get_handle);

    expect_function_call(__wrap_gpio_device_init);

    // Check dependencies
    assert_string_equal("gpio_lib", _fpfw_component_gpio_dev.children[0]);
    assert_string_equal("dfwk", _fpfw_component_gpio_dev.children[1]);

    // Call API under test
    _fpfw_component_gpio_dev.init_fn();
}

TEST_FUNCTION(test_gpio_interface_init, nullptr, nullptr)
{
    // Set up expectations
    expect_string(__wrap_fpfw_init_get_handle, id, "gpio_dev");
    expect_function_call(__wrap_fpfw_init_get_handle);

    expect_function_call(__wrap_gpio_interface_init);

    // Check dependencies
    assert_string_equal("gpio_dev", _fpfw_component_gpio_int.children[0]);
    assert_string_equal("nvic", _fpfw_component_gpio_int.children[1]);

    // Call API under test
    _fpfw_component_gpio_int.init_fn();
}
}
