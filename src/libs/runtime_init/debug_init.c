/**
 * @file debug_init.c
 * Initialize the debug library.
 */

/*------------- Includes -----------------*/

#include <debug.h>
#include <fpfw_init.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(debug, FPFW_INIT_DEPENDENCIES("uart_bm"))
{
    debug_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
