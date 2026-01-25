//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hw_version_init.cpp
 * Tests the init of the hw version component
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:textio_init

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h>      // for DFWK_SCHEDULE
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <FpFwUtils.h>       // for FPFW_UNUSED
#include <atu_api.h>
#include <boot_status.h> // for boot_status_notify_extd
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <idsw.h>
#include <idsw_kng.h>
#include <textio_pl011.h> // for textio_pl011_config_t, textio_pl011_dev...
#include <uart_pl011.h>   // for UART_PL011_PARITY_NONE, UART_PL011_STOP...

/*-- Symbolic Constant Macros (defines) --*/
#define UART_BAUD_RATE (115200)
#define UART_CLK_FREQ  (10000000)

/*------------- Typedefs -----------------*/

bool __real_config_get_uart_mcp_reassign(void);

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_uart;
extern fpfw_init_component_t _fpfw_component_std_io;

static bool mock_mcp_reassign = false;

/*------------- Functions ----------------*/
//
// Mocks
//
idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(idsw_plat_id_t);
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

uint32_t __wrap_systick_get_emcpu_clock(void)
{
    // Mocked to return a fixed clock frequency for testing purposes
    return mock_type(uint32_t);
}

void __wrap_textio_pl011_device_initialize(textio_pl011_device_t* device, const textio_pl011_config_t* config, DFWK_SCHEDULE* schedule)
{
    assert_non_null(device);
    assert_non_null(config);
    check_expected_ptr(schedule);
    //! Verify default expected uart configuration settings set by the init function
    check_expected(config->base_address);
    assert_int_equal(config->vuart_base_address, VUART_BASE_ADDR);
    assert_int_equal(config->interrupt, UART_IRQ);
    assert_int_equal(config->vuart_interrupt, VUART_IRQ);
    assert_int_equal(config->baud_rate, UART_BAUD_RATE);
    check_expected(config->clk_freq);
    assert_int_equal(config->wlen, UART_PL011_WLEN_8);
    assert_int_equal(config->stop_bits, UART_PL011_STOP_BITS_1);
    assert_int_equal(config->parity, UART_PL011_PARITY_NONE);
    check_expected(config->config_type);
    assert_true(config->is_vuart_enabled); // Verify virtual UART is enabled
    function_called();
}

void __wrap_textio_pl011_device_interface_initialize(textio_pl011_device_t* device, textio_pl011_interface_t* pl011_interface)
{
    check_expected_ptr(device);
    assert_non_null(pl011_interface);
    function_called();
}

bool __wrap_config_get_uart_mcp_reassign(void)
{
    if (mock_mcp_reassign)
    {
        return mock_type(bool);
    }
    return __real_config_get_uart_mcp_reassign();
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

void __wrap_stdio_textio_init(PDFWK_INTERFACE_HEADER textio_interface)
{
    FPFW_UNUSED(textio_interface);
    function_called();
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

TEST_FUNCTION(textio_init_uart, nullptr, nullptr)
{
    //! Set up expectations
    mock_mcp_reassign = true;
    DFWK_THREADX_HOST test_host = {};
    const auto test_die = (KNG_DIE_ID)0;

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    will_return(__wrap_fpfw_init_get_handle, &test_host); //! driver fmwk host handle
    expect_value(__wrap_textio_pl011_device_initialize, schedule, &(test_host.Schedule));
    expect_value(__wrap_textio_pl011_device_initialize, config->base_address, UART_BASE_ADDR);
    expect_value(__wrap_textio_pl011_device_initialize, config->config_type, TEXTIO_PL011_CONFIG_TYPE_INTERRUPT);
    expect_value(__wrap_textio_pl011_device_initialize, config->clk_freq, 250000000U);
    expect_function_call(__wrap_textio_pl011_device_initialize);

    will_return(__wrap_config_get_uart_mcp_reassign, false);
    will_return(__wrap_idsw_get_die_id, test_die);

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_UART_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_uart.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
    mock_mcp_reassign = false;
}

TEST_FUNCTION(textio_init_uart_mcp_0_reassign_silicon, nullptr, nullptr)
{
    //! Set up expectations
    mock_mcp_reassign = true;
    DFWK_THREADX_HOST test_host = {};
    const auto test_die = (KNG_DIE_ID)0;

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    will_return(__wrap_fpfw_init_get_handle, &test_host); //! driver fmwk host handle
    expect_value(__wrap_textio_pl011_device_initialize, schedule, &(test_host.Schedule));
    expect_value(__wrap_textio_pl011_device_initialize, config->base_address, MSCP_ATU_AP_WINDOW_UART_NS_BASE_ADDR);
    expect_value(__wrap_textio_pl011_device_initialize, config->config_type, TEXTIO_PL011_CONFIG_TYPE_POLLING);
    expect_value(__wrap_textio_pl011_device_initialize, config->clk_freq, 100000000);
    expect_function_call(__wrap_textio_pl011_device_initialize);

    will_return(__wrap_config_get_uart_mcp_reassign, true);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    will_return(__wrap_idsw_get_die_id, test_die);

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, (test_die == DIE_0) ? MCP_PRIMARY : MCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_UART_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_uart.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);

    mock_mcp_reassign = false;
}

TEST_FUNCTION(textio_init_uart_mcp_0_reassign_fpga, nullptr, nullptr)
{
    //! Set up expectations
    mock_mcp_reassign = true;
    DFWK_THREADX_HOST test_host = {};
    const auto test_die = (KNG_DIE_ID)0;

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE_RVP);

    will_return(__wrap_fpfw_init_get_handle, &test_host); //! driver fmwk host handle
    expect_value(__wrap_textio_pl011_device_initialize, schedule, &(test_host.Schedule));
    expect_value(__wrap_textio_pl011_device_initialize, config->base_address, MSCP_ATU_AP_WINDOW_UART_NS_BASE_ADDR);
    expect_value(__wrap_textio_pl011_device_initialize, config->config_type, TEXTIO_PL011_CONFIG_TYPE_POLLING);
    expect_value(__wrap_textio_pl011_device_initialize, config->clk_freq, 10000000);
    expect_function_call(__wrap_textio_pl011_device_initialize);

    will_return(__wrap_config_get_uart_mcp_reassign, true);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    will_return(__wrap_idsw_get_die_id, test_die);

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, (test_die == DIE_0) ? MCP_PRIMARY : MCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_UART_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_uart.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);

    mock_mcp_reassign = false;
}

TEST_FUNCTION(textio_init_std_io, nullptr, nullptr)
{
    // Set up expectations
    textio_pl011_device_t uart_device = {};
    const auto test_die = (KNG_DIE_ID)0;

    will_return(__wrap_fpfw_init_get_handle, &uart_device); //! uart device handle
    expect_value(__wrap_textio_pl011_device_interface_initialize, device, &uart_device);
    expect_function_call(__wrap_textio_pl011_device_interface_initialize);
    expect_function_call(__wrap_stdio_textio_init);

    will_return(__wrap_idsw_get_die_id, test_die);

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_STDIO_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_std_io.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
}
