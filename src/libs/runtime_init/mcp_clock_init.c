//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file mcp_clock_init.c
 * Instantiates PIK clock for MCP
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <fpfw_init.h>
#include <pik_clock_lib.h>
#include <stdint.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(pik_clock, FPFW_INIT_DEPENDENCIES("hw_ver"))
{
    int status = pik_clock_init(0, NULL, NULL);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

    for (int clk_dev_id = PIK_DEV_ID_MCP_CORECLK; clk_dev_id < PIK_DEV_ID_MCP_MAX; clk_dev_id++)
    {
        /* Turn on MCP clocks to desired frequency */
        status = pik_clock_power_transition(clk_dev_id, MOD_PD_STATE_ON);

        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, clk_dev_id);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
