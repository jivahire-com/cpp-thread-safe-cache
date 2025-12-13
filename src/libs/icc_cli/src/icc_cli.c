//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc commands for hsp mbx.c and mhu transports
 */

/*------------- Includes -----------------*/
#include "icc_hsp_mbox_cli.h" // for display_mbx_list, display_mbx_register_status, set_mbx_reg_val, hsp_mbox_echo, hsp_mbox_send, hsp_mbox_recv
#include "icc_large_fifo_mbox_cli.h"
#include "icc_mhu_cli_i.h"
#include "icc_rmss_d2d_mbox_cli.h" // for d2d_mbox_send, d2d_mbox_recv, d2d_sync_test, d2d_mbox_echo

#include <DfwkStatus.h>     // for DFWK_SUCCESS
#include <FpFwCli.h>        // for FpFwCliPrint, FPFW_CLI_STATUS
#include <FpFwLinkedList.h> // for NULL_LIST_ENTRY
#include <FpFwUtils.h>      // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <bug_check.h>      // for BUG_ASSERT_PARAM
#include <fpfw_icc_base.h>  // for fpfw_icc_base_recv_req_t
#include <icc_cli.h>        // for ICC_CLI_HSP_MBX, icc_cli_i...
#include <idhw.h>           // for idhw_is_single_die_boot_en
#include <idsw.h>           // for idsw_get_platform_sdv,
#include <idsw_kng.h>       // for PLATFORM_FPGA_LARGE
#include <stdint.h>         // for uint32_t, uint8_t
#include <stdlib.h>         // for atoi, NULL, size_t
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

//! Static string table mapping core type to its string
static const char* core_type_strings[] = {
    [CPU_MCP] = "MCP",
    [CPU_SCP] = "SCP",
};

//! top level table of icc cli context
static icc_cli_init_params_t* icc_cli_ctx = NULL;

/**
 * @brief List of transport interface type strings
 *
 */
const char* icc_cli_interface_type_strings[ICC_CLI_MAX_TRANSPORT_TYPE] = {
    [ICC_CLI_HSP_MBX] = "ICC_CLI_HSP_MBX",
    [ICC_CLI_SDM_FIFO_MBX] = "ICC_CLI_SDM_FIFO_MBX",
    [ICC_CLI_CDED_FIFO_MBX] = "ICC_CLI_CDED_FIFO_MBX",
    [ICC_CLI_D2D_MBX] = "ICC_CLI_D2D_MBX",
    [ICC_CLI_MSCP_MHU] = "ICC_CLI_MSCP_MHU",
    [ICC_CLI_AP_NS_MHU] = "ICC_CLI_AP_NS_MHU",
    [ICC_CLI_AP_S_MHU] = "ICC_CLI_AP_S_MHU",
    [ICC_CLI_AP_RT_MHU] = "ICC_CLI_AP_RT_MHU",
    [ICC_CLI_AP_RL_MHU] = "ICC_CLI_AP_RL_MHU",
    [ICC_CLI_D2D_MHU] = "ICC_CLI_D2D_MHU",
};

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND s_icc_hsp_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_hspmbx", "mbx_list", display_mbx_list, "show the mbx instances", "mbx_list"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "mbx_reg_show", display_mbx_register_status, "show mbx register status", "mbx_reg_show <inst_id, SCP=0, MCP=1>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "mbx_reg_set", set_mbx_reg_val, "Sets mbx register", "mbx_reg_set <inst_id (SCP=0, MCP=1)> <reg_id(0 to 7)> <val(uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "echo", hsp_mbox_echo, "HSP echo msg", "echo <(uint32_t)..(uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "send", hsp_mbox_send, "mbx msg to HSP", "send <(uint32_t)..(uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "recv", hsp_mbox_recv, "recv msg from HSP", "recv <(cmd code)>"},
};

static FPFW_CLI_COMMAND s_icc_d2d_rmss_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_d2dmbx", "send", d2d_mbox_send, "Sends a d2d msg", "send <(uint32_t)..(uint32_t) upto 16 words>"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "recv", d2d_mbox_recv, "Recv d2d msg from remote ", "recv <(cmd code)>"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "sync", d2d_sync_test, "Write sync data into shared sram, sync", "sync <(local addr) (remote addr) (uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "echo_client", d2d_mbox_echo_client, "mbx msg send & recv (Async)", "echo_client <(uint32_t) .. upto 16 words)> or `echo_client`"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "echo_serv", d2d_mbox_echo_server, "recv msg from remote & echo back", "echo_serv"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "echo_client_sync", d2d_mbox_echo_client_sync, "echo_client, sync", "echo_client <(uint32_t) .. upto 16 words)> or `echo_client`"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "echo_serv_sync", d2d_mbox_echo_server_sync, "d2d_mbox_echo_server,sync", "echo_serv"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "test_reset", d2d_reset_test, "Resets active test", "test_reset <id>"},
};

static FPFW_CLI_COMMAND s_icc_mhu_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_mhu", "list", mhu_list_indices, "Lists the MHU indices", "list"},
    {NULL_LIST_ENTRY, "icc_mhu", "recv", mhu_recv, "Recv a msg via MHU", "recv <idx> <cmd>"},
    {NULL_LIST_ENTRY, "icc_mhu", "send", mhu_send, "Sends a msg via MHU", "send <idx> <cmd> <size> <data0>..<data2>"},
    {NULL_LIST_ENTRY, "icc_mhu", "clear", mhu_clear, "Resets active test", "clear"},
    {NULL_LIST_ENTRY, "icc_mhu", "props", mhu_props, "Lists each channels properties", "props"},
};

static FPFW_CLI_COMMAND s_icc_sdm_large_fifo_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_send", large_fifo_mbox_send, "Recv msg from SDM", "sdm_send <(cmd)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_recv", large_fifo_mbox_recv, "Send msg to SDM", "sdm_recv <(cmd)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_echo", large_fifo_mbox_echo, "Echo msg to SDM", "sdm_echo <(cmd)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_loopback", large_fifo_mbox_loopback, "loopback msg to SDM", "sdm_loopback"},
};

static FPFW_CLI_COMMAND s_icc_cded_large_fifo_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_largembx", "cded_send", large_fifo_mbox_send, "Recv msg from CDED", "cded_send <(cmd)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "cded_recv", large_fifo_mbox_recv, "Send msg to CDED", "cded_recv <(cmd)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "cded_echo", large_fifo_mbox_echo, "Echo to CDED", "cded_echo <(cmd)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "cded_loopback", large_fifo_mbox_loopback, "loopback msg to CDED", "cded_loopback"},
};

/*------------- Functions ----------------*/

PLACED_CODE void icc_cli_init(icc_cli_init_params_t* params)
{
    BUG_ASSERT_PARAM(params != NULL, params, 0);
    icc_cli_ctx = params;

    //! Set the current die id
    current_die_id = icc_cli_ctx->setup_info.current.die_id;

    //! Set the current core string
    current_core_str = core_type_strings[icc_cli_ctx->setup_info.current.core_id];

    //! open interface for all the supported transports
    for (icc_cli_interface_type i = ICC_CLI_HSP_MBX; i < ICC_CLI_MAX_TRANSPORT_TYPE; i++)
    {
        if (icc_cli_ctx->icc_base_ctx[i] != NULL)
        {
            FpFwCliPrint("[ICC CLI] Initializing ICC transport: %s\n", icc_cli_interface_type_strings[i]);
            if (i == ICC_CLI_HSP_MBX)
            {
                //! fetch icc base ctx for hsp mbox from init
                icc_base_hsp_mbx_ctx = (fpfw_icc_base_ctx_t*)icc_cli_ctx->icc_base_ctx[ICC_CLI_HSP_MBX];
                //! register the icc_hspmbx commands
                FpFwCliRegisterTable(s_icc_hsp_mbx_cmd_list, FPFW_ARRAY_SIZE(s_icc_hsp_mbx_cmd_list));
            }
            else if ((i == ICC_CLI_D2D_MBX) && (icc_cli_ctx->setup_info.current.platform_is_multi_die) &&
                     (icc_cli_ctx->setup_info.current.core_id == CPU_SCP))
            {
                FpFwCliPrint("[ICC CLI] SCP Dual die, enabling D2D commands\n");
                //! fetch icc base ctx for d2d mbox from init
                icc_base_rmss_d2d_mbx_ctx = (fpfw_icc_base_ctx_t*)icc_cli_ctx->icc_base_ctx[ICC_CLI_D2D_MBX];
                //! register the icc_d2dmbx commands
                FpFwCliRegisterTable(s_icc_d2d_rmss_mbx_cmd_list, FPFW_ARRAY_SIZE(s_icc_d2d_rmss_mbx_cmd_list));
            }
            else if (i == ICC_CLI_SDM_FIFO_MBX)
            {
                FpFwCliPrint("[ICC CLI] SCP<-->SDM registering mailbox commands\n");
                icc_base_sdm_mbx_ctx = (fpfw_icc_base_ctx_t*)icc_cli_ctx->icc_base_ctx[ICC_CLI_SDM_FIFO_MBX];
                FpFwCliRegisterTable(s_icc_sdm_large_fifo_mbx_cmd_list, FPFW_ARRAY_SIZE(s_icc_sdm_large_fifo_mbx_cmd_list));
            }
            else if (i == ICC_CLI_CDED_FIFO_MBX)
            {
                FpFwCliPrint("[ICC CLI] SCP<-->CDED registering mailbox commands\n");
                icc_base_cded_mbx_ctx = (fpfw_icc_base_ctx_t*)icc_cli_ctx->icc_base_ctx[ICC_CLI_CDED_FIFO_MBX];
                FpFwCliRegisterTable(s_icc_cded_large_fifo_mbx_cmd_list,
                                     FPFW_ARRAY_SIZE(s_icc_cded_large_fifo_mbx_cmd_list));
            }
        }
    }

    FpFwCliRegisterTable(s_icc_mhu_cmd_list, FPFW_ARRAY_SIZE(s_icc_mhu_cmd_list));
}

PLACED_CODE icc_cli_init_params_t* get_icc_cli_ctx(void)
{
    return icc_cli_ctx;
}
