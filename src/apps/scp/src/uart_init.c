/**
 * @file uart_init.c
 * Instantiates UART
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <uart.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(uart_bm)
{
    UartInit(UART0BASE_SCP);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}