/**
 * @file boot_status_init.c
 * Instantiates sending boot status mesg to HSP for the MCP & SCP
 */

/*------------- Includes -----------------*/
#include <accelerator_ip.h> // for accel_setup_boot_status_code
#include <accelip_id.h>     // for NUM_VALID_ACCEL_ID, ACCEL_ID_SDM
#include <boot_status.h>    // for boot_status_init
#include <fpfw_icc_base.h>  // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fpfw_init.h>      // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <idsw_kng.h>       // for idsw_get_cpu_type, CPU_SCP

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(boot_stat, FPFW_INIT_DEPENDENCIES("icc_hspmbx", "hw_ver", "sysinfo"))
{
    static boot_status_icc_ctx_t boot_status_icc_ctx = {};

    //! Populate the boot status ctx, common for both SCP & MCP
    boot_status_icc_ctx.hsp_mbx_ctx = fpfw_init_get_handle("icc_hspmbx");

    //! Call the init API
    boot_status_init(&boot_status_icc_ctx);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

#ifdef SCP_RUNTIME_INIT
FPFW_INIT_COMPONENT(bs_accel, FPFW_INIT_DEPENDENCIES("icc_sdm_mbx", "icc_cded_mbx"))
{
    uint32_t status;

    for (ACCEL_ID accel_type = ACCEL_ID_SDM; accel_type < NUM_VALID_ACCEL_ID; accel_type++)
    {
        status = accel_setup_boot_status_code(accel_type);
        if (status != FPFW_INIT_STATUS_SUCCESS)
        {
            break;
        }
    }

    return (fpfw_init_result_t){status, NULL};
}
#endif // SCP_RUNTIME_INIT
