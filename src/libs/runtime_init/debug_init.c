/**
 * @file debug_init.c
 * Initialize the debug library.
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <debug.h>     // for debug_init
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <gtimer_prodfw.h>
#include <stddef.h> // for NULL

/*-- Symbolic Constant Macros (defines) --*/
#define US_PER_S (1000 * 1000)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
uint64_t dbgprint_counter_to_us()
{
    return US_PER_S * gtimer_prodfw_get_counter() / gtimer_prodfw_get_frequency();
}

FPFW_INIT_COMPONENT(debug, FPFW_INIT_DEPENDENCIES("std_io"))
{
    debug_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(debug_print, FPFW_INIT_DEPENDENCIES("std_io", "gtimer"))
{
    fpfw_debug_print_config_t config = {
        .default_level = FPFW_DEBUG_PRINT_LEVEL_INFO,
        .timestamp_us_cb = dbgprint_counter_to_us,
    };
    DbgPrintInit(&config);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}