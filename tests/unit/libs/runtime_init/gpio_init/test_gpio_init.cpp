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
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <boot_status.h>
#include <cstdint>         // for uint32_t
#include <fpfw_init.h>     // for fpfw_init_component_t
#include <gpio.h>          // for pgpio_device_t, gpio_device_t, pgpio_in...
#include <gpio_lib.h>      // for gpio_afm_entry_t, gpio_config_entry_t
#include <idsw.h>          // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>      // for KNG_DIE_ID, KNG_PLAT_ID
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
    check_expected_ptr(gpio_afm_tbl);
    check_expected(num);

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

int __wrap_gpio_override_uart_afmsel(uint8_t die_id, uart_afm_cfg_t* uart_afm)
{
    FPFW_UNUSED(die_id);
    FPFW_UNUSED(uart_afm);

    return mock_type(int);
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(idsw_plat_id_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type()
{
    return mock_type(idsw_cpu_type_t);
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    assert_non_null(addr);
    // return mock_type(uint32_t);
    return 0;
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    FPFW_UNUSED(data);
    assert_non_null(addr);
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_gpio_lib_init_SVP, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    // Set up expectations
    expect_value(__wrap_gpio_afm_init, gpio_afm_tbl, NULL);
    expect_value(__wrap_gpio_afm_init, num, 0);
    expect_function_call(__wrap_gpio_afm_init);

    expect_not_value(__wrap_gpio_afm_init, gpio_afm_tbl, NULL);
    expect_value(__wrap_gpio_afm_init, num, 2);
    expect_function_call(__wrap_gpio_afm_init);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_not_value(__wrap_gpio_init, gpio_cfg_tbl, NULL);
    expect_value(__wrap_gpio_init, num, 2);
    expect_function_call(__wrap_gpio_init);
    will_return(__wrap_gpio_override_uart_afmsel, SILIBS_SUCCESS);

    // Check dependencies
    assert_string_equal("mpu", _fpfw_component_gpio_lib.children[0]);
    assert_string_equal("hw_ver", _fpfw_component_gpio_lib.children[1]);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_GPIO_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_gpio_lib.init_fn();
}

TEST_FUNCTION(test_gpio_lib_init_FPGA, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    // Set up expectations

    expect_value(__wrap_gpio_afm_init, gpio_afm_tbl, NULL);
    expect_value(__wrap_gpio_afm_init, num, 0);
    expect_function_call(__wrap_gpio_afm_init);

    expect_not_value(__wrap_gpio_afm_init, gpio_afm_tbl, NULL);
    expect_value(__wrap_gpio_afm_init, num, 2);
    expect_function_call(__wrap_gpio_afm_init);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    expect_not_value(__wrap_gpio_init, gpio_cfg_tbl, NULL);
    expect_value(__wrap_gpio_init, num, 2); // Custom configuration table has 2 entries for MSCP_EXP_GPIO_4 and MSCP_EXP_GPIO_6
    expect_function_call(__wrap_gpio_init);
    will_return(__wrap_gpio_override_uart_afmsel, SILIBS_SUCCESS);

    // Check dependencies
    assert_string_equal("mpu", _fpfw_component_gpio_lib.children[0]);
    assert_string_equal("hw_ver", _fpfw_component_gpio_lib.children[1]);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_GPIO_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_gpio_lib.init_fn();
}

TEST_FUNCTION(test_gpio_lib_init_FPGA_die1, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    // Set up expectations

    expect_value(__wrap_gpio_afm_init, gpio_afm_tbl, NULL);
    expect_value(__wrap_gpio_afm_init, num, 0);
    expect_function_call(__wrap_gpio_afm_init);

    expect_not_value(__wrap_gpio_afm_init, gpio_afm_tbl, NULL);
    expect_value(__wrap_gpio_afm_init, num, 2);
    expect_function_call(__wrap_gpio_afm_init);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    expect_not_value(__wrap_gpio_init, gpio_cfg_tbl, NULL);
    expect_value(__wrap_gpio_init, num, 2); // Custom configuration table has 2 entries for MSCP_EXP_GPIO_4 and MSCP_EXP_GPIO_6
    expect_function_call(__wrap_gpio_init);
    will_return(__wrap_gpio_override_uart_afmsel, SILIBS_SUCCESS);

    // Check dependencies
    assert_string_equal("mpu", _fpfw_component_gpio_lib.children[0]);
    assert_string_equal("hw_ver", _fpfw_component_gpio_lib.children[1]);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_GPIO_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_gpio_lib.init_fn();
}

TEST_FUNCTION(test_gpio_lib_init_silicon_rvp, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    // Set up expectations
    expect_value(__wrap_gpio_afm_init, gpio_afm_tbl, NULL);
    expect_value(__wrap_gpio_afm_init, num, 0);
    expect_function_call(__wrap_gpio_afm_init);

    expect_not_value(__wrap_gpio_afm_init, gpio_afm_tbl, NULL);
    expect_value(__wrap_gpio_afm_init, num, 2);
    expect_function_call(__wrap_gpio_afm_init);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_not_value(__wrap_gpio_init, gpio_cfg_tbl, NULL);
    expect_value(__wrap_gpio_init, num, 2); // Custom configuration table has 2 entries for MSCP_EXP_GPIO_4 and MSCP_EXP_GPIO_6
    expect_function_call(__wrap_gpio_init);
    will_return(__wrap_gpio_override_uart_afmsel, SILIBS_SUCCESS);

    // Check dependencies
    assert_string_equal("mpu", _fpfw_component_gpio_lib.children[0]);
    assert_string_equal("hw_ver", _fpfw_component_gpio_lib.children[1]);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_GPIO_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_gpio_lib.init_fn();
}

TEST_FUNCTION(test_gpio_lib_init_FPGA_MCP, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

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
