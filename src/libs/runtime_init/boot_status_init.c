/**
 * @file boot_status_init.c
 * Instantiates sending boot status mesg to HSP for the MCP & SCP
 */

/*------------- Includes -----------------*/
#include <boot_status.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fpfw_init.h>     // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <idsw_kng.h>      // for KNG_CPU_TYPE

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(boot_stat, FPFW_INIT_DEPENDENCIES("icc_hspmbx", "icc_sdm_mbx", "icc_cded_mbx", "hw_ver", "sysinfo"))
{
    static boot_status_icc_ctx_t boot_status_icc_ctx = {};

    //! Populate the boot status ctx, common for both SCP & MCP
    boot_status_icc_ctx.hsp_mbx_ctx = fpfw_init_get_handle("icc_hspmbx");

    //! Only applicable for SCP
    if (idsw_get_cpu_type() == CPU_SCP)
    {
        static fpfw_icc_base_send_req_t send_params[BOOT_STATUS_ACCEL_MAX]; //! memory for send req for large fifo mbox
        static fpfw_icc_base_recv_req_t recv_params[BOOT_STATUS_ACCEL_MAX]; //! memory for recv req for large fifo mbox

        boot_status_icc_ctx.accel_icc_ctx[BOOT_STATUS_SDM].lfifo_mbx_ctx =
            fpfw_init_get_handle("icc_sdm_mbx");
        boot_status_icc_ctx.accel_icc_ctx[BOOT_STATUS_SDM].send_params = &send_params[BOOT_STATUS_SDM];
        boot_status_icc_ctx.accel_icc_ctx[BOOT_STATUS_SDM].recv_params = &recv_params[BOOT_STATUS_SDM];

        boot_status_icc_ctx.accel_icc_ctx[BOOT_STATUS_CDED].lfifo_mbx_ctx =
            fpfw_init_get_handle("icc_cded_mbx");
        boot_status_icc_ctx.accel_icc_ctx[BOOT_STATUS_CDED].send_params = &send_params[BOOT_STATUS_CDED];
        boot_status_icc_ctx.accel_icc_ctx[BOOT_STATUS_CDED].recv_params = &recv_params[BOOT_STATUS_CDED];
    }

    //! Call the init API
    boot_status_init(&boot_status_icc_ctx);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
