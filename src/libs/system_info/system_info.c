//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file system_info.c
 * Implements system info APIs
 */

/*------------- Includes -----------------*/
#include <assert.h>
#include <fpfw_icc_base.h>        // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#include <idsw.h>                 // for idsw_get_platform_sdv
#include <idsw_kng.h>             // for idsw_get_platform_sdv
#include <kingsgate_hsp_mailbox_commands.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <stdint.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bool is_hsp_present = false;
static bool is_warm_start = false;
static bool is_runtime = false;
static uint8_t board_id = 0xFF;
static uint8_t soc_position = 0xFF;
static uint8_t board_rework_id = 0xFF;
static fpfw_icc_base_ctx_t* icc_ctx = NULL;
static hsp_security_state_t security_state = HSP_SECURITY_STATE_UNKNOWN;
static uint8_t reset_reason = HSP_FIRMWARE_RESET_REASON_UNDEFINED;
static bool mission_mode = false;
static bool cli_enable = false;
static bool watchdog_enable = false;
static uint8_t bmc_profile = 0x0;

/*------------- Functions ----------------*/
static void hsp_send_recv_security_state_msg(uint16_t req_msg, uint16_t rsp_msg)
{
    size_t recv_msg_size_bytes = 0;
    kng_hsp_mailbox_msg msg = {
        .header.cmd = req_msg,
    };

    //! Send the message to HSP & get response, blocking call
    fpfw_status_t icc_status =
        fpfw_icc_base_send_recv_sync(icc_ctx, &msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);

    //! Verify sync return status & response message
    assert(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
    assert(recv_msg_size_bytes > 0);
    assert(msg.header.cmd == rsp_msg);

    bmc_profile = msg.policy_status_rsp.policy_status.profile;
    security_state = msg.policy_status_rsp.policy_status.security_state;
    mission_mode = (msg.policy_status_rsp.policy_status.mission_mode == 1) ? true : false;
    cli_enable = (msg.policy_status_rsp.policy_status.cli_enable == 1) ? true : false;
    watchdog_enable = (msg.policy_status_rsp.policy_status.watchdog_enable == 1) ? true : false;
}

uint8_t system_info_get_board_id()
{
    /*
     * If this is called before system_info_init, read the board_id from
     * HSP boot metadata here itself.
     */
    if (board_id == 0xFF)
    {
        HSP_BOOT_METADATA boot_meta_data = {0};
        boot_meta_data.AsUint32 = MMIO_READ32(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS);
        board_id = (uint8_t)(boot_meta_data.BoardId);
    }

    return board_id;
}

uint8_t system_info_get_soc_position()
{
    /*
     * If this is called before system_info_init, read the soc_position from
     * HSP boot metadata here itself.
     */
    if (soc_position == 0xFF)
    {
        HSP_BOOT_METADATA boot_meta_data = {0};
        boot_meta_data.AsUint32 = MMIO_READ32(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS);
        soc_position = (uint8_t)(boot_meta_data.SocPos);
    }

    return soc_position;
}

uint8_t system_info_get_board_rework_id()
{
    /*
     * If this is called before system_info_init, read the soc_position from
     * HSP boot metadata here itself.
     */
    if (board_rework_id == 0xFF)
    {
        HSP_BOOT_METADATA boot_meta_data = {0};
        boot_meta_data.AsUint32 = MMIO_READ32(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS);
        board_rework_id = (uint8_t)(boot_meta_data.BoardIdRework);
    }

    return board_rework_id;
}
bool system_info_is_hsp_present()
{
    return is_hsp_present;
}

bool system_info_is_warm_start()
{
    return is_warm_start;
}

bool system_info_is_init_complete()
{
    return is_runtime;
}

void system_info_set_init_complete()
{
    is_runtime = true;
}

void system_info_init(fpfw_icc_base_ctx_t* icc_base_ctx)
{
    icc_ctx = icc_base_ctx;

    HSP_BOOT_METADATA boot_meta_data = {0};
    boot_meta_data.AsUint32 = MMIO_READ32(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS);

    if (boot_meta_data.MetadataVersion == 0x1)
    {
        is_hsp_present = true;

        reset_reason = boot_meta_data.ResetReason;

        switch (reset_reason)
        {
        case HSP_FIRMWARE_RESET_REASON_CRASH:
        case HSP_FIRMWARE_RESET_REASON_UPDATE:
        case HSP_FIRMWARE_RESET_REASON_WARM_RESET:
            is_warm_start = true;
            break;
        }
    }
    else
    {
        reset_reason = HSP_FIRMWARE_RESET_REASON_UNDEFINED;
        assert(false);
    }

    if (is_hsp_present && icc_ctx != NULL)
    {
        hsp_send_recv_security_state_msg(HSP_MAILBOX_CMD_GET_SECURITY_STATE_REQ, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    }
    else
    {
        cli_enable = true;
    }

    // BoardId
    board_id = (uint8_t)(boot_meta_data.BoardId);

    // ReWorkID
    board_rework_id = (uint8_t)(boot_meta_data.BoardIdRework);

    // SocPos
    soc_position = (boot_meta_data.SocPos);
}

KNG_PLAT_ID system_info_get_platform(void)
{
    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();
    return platform_id;
}

hsp_security_state_t system_info_get_security_state()
{
    return security_state;
}

bool system_info_get_mission_mode(void)
{
    return mission_mode;
}

bool system_info_get_cli_enable(void)
{
    return cli_enable;
}

bool system_info_get_watchdog_disable_allowed(void)
{
    return watchdog_enable ? false : true;
}

uint8_t system_info_get_reset_reason(void)
{
    return reset_reason;
}

uint8_t system_info_get_bmc_profile(void)
{
    return bmc_profile;
}
