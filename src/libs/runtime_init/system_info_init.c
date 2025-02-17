/**
 * @file system_info_init.c
 * Instantiates system info for the MCP & SCP
 */

/*------------- Includes -----------------*/
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fpfw_init.h>     // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <system_info.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(sysinfo, FPFW_INIT_DEPENDENCIES("icc_hspmbx", "hw_ver"))
{
    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    system_info_init(icc_ctx);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
