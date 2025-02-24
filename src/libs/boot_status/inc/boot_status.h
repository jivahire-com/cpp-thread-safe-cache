//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file boot_status.h
 *  APIs for clients to raise boot status send requests to HSP
 */

 #pragma once

 /*--------------- Includes ---------------*/
 #include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
 
 /*-- Symbolic Constant Macros (defines) --*/
 
 /*-------------- Typedefs ----------------*/ 
 /**
  * @brief Add boot status code as applicable
  * SCP/MCP/SDM/CDED boot status codes are in the range 0x40 - 0x7F (0b01xxxxxx)
  * @todo ADD SDM/CDED specific codes here
  */
typedef enum _boot_status_codes_t
{
    //! progress codes for SCP
    BOOT_STATUS_CODE_SCP_OK = 0x40,
    BOOT_STATUS_CODE_SCP_START,
    BOOT_STATUS_CODE_SCP_COLD_BOOT,
    BOOT_STATUS_CODE_SCP_WARM_BOOT,
    BOOT_STATUS_CODE_SCP_IRQ_DISABLED,
    BOOT_STATUS_CODE_SCP_MESH_INIT_START,
    BOOT_STATUS_CODE_SCP_MESH_INIT_END,
    BOOT_STATUS_CODE_SCP_TOWER_INIT_START,
    BOOT_STATUS_CODE_SCP_TOWER_INIT_END,
    BOOT_STATUS_CODE_SCP_ACCEL_INIT_START,
    BOOT_STATUS_CODE_SCP_ACCEL_INIT_END,
    BOOT_STATUS_CODE_SCP_DDR_INIT_START,
    BOOT_STATUS_CODE_SCP_DDR_INIT_END,
    //! Error codes for SCP
    BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG,
    BOOT_STATUS_CODE_SCP_E_DDR0_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR1_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR2_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR3_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR4_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR5_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR6_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR7_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR8_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR9_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR10_TRAINING,
    BOOT_STATUS_CODE_SCP_E_DDR11_TRAINING,
    //! Keep last for SCP, do not use as post code
    BOOT_STATUS_CODE_SCP_MAX,

    //! progress codes for MCP
    BOOT_STATUS_CODE_MCP_OK = 0x60,
    BOOT_STATUS_CODE_MCP_START,
    BOOT_STATUS_CODE_MCP_COLD_BOOT,
    BOOT_STATUS_CODE_MCP_WARM_BOOT,
    BOOT_STATUS_CODE_MCP_IRQ_DISABLED,
    //! Error codes for MCP, do not use as post code
    BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG,
    //! Keep last for MCP
    BOOT_STATUS_CODE_MCP_MAX,

    //! Keep last entry in this enum, do not use as post code
    BOOT_STATUS_CODE_MAX = 0x7F,

} boot_status_code_t;
 
/*--------- Function Prototypes ----------*/
/**
 * @brief Called in runtime init to cache the icc base ctx.
 * 
 * @param icc_base_ctx hsp mbox ICC base context.
 */
void boot_status_init(fpfw_icc_base_ctx_t* icc_base_ctx);
  
/**
 * @brief Sends the boot status notification to the HSP. Raises a sync/async
 * ICC send req over HSP mailbox.
 * 
 * @param boot_status Status to send to HSP, given by boot_status_code_t
 * 
 * @details 
 * HSP needs the boot status notification to determine the next steps.
 * It uses the boot status to determine if the AP cores are up and running.
 * If HSP doesn't get the boot status, it won't allow mailbox commands from MSCP
 * like cold boot, shutdown, fw update requests.
 * Currently HSP implements no timeout for this message on their end.
 * HSP only cares about the boot status ex field. If status ex is err, it sends a gpio 
 * signal to BMC to take action.
 * 
 * @return fpfw_status_t 
 * FPFW_STATUS_INVALID_ARGS for Invalid args
 * FPFW_STATUS_INVALID_STATE, if HSP not present or ICC ctx not set
 * FPFW_STATUS_BUSY if there is a previous ongoing send request that hasn't completed yet
 * FPFW_STATUS_SUCCESS for Success or else ICC Base specific error code
 */
fpfw_status_t boot_status_notify(boot_status_code_t boot_status);
 
 