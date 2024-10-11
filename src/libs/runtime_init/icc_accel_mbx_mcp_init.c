/**
 * @file icc_accel_mbx_mcp_init.c
 * Instantiates icc base ctx for accel mailbox transport interface for MCP platform
 */

/*------------- Includes -----------------*/

#include <DfwkHost.h>                   // for DfwkDeviceInitialize
#include <DfwkThreadXHost.h>            // for PDFWK_THREADX_HOST
#include <MboxPrimitives.h>
#include <accel_intr.h>
#include <accelerator_ip.h>
#include <fpfw_icc_base.h>              // for fpfw_icc_base_ctx_t
#include <fpfw_icc_base_i.h>
#include <fpfw_init.h>                  // for FPFW_INIT_STATUS_E_INVALID_NODE
#include <fpfw_mbox_icc_transport.h>    // for ICC_MAX_ASYNC_REQ_TYPE               
#include <fpfw_timer.h>                 // for fpfw_timer_t
#include <fpfw_timer_port.h>
#include <icc_platform_defines.h>
#include <idsw.h>
#include <idsw_kng.h>   
#include <interrupts.h>
#include <sdm_ext_cfg_regs.h>  
#include <stddef.h>                     // for NULL


/*-------------- Macros ------------------*/

/*------------- Typedefs -----------------*/

#define SCP_MBX_OFFSET      (SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS + SDM_MBOX_OFFSET_AFTER_0X100000)

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/**
 * @note ICC Support added for Large FIFO Mailbox
 *
 * Similarly add config for each icc transport interface
 * A single icc base ctx for every transport interface.
 * Transport driver interface is not directly exposed to the users
 * It's recommended all inter core communication goes thru icc base service
 *
 * Steps to initialize an instance of icc base context:
 * 1. Declare the correct configs required for the specific transport
 * 2. Initialize the transport driver (Device & interface init)
 * 3. Call fpfw_icc_base_init() with icc config, this populates the ctx
 * 4. If the base init is successful, call fpfw_icc_dispatcher_start()
 * 5. Finally return the initialized icc base ctx to the user for invoking icc base APIs
 */
FPFW_INIT_COMPONENT(icc_sdm_mbx, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver"))
{

    //! This is just a placeholder for Accel <--> MCP mailbox
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
