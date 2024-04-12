/**
 * @file uart_mcp_init.c
 * Instantiates baremetal, polling based, UART for the MCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <silibs_mcp_top_regs.h>
#include <uart.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(uart_bm)
{
    UartInit(MCP_TOP_MCP_UART_ADDRESS);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}