//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_stdio.cpp
 * STDIO tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {

#include <FpFwUtils.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio_textio.h>
#include <textio_pl011.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// Mocks for dependencies
void __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER textio_interface)
{
    check_expected_ptr(textio_interface);
}

UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit)
{
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(inherit);
    FPFW_UNUSED(mutex_ptr);
    return TX_SUCCESS;
}

UINT __wrap__txe_mutex_get(TX_MUTEX* mutex_ptr, ULONG wait_option)
{
    check_expected_ptr(mutex_ptr);
    check_expected(wait_option);
    return TX_SUCCESS;
}

UINT __wrap__txe_mutex_put(TX_MUTEX* mutex_ptr)
{
    check_expected_ptr(mutex_ptr);
    return TX_SUCCESS;
}

void __wrap_uart_pl011_write_byte(uintptr_t base, uint8_t byte)
{
    check_expected(base);
    check_expected(byte);
}

void __wrap_uart_pl011_write_assume_ready(uintptr_t base, char* buf, unsigned len)
{
    check_expected(base);
    check_expected_ptr(buf);
    check_expected(len);
}

} // extern "C"

// Test for _write_r
TEST_FUNCTION(test_write_r_basic, NULL, NULL)
{
    // Setup fake config and interface
    static textio_pl011_config_t config = {
        .base_address = 0x1000,
        .vuart_base_address = 0,
        .is_vuart_enabled = false,
        .disable_auto_cr = false,
    };
    static textio_pl011_device_t device = {
        .config = &config,
    };
    static textio_pl011_interface_t pl011_interface = {
        .device = &device,
    };

    // Call the stdio_textio_init
    expect_any(__wrap_DfwkClientInterfaceOpen, textio_interface);
    stdio_textio_init((PDFWK_INTERFACE_HEADER)&pl011_interface);

    // Prepare test data
    unsigned char buf[] = "A\nB";
    unsigned len = sizeof(buf) - 1; // exclude null terminator

    // Expect uart writes: 'A', '\n', '\r', 'B'
    expect_value(__wrap_uart_pl011_write_byte, base, config.base_address);
    expect_value(__wrap_uart_pl011_write_byte, byte, 'A');
    expect_value(__wrap_uart_pl011_write_byte, base, config.base_address);
    expect_value(__wrap_uart_pl011_write_byte, byte, '\n');
    expect_value(__wrap_uart_pl011_write_byte, base, config.base_address);
    expect_value(__wrap_uart_pl011_write_byte, byte, '\r');
    expect_value(__wrap_uart_pl011_write_byte, base, config.base_address);
    expect_value(__wrap_uart_pl011_write_byte, byte, 'B');

    // Call function under test
    int written = _write_r(NULL, 0, buf, len);

    // Should write 3 bytes (A, \n, B), CR is extra for \n
    assert_int_equal(written, len);
}