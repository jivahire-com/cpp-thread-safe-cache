//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file debug_init.c
 * Initialize the debug library.
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <cli_mem.h>
#include <debug.h>     // for debug_init
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <gtimer_prodfw.h>
#include <stddef.h> // for NULL
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/
#define US_PER_S (1000 * 1000)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(debug, FPFW_INIT_DEPENDENCIES("std_io"))
{
    debug_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(debug_print, FPFW_INIT_DEPENDENCIES("std_io", "gtimer_stg_2", "sysinfo"))
{
    fpfw_debug_print_config_t config = {
        // Set the default debug print level based on whether CLI is enabled or not
        .default_level = system_info_get_cli_enable() ? FPFW_DEBUG_PRINT_LEVEL_INFO : FPFW_DEBUG_PRINT_LEVEL_WARNING,
        .timestamp_us_cb = gtimer_get_timestamp_us,
    };
    DbgPrintInit(&config);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(mem_cli, FPFW_INIT_DEPENDENCIES("cli"))
{
    cli_mem_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}