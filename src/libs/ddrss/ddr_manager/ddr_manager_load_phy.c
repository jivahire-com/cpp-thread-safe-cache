//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_load_phy.c
 * This file contains
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <bug_check.h> // for BUG_ASSERT_PARAM, BUG_ASSERT
#include <ddr_manager_events.h>
#include <ddr_manager_i.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send, fpfw_icc_base...
#include <fpfw_init.h>     // for fpfw_init_get_handle
#include <hsp_firmware_headers.h>
#include <idsw_kng.h> // for KNG_PLAT_ID, PLATFORM_RVP_EVT_SILICON
#include <mscp_exp_rmss_memory_map.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/**
 *  Check if platform supports PHY binary loading
 *
 *  @return
 *      true if platform supports PHY binary loading, false otherwise
 */
bool platform_supports_phy_bin_loading()
{
    // True if platform is PLATFORM_FPGA, PLATFORM_FPGA_LARGE, PLATFORM_FPGA_LARGE_RVP, PLATFORM_RVP_EVT_SILICON

    int platform = idsw_get_platform_sdv();
    return (platform == PLATFORM_FPGA_LARGE || platform == PLATFORM_FPGA_LARGE_RVP || platform == PLATFORM_RVP_EVT_SILICON);
}

/**
 *  Send HSP Mailbox cmd to load PHY binaries stored in flash in a single slot
 *  @param
 *      icc_hspmbx_ctx - Pointer to icc base context structure
 *
 *  @return
 *      none
 */
void hsp_send_recv_load_fw_ddr_phy_req(fpfw_icc_base_ctx_t* icc_ctx)
{
    if (icc_ctx == NULL)
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_ICC_NULL_POINTER, ET_NOPARAM);
        DDR_LOG_CRIT("Invalid icc_ctx: NULL pointer provided.");
        return;
    }

    if (!platform_supports_phy_bin_loading())
    {
        // does not support, skip it.
        DDR_LOG_WARN("PHY load bin is skipped\n");
        return;
    }

    if (system_info_is_hsp_present())
    {
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SEND_REQUEST_PHY_BIN);
        size_t recv_msg_size_bytes = 0;
        kng_hsp_cmd_load_fw_mailbox_msg msg = {
            .load_fw_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
            .load_fw_req.id = HSP_FIRMWARE_ID_DDR_PHY,
            .load_fw_req.address = SCP_EXP_DDR_PHY_DATA_BASE,
            .load_fw_req.size = SCP_EXP_DDR_PHY_DATA_SIZE,
        };

        fpfw_status_t icc_status =
            fpfw_icc_base_send_recv_sync(icc_ctx, &msg, sizeof(kng_hsp_cmd_load_fw_mailbox_msg), &recv_msg_size_bytes);

        //! Verify sync return status & response message
        BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
        BUG_ASSERT(recv_msg_size_bytes > 0);
        BUG_ASSERT(msg.load_fw_req.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_RSP);
        BUG_ASSERT(msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);

        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_PHY_BIN_LOAD_COMPLETE);
    }
}