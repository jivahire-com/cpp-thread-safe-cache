/**
 * @file boot_status_init.c
 * Instantiates sending boot status mesg to HSP for the MCP & SCP
 */

/*------------- Includes -----------------*/
#include <boot_status.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fpfw_init.h>     // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(boot_stat, FPFW_INIT_DEPENDENCIES("icc_hspmbx", "hw_ver", "sysinfo"))
{
    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    boot_status_init(icc_ctx);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
