//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file textio_init.c
 * Instantiates TextIO Interface from UART for MSCP
 */

/*------------- Includes -----------------*/

#include <DfwkThreadXHost.h>     // for PDFWK_THREADX_HOST
#include <fpfw_init.h>           // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_get...
#include <interrupts.h>          // IWYU pragma: keep
#include <silibs_mcp_top_regs.h> // IWYU pragma: keep
#include <silibs_scp_top_regs.h> // IWYU pragma: keep
#include <stddef.h>              // for NULL
#include <stdio_textio.h>        // for stdio_textio_init
#include <textio_pl011.h>        // for textio_pl011_device_t, textio_pl011_dev...
#include <uart_pl011.h>          // for UART_PL011_PARITY_NONE, UART_PL011_STOP...

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(uart, FPFW_INIT_DEPENDENCIES("dfwk", "nvic"))
{
    fpfw_init_component_id_t dfwk_id = "dfwk";
    static textio_pl011_config_t pl011_config = {
        .base_address = UART_BASE_ADDR,
        .interrupt = UART_IRQ,
        .baud_rate = 115200,
        .clk_freq = 10000000,
        .wlen = UART_PL011_WLEN_8,
        .stop_bits = UART_PL011_STOP_BITS_1,
        .parity = UART_PL011_PARITY_NONE,
        .config_type = TEXTIO_PL011_CONFIG_TYPE_INTERRUPT,
    };
    static textio_pl011_device_t pl011_device = {0};

    // get driver fwk threadx handle
    PDFWK_THREADX_HOST drvfwk = (PDFWK_THREADX_HOST)fpfw_init_get_handle(dfwk_id);

    // Initialize the pl011 uart device.
    textio_pl011_device_initialize(&pl011_device, &pl011_config, &drvfwk->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &pl011_device};
}

FPFW_INIT_COMPONENT(std_io, FPFW_INIT_DEPENDENCIES("uart"))
{

    // Pl011 TextIo Interfaces
    static textio_pl011_interface_t pl011_interface_stdio = {0};
    fpfw_init_component_id_t uart_id = "uart";

    // get uart device handle
    textio_pl011_device_t* uart_handle = (textio_pl011_device_t*)fpfw_init_get_handle(uart_id);

    // Initialize a pl011 interface for stdio_textio
    textio_pl011_device_interface_initialize(uart_handle, &pl011_interface_stdio);
    stdio_textio_init(&pl011_interface_stdio.header);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
