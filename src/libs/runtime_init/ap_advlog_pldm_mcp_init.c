/**
 * @file ap_advlog_pldm_mcp_init.c
 * Instantiates AP Adv Logger PLDM for MCP core
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <ap_advlog_pldm.h> // for ap_advlog_pldm_init
#include <fpfw_init.h>
#include <stdint.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(ap_advlog_pldm, FPFW_INIT_DEPENDENCIES("pldm", "debug_print"))
{
    ap_advlog_pldm_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
