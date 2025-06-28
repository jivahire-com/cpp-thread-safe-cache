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
#include <fpfw_init.h>       // for fpfw_init_result_t, fpfw_init_component_t
#include <idsw.h>
#include <idsw_kng.h>
#include <textio_pl011.h> // for textio_pl011_config_t, textio_pl011_dev...
#include <uart_pl011.h>   // for UART_PL011_PARITY_NONE, UART_PL011_STOP...

/*-- Symbolic Constant Macros (defines) --*/
#define UART_BAUD_RATE (115200)
#define UART_CLK_FREQ  (10000000)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_uart;
extern fpfw_init_component_t _fpfw_component_std_io;

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
    assert_int_equal(config->base_address, UART_BASE_ADDR);
    assert_int_equal(config->vuart_base_address, VUART_BASE_ADDR);
    assert_int_equal(config->interrupt, UART_IRQ);
    assert_int_equal(config->vuart_interrupt, VUART_IRQ);
    assert_int_equal(config->baud_rate, UART_BAUD_RATE);
    check_expected(config->clk_freq);
    assert_int_equal(config->wlen, UART_PL011_WLEN_8);
    assert_int_equal(config->stop_bits, UART_PL011_STOP_BITS_1);
    assert_int_equal(config->parity, UART_PL011_PARITY_NONE);
    assert_int_equal(config->config_type, TEXTIO_PL011_CONFIG_TYPE_INTERRUPT);
    assert_true(config->is_vuart_enabled); // Verify virtual UART is enabled
    function_called();
}

void __wrap_textio_pl011_device_interface_initialize(textio_pl011_device_t* device, textio_pl011_interface_t* pl011_interface)
{
    check_expected_ptr(device);
    assert_non_null(pl011_interface);
    function_called();
}

//
// Tests
//

TEST_FUNCTION(textio_init_uart, nullptr, nullptr)
{
    //! Set up expectations
    DFWK_THREADX_HOST test_host = {};

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    will_return(__wrap_fpfw_init_get_handle, &test_host); //! driver fmwk host handle
    expect_value(__wrap_textio_pl011_device_initialize, schedule, &(test_host.Schedule));
    expect_value(__wrap_textio_pl011_device_initialize, config->clk_freq, 250000000U);
    expect_function_call(__wrap_textio_pl011_device_initialize);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_uart.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(textio_init_std_io, nullptr, nullptr)
{
    // Set up expectations
    textio_pl011_device_t uart_device = {};
    will_return(__wrap_fpfw_init_get_handle, &uart_device); //! uart device handle
    expect_value(__wrap_textio_pl011_device_interface_initialize, device, &uart_device);
    expect_function_call(__wrap_textio_pl011_device_interface_initialize);
    expect_function_call(stdio_textio_init);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_std_io.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
}
