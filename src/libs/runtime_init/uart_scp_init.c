/**
 * @file uart_scp_init.c
 * Instantiates baremetal, polling based, UART for the SCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <silibs_scp_top_regs.h>
#include <uart.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(uart_bm)
{
    UartInit(SCP_TOP_SCP_UART_ADDRESS);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}