//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file textio_pl011.c
 * This file contains the implementation for the pl011 uart driver.
 */

/*------------- Includes -----------------*/
#include <DfwkDriver.h>             // for DfwkAsyncRequestComplete, DfwkQu...
#include <DfwkHost.h>               // for DfwkDeviceInitialize
#include <FpFwAssert.h>             // for FPFW_RUNTIME_ASSERT
#include <fpfw_text_io_interface.h> // for _fpfw_text_io_async_read_request...
#include <stdbool.h>                // for false
#include <stddef.h>                 // for NULL, size_t
#include <stdint.h>                 // for uint8_t, uint16_t, uint32_t, int...
#include <textio_pl011.h>           // for textio_pl011_device_t, textio_pl...
#include <textio_pl011_i.h>         // for textio_pl011_timer_cb
#include <tx_api.h>                 // for TX_SUCCESS, TX_TIMER_ERROR, ULONG
#include <uart_pl011.h>             // for uart_pl011_intr_mask_clear, uart...

/*-- Symbolic Constant Macros (defines) --*/

#define TIMER_INIT_TICKS       (1)
#define TIMER_RESCHEDULE_TICKS (1)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

static void textio_pl011_read_rx_data(textio_pl011_device_t* device, p_fpfw_text_io_async_read_request_t request);
static void textio_pl011_write_tx_data(textio_pl011_device_t* device, p_fpfw_text_io_async_write_request_t request);

/*-------------- Functions ---------------*/

void textio_pl011_default_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    textio_pl011_device_t* device = (textio_pl011_device_t*)context;

    // Move the request to the appropriate queue.
    switch (request->RequestType)
    {
    case FPFW_TEXT_IO_REQUEST_ID_READ_ASYNC:
        DfwkQueueEnqueueRequest(&device->read_queue, request);
        break;
    case FPFW_TEXT_IO_REQUEST_ID_WRITE_ASYNC:
        DfwkQueueEnqueueRequest(&device->write_queue, request);
        break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

void textio_pl011_rx_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    textio_pl011_device_t* device = (textio_pl011_device_t*)context;

    FPFW_RUNTIME_ASSERT(request->RequestType == FPFW_TEXT_IO_REQUEST_ID_READ_ASYNC);

    p_fpfw_text_io_async_read_request_t read_request = (p_fpfw_text_io_async_read_request_t)request;
    read_request->output.read_bytes = 0;
    read_request->output.read_status = FPFW_TEXT_IO_STATUS_FAIL;
    textio_pl011_read_rx_data(device, read_request);

    if (read_request->output.read_bytes == read_request->input.byte_count)
    {
        uart_pl011_intr_mask_clear(device->config->base_address, UART_PL011_RX_TIMEOUT_INTR_MASK | UART_PL011_RX_INTR_MASK);
        read_request->output.read_status = FPFW_TEXT_IO_STATUS_SUCCESS;
        DfwkAsyncRequestComplete(request);
    }
    else
    {
        // Pend request and enable timer
        device->pending_read_request = read_request;
        uart_pl011_intr_mask_set(device->config->base_address, UART_PL011_RX_TIMEOUT_INTR_MASK | UART_PL011_RX_INTR_MASK);
        uint32_t status = tx_timer_activate(&device->polling_timer);
        FPFW_RUNTIME_ASSERT(status != TX_TIMER_ERROR);
    }
}

void textio_pl011_tx_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    textio_pl011_device_t* device = (textio_pl011_device_t*)context;

    FPFW_RUNTIME_ASSERT(request->RequestType == FPFW_TEXT_IO_REQUEST_ID_WRITE_ASYNC);

    p_fpfw_text_io_async_write_request_t write_request = (p_fpfw_text_io_async_write_request_t)request;
    write_request->output.written_bytes = 0;
    write_request->output.write_status = FPFW_TEXT_IO_STATUS_FAIL;
    textio_pl011_write_tx_data(device, write_request);

    if (write_request->output.written_bytes == write_request->input.byte_count)
    {
        uart_pl011_intr_mask_clear(device->config->base_address, UART_PL011_TX_INTR_MASK);
        write_request->output.write_status = FPFW_TEXT_IO_STATUS_SUCCESS;
        DfwkAsyncRequestComplete(request);
    }
    else
    {
        // Pend request and enable timer
        device->pending_write_request = write_request;
        uart_pl011_intr_mask_set(device->config->base_address, UART_PL011_TX_INTR_MASK);
        uint32_t status = tx_timer_activate(&device->polling_timer);
        FPFW_RUNTIME_ASSERT(status != TX_TIMER_ERROR);
    }
}

int32_t textio_pl011_request_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER request)
{
    textio_pl011_interface_t* pl011_interface = (textio_pl011_interface_t*)request->OwningInterface;
    const textio_pl011_config_t* config = pl011_interface->device->config;

    switch (request->RequestType)
    {
    case FPFW_TEXT_IO_REQUEST_ID_READ_SYNC: {
        // clang-format off
            p_fpfw_text_io_sync_read_request_t read_request = (p_fpfw_text_io_sync_read_request_t)request;
            size_t bytes_read = uart_pl011_read(
                                    config->base_address,
                                    read_request->input.buffer,
                                    read_request->input.byte_count
                                );
            read_request->output.read_bytes = bytes_read;
        // clang-format on
        break;
    }
    case FPFW_TEXT_IO_REQUEST_ID_WRITE_SYNC: {
        p_fpfw_text_io_sync_write_request_t write_request = (p_fpfw_text_io_sync_write_request_t)request;
        uint8_t* byte_ptr = (uint8_t*)write_request->input.buffer;

        // Ensure the output written bytes is 0
        write_request->output.written_bytes = 0;

        while (write_request->output.written_bytes < write_request->input.byte_count)
        {
            uart_pl011_write_byte(config->base_address, *byte_ptr);
            if (*byte_ptr == '\n')
            {
                uart_pl011_write_byte(config->base_address, '\r');
            }
            byte_ptr++;
            write_request->output.written_bytes++;
        }
        break;
    }
    default:
        // No other types of requests are supported
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    return DFWK_SUCCESS;
}

void textio_pl011_device_initialize(textio_pl011_device_t* device, const textio_pl011_config_t* config, DFWK_SCHEDULE* schedule)
{
    // Init driver default queue.
    DfwkQueueInitialize(&device->default_queue, &device->header, textio_pl011_default_dispatch_async, device, DfwkQueueType_ImmediateDispatch);
    DfwkQueueInitialize(&device->read_queue, &device->header, textio_pl011_rx_dispatch_async, device, DfwkQueueType_SerializedDispatch);
    DfwkQueueInitialize(&device->write_queue, &device->header, textio_pl011_tx_dispatch_async, device, DfwkQueueType_SerializedDispatch);

    device->config = config;

    const uart_pl011_cfg_t hw_config = {.baud_rate = config->baud_rate,
                                        .clk_freq = config->clk_freq,
                                        .wlen = config->wlen,
                                        .stop_bits = config->stop_bits,
                                        .parity = config->parity};

    uart_pl011_init(config->base_address, &hw_config);
    uart_pl011_intr_rx_fifo_level_sel(config->base_address, UART_PL011_FIFO_LEVEL_1_2);
    uart_pl011_intr_tx_fifo_level_sel(config->base_address, UART_PL011_FIFO_LEVEL_1_2);

    uint16_t intr_mask = 0;
    uart_pl011_intr_mask_set(config->base_address, intr_mask);

    // Replace TX_TIMER with proper interrupt: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1741558
    // clang-format off
    UINT status = tx_timer_create(
        &device->polling_timer,
        "textio_pl011_uart_timer",
        textio_pl011_timer_cb,
        (ULONG)device,
        TIMER_INIT_TICKS,
        TIMER_RESCHEDULE_TICKS,
        TX_NO_ACTIVATE
    );
    // clang-format on

    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);

    DfwkDeviceInitialize(&device->header, schedule);
}

void textio_pl011_device_interface_initialize(textio_pl011_device_t* device, textio_pl011_interface_t* pl011_interface)
{
    DfwkInterfaceInitialize(&pl011_interface->header, &device->header, &device->default_queue, textio_pl011_request_dispatch_sync);
    pl011_interface->device = device;
}

static void textio_pl011_copy_fifo_to_request(textio_pl011_device_t* device)
{
    // If CLI is enabled expect there to always be a pending request
    if (device->pending_read_request != NULL)
    {
        // Copy data from the hw fifo to the pending request if there is one.
        textio_pl011_read_rx_data(device, device->pending_read_request);
        if (device->pending_read_request->output.read_bytes == device->pending_read_request->input.byte_count)
        {
            uart_pl011_intr_mask_clear(device->config->base_address, UART_PL011_RX_TIMEOUT_INTR_MASK | UART_PL011_RX_INTR_MASK);
            device->pending_read_request->output.read_status = FPFW_TEXT_IO_STATUS_SUCCESS;
            DfwkAsyncRequestComplete(&device->pending_read_request->header);
            device->pending_read_request = NULL;
        }
    }
    else
    {
        uart_pl011_intr_mask_clear(device->config->base_address, UART_PL011_RX_TIMEOUT_INTR_MASK | UART_PL011_RX_INTR_MASK);
    }
}

static void textio_pl011_copy_request_to_fifo(textio_pl011_device_t* device)
{
    if (device->pending_write_request != NULL)
    {
        textio_pl011_write_tx_data(device, device->pending_write_request);

        if (device->pending_write_request->output.written_bytes == device->pending_write_request->input.byte_count)
        {
            uart_pl011_intr_mask_clear(device->config->base_address, UART_PL011_TX_INTR_MASK);
            device->pending_write_request->output.write_status = FPFW_TEXT_IO_STATUS_SUCCESS;
            DfwkAsyncRequestComplete(&device->pending_write_request->header);
            device->pending_write_request = NULL;
        }
    }
    else
    {
        uart_pl011_intr_mask_clear(device->config->base_address, UART_PL011_TX_INTR_MASK);
    }
}

void textio_pl011_isr(void* context)
{
    textio_pl011_device_t* device = (textio_pl011_device_t*)context;

    uint16_t intr_status = uart_pl011_get_intr_status(device->config->base_address);

    // Handle the overrun status
    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1749333

    // If the rx timeout or the rx int bit is set, copy bytes from the rx fifo to the pending request.
    if ((intr_status & UART_PL011_RX_TIMEOUT_INTR_MASK) != 0 || (intr_status & UART_PL011_RX_INTR_MASK) != 0)
    {
        textio_pl011_copy_fifo_to_request(device);
    }

    // If the tx intr bit is set, copy data from the pending tx request to the tx fifo.
    if ((intr_status & UART_PL011_TX_INTR_MASK) != 0)
    {
        textio_pl011_copy_request_to_fifo(device);
    }

    // If both requests pending requests are NULL, stop the timer.
    // TODO: Delete this when implementing the proper NVIC interrupt
    if (device->pending_read_request == NULL && device->pending_write_request == NULL)
    {
        FPFW_RUNTIME_ASSERT(TX_SUCCESS == tx_timer_deactivate(&device->polling_timer));
    }
}

static void textio_pl011_read_rx_data(textio_pl011_device_t* device, p_fpfw_text_io_async_read_request_t request)
{
    uint8_t* byte_ptr = (uint8_t*)request->input.buffer + request->output.read_bytes;

    while ((request->output.read_bytes < request->input.byte_count) &&
           !uart_pl011_rx_fifo_is_empty(device->config->base_address))
    {
        *byte_ptr = uart_pl011_read_byte(device->config->base_address);
        byte_ptr++;
        request->output.read_bytes++;
    }
}

static void textio_pl011_write_tx_data(textio_pl011_device_t* device, p_fpfw_text_io_async_write_request_t request)
{
    uint8_t* byte_ptr = (uint8_t*)request->input.buffer + request->output.written_bytes;
    while ((request->output.written_bytes < request->input.byte_count) &&
           !uart_pl011_tx_fifo_is_full(device->config->base_address))
    {
        uart_pl011_write_byte(device->config->base_address, *byte_ptr);
        if (*byte_ptr == '\n')
        {
            uart_pl011_write_byte(device->config->base_address, '\r');
        }
        byte_ptr++;
        request->output.written_bytes++;
    }
}

void textio_pl011_timer_cb(ULONG input)
{
    textio_pl011_isr((void*)input);
}
