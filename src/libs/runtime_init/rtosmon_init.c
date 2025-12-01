//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file rtosmon_init.c
 * @brief FPFW init component for RTOSMon on SCP/MCP Cortex-M7
 * This component initializes RTOSMon via rtosmon_plugin_init(), after its
 * declared dependencies are ready.
 *
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <rtosmon_config.h> /* rtosmon_plugin_init */

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(rtosmon, FPFW_INIT_DEPENDENCIES("etc", "systick_upd"))
{
    rtosmon_plugin_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
