//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc commands for hsp mbx.c and mhu transports
 */

/*------------- Includes -----------------*/
#include "icc_hsp_mbox_cli.h" // for display_mbx_list, display_mbx_register_status, set_mbx_reg_val, hsp_mbox_echo, hsp_mbox_send, hsp_mbox_recv
#include "icc_large_fifo_mbox_cli.h"
#include "icc_rmss_d2d_mbox_cli.h" // for d2d_mbox_send, d2d_mbox_recv, d2d_sync_test, d2d_mbox_echo
#include "status_decoder.h"        // for get_fpfw_status_code_string

#include <DfwkStatus.h>         // for DFWK_SUCCESS
#include <FpFwAssert.h>         // for FPFW_RUNTIME_ASSERT
#include <FpFwCli.h>            // for FpFwCliPrint, FPFW_CLI_STATUS
#include <FpFwLinkedList.h>     // for NULL_LIST_ENTRY
#include <FpFwUtils.h>          // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <fpfw_icc_base.h>      // for fpfw_icc_base_recv_req_t
#include <icc_cli.h>            // for ICC_CLI_HSP_MBX, icc_cli_i...
#include <icc_mhu_trans_prim.h> // for icc_mhu primitives
#include <idhw.h>               // for idhw_is_single_die_boot_en
#include <idsw.h>               // for idsw_get_platform_sdv,
#include <idsw_kng.h>           // for PLATFORM_FPGA_LARGE
#include <stdint.h>             // for uint32_t, uint8_t
#include <stdlib.h>             // for atoi, NULL, size_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/**
 * @brief List of transport interface type strings
 *
 */
static const char* icc_cli_interface_type_strings[ICC_CLI_MAX_TRANSPORT_TYPE] = {
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

//! Static string table mapping core type to its string
static const char* core_type_strings[] = {
    [CPU_MCP] = "MCP",
    [CPU_SCP] = "SCP",
};

//! top level table of icc cli context
static icc_cli_init_params_t* icc_cli_ctx = NULL;

/*--------- Function Prototypes ----------*/

static FPFW_CLI_STATUS mhu_send(int argc, const char** argv);
static FPFW_CLI_STATUS scmi_stat(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND s_icc_hsp_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_hspmbx", "mbx_list", display_mbx_list, "Displays the mailbox instances", "Usage: mbx_list (no arguments)"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "mbx_reg_show", display_mbx_register_status, "Displays the mailbox register status", "Usage: mbx_reg_show <inst_id, SCP=0, MCP=1>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "mbx_reg_set", set_mbx_reg_val, "Sets the mailbox register value", "Usage: mbx_reg_set <inst_id (SCP=0, MCP=1)> <reg_id(0 to 7)> <val(uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "echo", hsp_mbox_echo, "Sends a mailbox mesg to HSP & receives one", "Usage: echo <(uint32_t) (uint32_t) (uint32_t) (uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "send", hsp_mbox_send, "Sends a mailbox mesg to HSP", "Usage: send <(uint32_t) (uint32_t) (uint32_t) (uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_hspmbx", "recv", hsp_mbox_recv, "Expects to recv mailbox mesg from HSP", "Usage: recv <(cmd code)>"},
};

static FPFW_CLI_COMMAND s_icc_d2d_rmss_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_d2dmbx", "send", d2d_mbox_send, "Sends a mailbox mesg to scp of remote die", "Usage: die_send <(uint32_t) (uint32_t) (uint32_t) (uint32_t) ... upto 16 words>"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "recv", d2d_mbox_recv, "Expects to recv mailbox mesg from scp of remote die", "Usage: die_recv <(cmd code)>"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "sync", d2d_sync_test, "Write to shared sram from current to remote die", "Usage: die_sync <(local addr) (remote addr) (uint32_t)>"},
    {NULL_LIST_ENTRY, "icc_d2dmbx", "echo", d2d_mbox_echo, "Sends a mailbox mesg to remote scp & receives back same content", "Usage: echo <(uint32_t) (uint32_t) (uint32_t) ... upto 16 words)>"},
};

static FPFW_CLI_COMMAND s_icc_mhu_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_mhu", "mhu_send", mhu_send, "Sends message via MHU", "Usage: mhu_send <index> <command> <size> <data0> ... <data 2>"},
    {NULL_LIST_ENTRY, "icc_mhu", "scmi_stat", scmi_stat, "Checks MHU SCMI Stat bit", "Usage: scmi_stat <index>"},
};

static FPFW_CLI_COMMAND s_icc_large_fifo_mbx_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_send", large_fifo_mbox_send, "Expects to recv mailbox mesg from SDM", "Usage: sdm_send <(cmd code)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_recv", large_fifo_mbox_recv, "Expects to send mailbox mesg to SDM", "Usage: sdm_recv <(cmd code)>"},
    {NULL_LIST_ENTRY, "icc_largembx", "sdm_echo", large_fifo_mbox_echo, "Expects to send & receive mailbox mesg to SDM", "Usage: sdm_echo <(cmd code)>"},
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
                icc_base_sdm_mbx_ctx = (fpfw_icc_base_ctx_t*)icc_cli_ctx->icc_base_ctx[ICC_CLI_SDM_FIFO_MBX];
                FpFwCliRegisterTable(s_icc_large_fifo_mbx_cmd_list, FPFW_ARRAY_SIZE(s_icc_large_fifo_mbx_cmd_list));
            }
        }
    }

    //! register the icc_mhu commands, not integrated with icc base yet
    if (icc_cli_ctx->setup_info.current.core_id == CPU_SCP)
    {
        FpFwCliRegisterTable(s_icc_mhu_cmd_list, FPFW_ARRAY_SIZE(s_icc_mhu_cmd_list));
    }
}

//! @todo move this to icc mhu cli src file when integrated with icc base
static FPFW_CLI_STATUS mhu_send(int argc, const char** argv)
{

    if (argc < 4)
    {
        FpFwCliPrint("ERROR! Insufficient Args mhu_send\n");
        FpFwCliPrint("Usage: mhu_send <index> <command> <size> <data0> ... <data 2>\n");
        FpFwCliPrint("          up to 32 bytes of data can be sent\n");
        return CLI_ERROR;
    }

    // Take the arguments
    int index = atoi(argv[1]);
    uint32_t command = atoi(argv[2]);
    size_t size = atoi(argv[3]);
    uint8_t data[32];

    if (size != 0 && argc > 4 && (size <= (size_t)(argc - 4)))
    {
        for (uint8_t count = 0; count < size; count++)
        {
            data[count] = atoi(argv[4 + count]);
        }
    }

    FpFwCliPrint("Sending data to MHU\n");
    icc_mhu_trans_send_message_idx(index, command, data, size);

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS scmi_stat(int argc, const char** argv)
{

    if (argc != 2)
    {
        FpFwCliPrint("ERROR! Insufficient Args mhu_dbs\n");
        FpFwCliPrint("Usage: scmi_stat <index>\n");

        return CLI_ERROR;
    }
    int index = atoi(argv[1]);
    icc_mhu_trans_check_scmi_status_bit(index);
    FpFwCliPrint("MHU SCMI status bit check\n");
    return CLI_SUCCESS;
}