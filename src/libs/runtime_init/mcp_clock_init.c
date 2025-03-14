/**
 * @file mcp_clock_init.c
 * Instantiates PIK clock for MCP
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
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
    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);

    for (int clk_dev_id = PIK_DEV_ID_MCP_CORECLK; clk_dev_id < PIK_DEV_ID_MCP_MAX; clk_dev_id++)
    {
        /* Turn on MCP clocks to desired frequency */
        status = pik_clock_power_transition(clk_dev_id, MOD_PD_STATE_ON);

        FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
