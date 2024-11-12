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
#include <FpFwCli.h>   // for FPFW_CLI_COMMAND, FpFwCliRegisterTable
#include <cstdint>     // for uint32_t
#include <fpfw_init.h> // for fpfw_init_component_t
#include <gpio.h>      // for pgpio_device_t, gpio_device_t, pgpio_in...
#include <gpio_lib.h>  // for gpio_afm_entry_t, gpio_config_entry_t
#include <idsw.h>      // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <silibs_status.h> // for SILIBS_SUCCESS
#include <string.h>        // for strcmp

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_gpio_lib;
extern fpfw_init_component_t _fpfw_component_gpio_dev;
extern fpfw_init_component_t _fpfw_component_gpio_cli;

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
    else if (strcmp(id, "gpio_lib") == 0)
    {
        static gpio_init_config_t gpio_init_config;

        return &gpio_init_config;
    }
    else if (strcmp(id, "gpio_dev") == 0)
    {
        static gpio_device_t gpio_dev;

        return &gpio_dev;
    }

    return nullptr;
}

void __wrap_FpFwCliRegisterTable(PFPFW_CLI_COMMAND pTable, size_t TableLength)
{
    size_t loop = 0;
    assert_non_null(pTable);
    assert_true(TableLength > 0);

    while (loop < TableLength)
    {
        assert_non_null(&pTable[loop]);
        assert_non_null(pTable[loop].MenuName);
        assert_non_null(pTable[loop].Syntax);
        assert_non_null(pTable[loop].Routine);
        assert_non_null(pTable[loop].Description);
        assert_non_null(pTable[loop].Usage);

        loop++;
    }
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
    check_expected_ptr(gpio_cfg_tbl);
    check_expected(num);

    function_called();

    return SILIBS_SUCCESS;
}

void __wrap_gpio_device_init(pgpio_device_t dev, PDFWK_SCHEDULE schedule)
{
    assert_non_null(dev);
    assert_non_null(schedule);

    function_called();
}

void __wrap_gpio_interface_init(pgpio_interface_t iface)
{
    assert_non_null(iface);

    function_called();
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    function_called();

    return mock_type(idsw_plat_id_t);
}

//
// Tests
//
TEST_FUNCTION(test_gpio_lib_init_SVP, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_gpio_afm_init);
    expect_function_call(__wrap_idsw_get_platform_sdv);
    expect_function_call(__wrap_idsw_get_platform_sdv);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_not_value(__wrap_gpio_init, gpio_cfg_tbl, NULL);
    expect_value(__wrap_gpio_init, num, 2);
    expect_function_call(__wrap_gpio_init);

    // Check dependencies
    assert_string_equal("mpu", _fpfw_component_gpio_lib.children[0]);
    assert_string_equal("hw_ver", _fpfw_component_gpio_lib.children[1]);

    // Call API under test
    _fpfw_component_gpio_lib.init_fn();
}

TEST_FUNCTION(test_gpio_lib_init_FPGA, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_gpio_afm_init);
    expect_function_call(__wrap_idsw_get_platform_sdv);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    expect_not_value(__wrap_gpio_init, gpio_cfg_tbl, NULL);
    expect_value(__wrap_gpio_init, num, 2); // Custom configuration table has 2 entries for MSCP_EXP_GPIO_4 and MSCP_EXP_GPIO_6
    expect_function_call(__wrap_gpio_init);

    // Check dependencies
    assert_string_equal("mpu", _fpfw_component_gpio_lib.children[0]);
    assert_string_equal("hw_ver", _fpfw_component_gpio_lib.children[1]);

    // Call API under test
    _fpfw_component_gpio_lib.init_fn();
}

TEST_FUNCTION(test_gpio_device_init, nullptr, nullptr)
{
    // Set up expectations
    expect_string(__wrap_fpfw_init_get_handle, id, "gpio_lib");
    expect_function_call(__wrap_fpfw_init_get_handle);

    expect_string(__wrap_fpfw_init_get_handle, id, "dfwk");
    expect_function_call(__wrap_fpfw_init_get_handle);

    expect_function_call(__wrap_gpio_device_init);

    // Check dependencies
    assert_string_equal("gpio_lib", _fpfw_component_gpio_dev.children[0]);
    assert_string_equal("dfwk", _fpfw_component_gpio_dev.children[1]);
    assert_string_equal("nvic", _fpfw_component_gpio_dev.children[2]);

    // Call API under test
    _fpfw_component_gpio_dev.init_fn();
}

TEST_FUNCTION(test_gpio_cli_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_gpio_interface_init);

    // Check dependencies
    assert_string_equal("gpio_dev", _fpfw_component_gpio_cli.children[0]);
    assert_string_equal("cli", _fpfw_component_gpio_cli.children[1]);

    // Call API under test
    _fpfw_component_gpio_cli.init_fn();
}
}
