/**
 * @file debug_init.c
 * Initialize the debug library.
 */

/*------------- Includes -----------------*/

#include <debug.h>      // for debug_init
#include <fpfw_init.h>  // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <stddef.h>     // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(debug, FPFW_INIT_DEPENDENCIES("std_io"))
{
    debug_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
