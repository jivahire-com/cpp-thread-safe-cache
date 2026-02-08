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
#include <bug_check.h>          // for BUG_ASSERT
#include <fpfw_init.h>          // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_result_t
#include <fpfw_status.h>        // for fpfw_status_t
#include <rtosmon_service.h>    // rtosmon_service_init

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(rtosmon, FPFW_INIT_DEPENDENCIES("etc", "systick_upd"))
{
    BUG_ASSERT(rtosmon_service_init() == FPFW_STATUS_SUCCESS);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
