/**
 * @file uart_scp_init.c
 * Instantiates UART for the SCP
 */

/*------------- Includes -----------------*/
#include <DfwkHost.h>
#include <fpfw_init.h>
#include <silibs_mcp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdio_textio.h>
#include <textio_pl011.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_dfwk;

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(uart, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    // clang-format off
    static textio_pl011_config_t pl011_config = {
        .base_address = UART_BASE_ADDR,
        .interrupt    = 0,
        .baud_rate    = 460800,
        .clk_freq     = 24000000,
        .wlen         = UART_PL011_WLEN_8,
        .stop_bits    = UART_PL011_STOP_BITS_1,
        .parity       = UART_PL011_PARITY_NONE,
    };
    // clang-format on
    static textio_pl011_device_t pl011_device = {0};

    // Initialize the pl011 uart device.
    textio_pl011_device_initialize(&pl011_device, &pl011_config, _fpfw_component_dfwk.handle);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &pl011_device};
}

FPFW_INIT_COMPONENT(std_io, FPFW_INIT_DEPENDENCIES("uart"))
{

    // Pl011 TextIo Interfaces
    static textio_pl011_interface_t pl011_interface_stdio = {0};

    // Initialize a pl011 interface for stdio_textio
    textio_pl011_device_interface_initialize(_fpfw_component_uart.handle, &pl011_interface_stdio);
    stdio_textio_init(&pl011_interface_stdio.header);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
