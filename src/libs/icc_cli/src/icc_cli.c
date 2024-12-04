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
#include "status_decoder.h"        // for get_fpfw_status_code_string

#include <DfwkStatus.h>     // for DFWK_SUCCESS
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <FpFwCli.h>        // for FpFwCliPrint, FPFW_CLI_STATUS
#include <FpFwLinkedList.h> // for NULL_LIST_ENTRY
#include <FpFwUtils.h>      // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <fpfw_icc_base.h>  // for fpfw_icc_base_recv_req_t
#include <icc_cli.h>        // for ICC_CLI_HSP_MBX, icc_cli_i...
#include <idhw.h>           // for idhw_is_single_die_boot_en
#include <idsw.h>           // for idsw_get_platform_sdv,
#include <idsw_kng.h>       // for PLATFORM_FPGA_LARGE
#include <stdint.h>         // for uint32_t, uint8_t
#include <stdlib.h>         // for atoi, NULL, size_t

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
    {NULL_LIST_ENTRY, "icc_hspmbx", "mbx_list", display_mbx_list, "Displays the mailbox instances", "Usage: mbx_list (no arguments)"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "mbx_reg_show", display_mbx_register_status, "Displays the mailbox register status", "Usage: mbx_reg_show <inst_id, SCP=0, MCP=1>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "mbx_reg_set", set_mbx_reg_val, "Sets the mailbox register value", "Usage: mbx_reg_set <inst_id (SCP=0, MCP=1)> <reg_id(0 to 7)> <val(uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "echo", hsp_mbox_echo, "Sends a mailbox mesg to HSP & receives one, Async Operation", "Usage: echo <(uint32_t) (uint32_t) (uint32_t) (uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "send", hsp_mbox_send, "Sends a mailbox mesg to HSP, Async Operation", "Usage: send <(uint32_t) (uint32_t) (uint32_t) (uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "recv", hsp_mbox_recv, "Expects to recv mailbox mesg from HSP, Async Operation", "Usage: recv <(cmd code)>"},
};

static FPFW_CLI_COMMAND s_icc_d2d_rmss_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_d2dmbx", "send", d2d_mbox_send, "Sends a mailbox mesg to scp of remote die, Async Operation", "Usage: die_send <(uint32_t) (uint32_t) (uint32_t) (uint32_t) ... upto 16 words>"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "recv", d2d_mbox_recv, "Expects to recv mailbox mesg from scp of remote die, Async Operation", "Usage: die_recv <(cmd code)>"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "sync", d2d_sync_test, "Write to shared sram from current to remote die, Sync Blocking", "Usage: die_sync <(local addr) (remote addr) (uint32_t)>"},
    {NULL_LIST_ENTRY,
     "icc_d2dmbx",
     "echo_client",
     d2d_mbox_echo_client,
     "Sends a mailbox mesg to remote scp & receives back same content, Async Operation",
     "Usage: echo_client <(uint32_t) (uint32_t) (uint32_t) ... upto 16 words)> or simply `echo_client`"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "echo_serv", d2d_mbox_echo_server, "Receives mbox mesg from remote scp & send back the same content, Async Operation", "Usage: echo_serv"},
    {NULL_LIST_ENTRY,
     "icc_d2dmbx",
     "echo_client_sync",
     d2d_mbox_echo_client_sync,
     "Sends a mailbox mesg to remote scp & receives back same content, Blocking Operation",
     "Usage: echo_client <(uint32_t) (uint32_t) (uint32_t) ... upto 16 words)> or simply `echo_client`"},
    {NULL_LIST_ENTRY,
     "icc_d2dmbx",
     "echo_serv_sync",
     d2d_mbox_echo_server_sync,
     "Receives mbox mesg from remote scp & send back the same content, Blocking Operation",
     "Usage: echo_serv"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "test_reset", d2d_reset_test, "Resets currently active test, Only to be used for debugging", "Usage: test_reset <id>"},
};

static FPFW_CLI_COMMAND s_icc_mhu_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_mhu", "list", mhu_list_indices, "Lists the MHU indices", "Usage: list_indices (no arguments)"},
    {NULL_LIST_ENTRY, "icc_mhu", "recv", mhu_recv, "Receives a message via MHU", "Usage: recv <index> <command>"},
    {NULL_LIST_ENTRY, "icc_mhu", "send", mhu_send, "Sends message via MHU", "Usage: send <index> <command> <size> <data0> ... <data 2>"},
    {NULL_LIST_ENTRY, "icc_mhu", "clear", mhu_clear, "clears all pending cli messages", "Usage: clear"},
};

static FPFW_CLI_COMMAND s_icc_sdm_large_fifo_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_send", large_fifo_mbox_send, "Expects to recv mailbox mesg from SDM", "Usage: sdm_send <(cmd code)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_recv", large_fifo_mbox_recv, "Expects to send mailbox mesg to SDM", "Usage: sdm_recv <(cmd code)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_echo", large_fifo_mbox_echo, "Expects to send & receive mailbox mesg to SDM", "Usage: sdm_echo <(cmd code)>"},
};

static FPFW_CLI_COMMAND s_icc_cded_large_fifo_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_largembx", "cded_send", large_fifo_mbox_send, "Expects to recv mailbox mesg from CDED", "Usage: cded_send <(cmd code)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "cded_recv", large_fifo_mbox_recv, "Expects to send mailbox mesg to CDED", "Usage: cded_recv <(cmd code)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "cded_echo", large_fifo_mbox_echo, "Expects to send & receive mailbox mesg to CDED", "Usage: cded_echo <(cmd code)>"},
};

//! @todo separate this out in a generic cli utils file
static FPFW_CLI_COMMAND s_common_list[] = {
    {NULL_LIST_ENTRY, "common", "decode_err", print_error_string, "Print the error code string", "Usage: decode_err <numeric err code>"},
};
/*------------- Functions ----------------*/

void icc_cli_init(icc_cli_init_params_t* params)
{
    FPFW_RUNTIME_ASSERT(params != NULL);
    icc_cli_ctx = params;

    //! Set the current die id
    current_die_id = icc_cli_ctx->setup_info.current.die_id;

    //! Set the current core string
    current_core_str = core_type_strings[icc_cli_ctx->setup_info.current.core_id];

    //! @todo separate this out in a generic cli utils file
    FpFwCliRegisterTable(s_common_list, FPFW_ARRAY_SIZE(s_common_list));

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

icc_cli_init_params_t* get_icc_cli_ctx(void)
{
    return icc_cli_ctx;
}
