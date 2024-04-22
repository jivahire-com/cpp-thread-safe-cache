//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_collector.cpp
 * Event Trace Collector test
 */

/*------------- Includes -----------------*/

extern "C" {
#include <DfwkClient.h>
#include <DfwkHost.h>
#include <FpFwUtils.h>
#include <error_handler.h>
#include <textio_pl011.h> // for textio_pl011_device_t, textio_pl011...
#include <textio_pl011_i.h>
#include <tx_api.h>     // for TX_SUCCESS, TX_NO_ACTIVATE, tx_time...
#include <uart_pl011.h> // for uart_pl011_intr_mask_clear, uart_pl...
}

#include <CMockaWrapper.h>
#include <cstddef>
#include <cstdint>
#include <cstring>

/*-- Symbolic Constant Macros (defines) --*/

#define REGISTER_BLOCK_SIZE (0xFFF)
#define BUFFER_SIZE_BYTES   (0xF)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// Blocks of memory to either mock address space or test specific buffers
static uint8_t s_test_uart_memory[REGISTER_BLOCK_SIZE] = {};
static uint8_t s_test_buffer[BUFFER_SIZE_BYTES] = {};

// pl011 specific objects
// clang-format off
static textio_pl011_config_t s_pl011_config = {
    .base_address = (uintptr_t)s_test_uart_memory,
    .interrupt    = 0,
    .baud_rate    = 460800,
    .clk_freq     = 24000000,
    .wlen         = UART_PL011_WLEN_8,
    .stop_bits    = UART_PL011_STOP_BITS_1,
    .parity       = UART_PL011_PARITY_NONE,
};
static textio_pl011_device_t s_test_dev = {};
static textio_pl011_interface_t s_test_interface = {};
// clang-format on

// DFWK schedule
static DFWK_SCHEDULE s_dfwk_schedule = {};

// Persistent async requests
static fpfw_text_io_async_read_request_t s_async_read_req = {};
static fpfw_text_io_async_write_request_t s_async_write_req = {};

/*------------- Functions ----------------*/

static void request_schedule_run(PDFWK_SCHEDULE pSchedule, void* pContext)
{
    FPFW_UNUSED(pContext);
    DfwkScheduleRun(pSchedule);
}

static void async_cb(PDFWK_ASYNC_REQUEST_HEADER req, void* context)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(req);
    function_called();
}

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    DfwkScheduleInitialize(&s_dfwk_schedule, request_schedule_run, nullptr);

    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    textio_pl011_device_initialize(&s_test_dev, &s_pl011_config, &s_dfwk_schedule);

    textio_pl011_device_interface_initialize(&s_test_dev, &s_test_interface);
    assert_int_equal(DfwkClientInterfaceOpen(&s_test_interface.header), DFWK_SUCCESS);

    memset(&s_async_read_req, 0, sizeof(s_async_read_req));
    memset(&s_async_write_req, 0, sizeof(s_async_read_req));

    s_test_buffer[BUFFER_SIZE_BYTES - 1] = '\n';

    // Keep cmocka happy
    return 0;
}

/**
 *
 * MOCKS
 *
 */
extern "C" {

uint16_t __wrap_uart_pl011_get_intr_status(uintptr_t base_addr)
{
    FPFW_UNUSED(base_addr);
    return mock_type(uint16_t);
}

size_t __wrap_uart_pl011_read(uintptr_t base_addr, char* data, size_t size)
{
    FPFW_UNUSED(base_addr);
    FPFW_UNUSED(data);
    FPFW_UNUSED(size);
    return mock_type(size_t);
}

bool __wrap_uart_pl011_rx_fifo_is_empty(uintptr_t base_addr)
{
    FPFW_UNUSED(base_addr);
    return mock_type(bool);
}

uint8_t __wrap_uart_pl011_read_byte(uintptr_t base_addr)
{
    FPFW_UNUSED(base_addr);
    return 0;
}

bool __wrap_uart_pl011_tx_fifo_is_full(uintptr_t base_addr)
{
    FPFW_UNUSED(base_addr);
    return mock_type(bool);
}

void __wrap_uart_pl011_write_byte(uintptr_t base_addr, uint8_t value)
{
    FPFW_UNUSED(base_addr);
    FPFW_UNUSED(value);
}
}
/**
 *
 * TESTS
 *
 */

TEST_FUNCTION(textio_pl011_device_initialize_nominal, NULL, NULL)
{
    textio_pl011_device_t dev = {};

    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    textio_pl011_device_initialize(&dev, &s_pl011_config, &s_dfwk_schedule);
}

TEST_FUNCTION(textio_pl011_device_initialize_fail, NULL, NULL)
{
    textio_pl011_device_t dev = {};

    will_return(__wrap__txe_timer_create, TX_TIMER_ERROR);

    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
    {
        textio_pl011_device_initialize(&dev, &s_pl011_config, &s_dfwk_schedule);
    }
}

TEST_FUNCTION(textio_pl011_device_interface_initialize_nominal, NULL, NULL)
{
    textio_pl011_device_t dev = {};
    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    textio_pl011_device_initialize(&dev, &s_pl011_config, &s_dfwk_schedule);

    textio_pl011_interface_t pl011_interface = {};
    textio_pl011_device_interface_initialize(&dev, &pl011_interface);
}

TEST_FUNCTION(textio_pl011_device_interface_read_sync, test_setup, NULL)
{
    size_t read_bytes = 0;

    will_return(__wrap_uart_pl011_read, BUFFER_SIZE_BYTES);

    int32_t status =
        fpfw_text_io_try_read_request_sync(&s_test_interface.header, s_test_buffer, BUFFER_SIZE_BYTES, &read_bytes);

    assert_int_equal(status, DFWK_SUCCESS);
    assert_int_equal(read_bytes, BUFFER_SIZE_BYTES);
}

TEST_FUNCTION(textio_pl011_device_interface_write_sync, test_setup, NULL)
{
    size_t written_bytes = 0;

    // clang-format off
    int32_t status = fpfw_text_io_try_write_request_sync(
                        &s_test_interface.header,
                        s_test_buffer,
                        BUFFER_SIZE_BYTES,
                        &written_bytes
                     );
    // clang-format on

    assert_int_equal(status, DFWK_SUCCESS);
    assert_int_equal(written_bytes, BUFFER_SIZE_BYTES);
}

// Tests the async path where an async read is completed on the first attempt to read
TEST_FUNCTION(textio_pl011_device_interface_read_async_nominal, test_setup, NULL)
{
    DfwkAsyncRequestInititalize(&s_async_read_req.header, sizeof(fpfw_text_io_async_read_request_t));

    expect_function_call(async_cb);

    // setup the fifo to fill the request
    will_return_count(__wrap_uart_pl011_rx_fifo_is_empty, false, BUFFER_SIZE_BYTES);

    fpfw_text_io_read_request_async(&s_test_interface.header, &s_async_read_req.header, s_test_buffer, BUFFER_SIZE_BYTES, async_cb, nullptr);

    assert_int_equal(s_async_read_req.output.read_bytes, BUFFER_SIZE_BYTES);
}

TEST_FUNCTION(textio_pl011_device_interface_read_async_timer, test_setup, NULL)
{
    DfwkAsyncRequestInititalize(&s_async_read_req.header, sizeof(fpfw_text_io_async_read_request_t));

    // setup the fifo to not fill up the request
    will_return_count(__wrap_uart_pl011_rx_fifo_is_empty, false, BUFFER_SIZE_BYTES - 1);
    will_return(__wrap_uart_pl011_rx_fifo_is_empty, true);

    // setup threadx returns
    will_return(__wrap__txe_timer_activate, TX_SUCCESS);

    // The request is not complete yet
    fpfw_text_io_read_request_async(&s_test_interface.header, &s_async_read_req.header, s_test_buffer, BUFFER_SIZE_BYTES, async_cb, nullptr);

    assert_true(s_async_read_req.output.read_bytes < BUFFER_SIZE_BYTES);

    expect_function_call(async_cb);
    will_return(__wrap_uart_pl011_get_intr_status, UART_PL011_RX_INTR_MASK);
    will_return(__wrap_uart_pl011_rx_fifo_is_empty, false);

    // setup threadx returns
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    // trigger the timer callback
    textio_pl011_timer_cb((ULONG)&s_test_dev);

    assert_int_equal(s_async_read_req.output.read_bytes, BUFFER_SIZE_BYTES);
}

TEST_FUNCTION(textio_pl011_device_interface_read_async_no_req, test_setup, NULL)
{
    will_return(__wrap_uart_pl011_get_intr_status, UART_PL011_RX_INTR_MASK);

    // setup threadx returns
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    // trigger the timer callback
    textio_pl011_timer_cb((ULONG)&s_test_dev);
}

TEST_FUNCTION(textio_pl011_device_interface_write_async_nominal, test_setup, NULL)
{
    DfwkAsyncRequestInititalize(&s_async_write_req.header, sizeof(fpfw_text_io_async_write_request_t));

    expect_function_call(async_cb);

    // setup the fifo to fill the request
    will_return_count(__wrap_uart_pl011_tx_fifo_is_full, false, BUFFER_SIZE_BYTES);

    fpfw_text_io_write_request_async(&s_test_interface.header, &s_async_write_req.header, s_test_buffer, BUFFER_SIZE_BYTES, async_cb, nullptr);

    assert_int_equal(s_async_write_req.output.written_bytes, BUFFER_SIZE_BYTES);
}

TEST_FUNCTION(textio_pl011_device_interface_write_async_timer, test_setup, NULL)
{
    DfwkAsyncRequestInititalize(&s_async_write_req.header, sizeof(fpfw_text_io_async_read_request_t));

    // setup the fifo to not fill up the request
    will_return_count(__wrap_uart_pl011_tx_fifo_is_full, false, BUFFER_SIZE_BYTES - 1);
    will_return(__wrap_uart_pl011_tx_fifo_is_full, true);

    // setup threadx returns
    will_return(__wrap__txe_timer_activate, TX_SUCCESS);

    // The request is not complete yet
    fpfw_text_io_write_request_async(&s_test_interface.header, &s_async_write_req.header, s_test_buffer, BUFFER_SIZE_BYTES, async_cb, nullptr);

    assert_true(s_async_write_req.output.written_bytes < BUFFER_SIZE_BYTES);

    expect_function_call(async_cb);
    will_return(__wrap_uart_pl011_get_intr_status, UART_PL011_TX_INTR_MASK);
    will_return(__wrap_uart_pl011_tx_fifo_is_full, false);

    // setup threadx returns
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    // trigger the timer callback
    textio_pl011_timer_cb((ULONG)&s_test_dev);

    assert_int_equal(s_async_write_req.output.written_bytes, BUFFER_SIZE_BYTES);
}

TEST_FUNCTION(textio_pl011_device_interface_write_async_no_req, test_setup, NULL)
{
    will_return(__wrap_uart_pl011_get_intr_status, UART_PL011_TX_INTR_MASK);

    // setup threadx returns
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    // trigger the timer callback
    textio_pl011_timer_cb((ULONG)&s_test_dev);
}