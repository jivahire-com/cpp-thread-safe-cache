/**
 * @file icc_rmss_d2d_mbox_cli.c
 *
 */

/*------------- Includes -----------------*/
#include "icc_hsp_mbox_cli.h"

#include <DfwkStatus.h>    // for DFWK_SUCCESS
#include <FpFwCli.h>       // for FpFwCliPrint, FPFW_CLI_STATUS
#include <FpFwUtils.h>     // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <fpfw_icc_base.h> // for fpfw_icc_base_recv_req_t
#include <fpfw_status.h>   // for fpfw_status_t
#include <hsp_firmware_headers.h>
#include <icc_platform_defines.h> // for D2D_MBOX_FIFO_DEPTH, HSP_...
#include <silibs_mcp_top_regs.h>  // for MCP_TOP_MCP2HSP_MAILBOX_AD...
#include <silibs_scp_top_regs.h>  // for SCP_TOP_SCP2HSP_MAILBOX_AD...
#include <stdbool.h>              // for false, bool, true
#include <stdint.h>               // for uint32_t, uint8_t
#include <stdlib.h>               // for atoi, NULL, size_t
#include <string.h>               // for memset

//*-- Symbolic Constant Macros (defines) --*/
#define MAX_ICC_MAILBOXES_INST  2
#define MAX_ICC_MAILBOX_REGS    8
#define HSP_MBOX_TEST_PAYLOAD_1 0xAAAAAAAAUL
#define HSP_MBOX_TEST_PAYLOAD_2 0xBBBBBBBBUL
#define HSP_MBOX_TEST_PAYLOAD_3 0xCCCCCCCCUL

/*-------------- Typedefs ----------------*/
/**
 * @brief  ICC mailbox context for displaying mbx registers
 *
 */
typedef struct _icc_mbx_ctx_t
{
    uint32_t mbx_base_address;
    uint8_t mbx_name[16];
} icc_mbx_ctx_t, *p_icc_mbx_ctx_t;

/*-- Declarations (Statics and globals) --*/

/**
 * @brief  List of ICC mailbox instances
 *
 */
static icc_mbx_ctx_t s_mbx_types[MAX_ICC_MAILBOXES_INST] = {
    {SCP_TOP_SCP2HSP_MAILBOX_ADDRESS, "SCP2HSP"},
    {MCP_TOP_MCP2HSP_MAILBOX_ADDRESS, "MCP2HSP"},
};

/**
 * @brief  List of ICC mailbox register names
 *
 */
static const char* mbx_reg_names[MAX_ICC_MAILBOX_REGS] = {
    "S2H_CTRL",
    "S2H_INSTS",
    "S2H_FIFO_PUSH",
    "S2H_FIFO_POP",
    "H2S_CTRL",
    "H2S_INSTS",
    "H2S_FIFO_PUSH",
    "H2S_FIFO_POP",
};

//! externs populated by icc_cli_init
const char* current_core_str = "SCP";
fpfw_icc_base_ctx_t* icc_base_hsp_mbx_ctx = NULL;

//! hsp mbox message buffers for send/recv/echo
static kng_hsp_mailbox_msg hsp_recv_msg;
static kng_hsp_mailbox_msg hsp_send_msg;
static kng_hsp_mailbox_msg hsp_echo_msg = {
    .header.cmd = HSP_MAILBOX_CMD_TEST_ECHO_REQ, //! dedicated command code for echo test
    .header.seq = 0,
    .header.context = 0,
    .header.flags = 0,
};

//! icc base send, recv & send/recv params for hsp mbox
static fpfw_icc_base_send_recv_req_t hsp_send_recv_params;
static fpfw_icc_base_send_req_t hsp_send_params;
static fpfw_icc_base_recv_req_t hsp_recv_params;

//! test flags to prevent overlapping of send/recv operations
static bool is_echo_test_active = false;
static bool is_send_test_active = false;
static bool is_recv_test_active = false;

/*------------- Functions ----------------*/
FPFW_CLI_STATUS display_mbx_list(int argc, const char** argv)
{
    FPFW_UNUSED(argv);
    if (argc != 1)
    {
        return CLI_ERROR;
    }

    FpFwCliPrint("ICC Mailbox instances:\n");
    for (uint32_t i = 0; i < MAX_ICC_MAILBOXES_INST; i++)
    {
        FpFwCliPrint("  %d: %s (Address: 0x%08x)\n", i, s_mbx_types[i].mbx_name, s_mbx_types[i].mbx_base_address);
    }
    return CLI_SUCCESS;
}

FPFW_CLI_STATUS display_mbx_register_status(int argc, const char** argv)
{
    FPFW_UNUSED(argv);
    if (argc != 2)
    {
        FpFwCliPrint("ERROR! Insufficient Args\n");
        return CLI_ERROR;
    }
    int inst_id = atoi(argv[1]);
    if (inst_id < 0 || inst_id >= MAX_ICC_MAILBOXES_INST)
    {
        FpFwCliPrint("ERROR! Invalid Arg\n");
        return CLI_ERROR;
    }

    uint32_t* mbx_address = (uint32_t*)s_mbx_types[inst_id].mbx_base_address; // NO LINT
    FpFwCliPrint("Mailbox instance %d: %s (Address: 0x%08x)\n",
                 inst_id,
                 s_mbx_types[inst_id].mbx_name,
                 s_mbx_types[inst_id].mbx_base_address);
    FpFwCliPrint("| Register       | Address(hex) | Value(hex) |\n");
    FpFwCliPrint("|----------------|--------------|------------|\n");
    for (uint32_t i = 0; i < MAX_ICC_MAILBOX_REGS; i++)
    {
        uint32_t reg_value = *(mbx_address + i);
        FpFwCliPrint("| %-14s | 0x%08x   | 0x%08x |\n", mbx_reg_names[i], (uint32_t)(mbx_address + i), reg_value); // NO LINT
    }
    return CLI_SUCCESS;
}

FPFW_CLI_STATUS set_mbx_reg_val(int argc, const char** argv)
{
    uint32_t old_value = 0;
    uint32_t new_value = 0;

    FPFW_UNUSED(argv);
    if (argc != 4)
    {
        FpFwCliPrint("ERROR! Insufficient Args\n");
        FpFwCliPrint("Usage: mbx_reg_set <inst_id (SCP=0, MCP=1)> <reg_id(0 to 7)> <val(uint32_t)>\n");
        return CLI_ERROR;
    }
    int inst_id = atoi(argv[1]);
    int reg_id = atoi(argv[2]);
    int value = atoi(argv[3]);
    if (inst_id < 0 || inst_id >= MAX_ICC_MAILBOXES_INST || reg_id < 0 || reg_id >= MAX_ICC_MAILBOX_REGS)
    {
        FpFwCliPrint("ERROR! Invalid Arg\n");
        FpFwCliPrint("Usage: mbx_reg_set <inst_id (SCP=0, MCP=1)> <reg_id(0 to 7)> <val(uint32_t)>\n");
        return CLI_ERROR;
    }

    uint32_t* mbx_address = (uint32_t*)s_mbx_types[inst_id].mbx_base_address; // NO LINT
    uint32_t* reg_address = mbx_address + reg_id;
    old_value = *reg_address;
    *reg_address = value;
    new_value = *reg_address;
    FpFwCliPrint("Mailbox inst %d:\tReg %s\tAddress: 0x%08x\tOld Val: 0x%x\tNew Val: 0x%x\n",
                 inst_id,
                 mbx_reg_names[reg_id],
                 (uint32_t)reg_address, // NO LINT
                 old_value,
                 new_value);
    return CLI_SUCCESS;
}

void my_icc_base_send_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{

    fpfw_icc_base_send_recv_req_t* req_params = (fpfw_icc_base_send_recv_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[ECHO TEST]   %s->HSP Recv Failed: Status[0x%x]\n", current_core_str, status);
    }
    else
    {
        uint32_t* recv_payload = (uint32_t*)req_params->recv_entry.payload_buffer; // NOLINT
        //! verify success, output status
        FpFwCliPrint("[ECHO TEST]   %s->HSP Recv Complete: Status[0x%x] ReceivedBytes[%d] CmdCode[0x%x] "
                     "Payload[0x%x 0x%x "
                     "0x%x 0x%x]\n",
                     current_core_str,
                     status,
                     output_size_bytes,
                     GET_HSP_MBOX_CMD_CODE(recv_payload[0]),
                     recv_payload[0],
                     recv_payload[1],
                     recv_payload[2],
                     recv_payload[3]);
    }
    is_echo_test_active = false;
}

FPFW_CLI_STATUS hsp_mbox_echo(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;

    //! Prevent overwriting of the payload buffer
    if (is_echo_test_active)
    {
        FpFwCliPrint("Echo cmd: Test already active, please wait for completion\n");
        return cli_status;
    }

    if (argc != 5)
    {
        FpFwCliPrint("Echo cmd: Insufficient Payload Args, Using default values\n");
        hsp_echo_msg.as_uint32[1] = HSP_MBOX_TEST_PAYLOAD_1;
        hsp_echo_msg.as_uint32[2] = HSP_MBOX_TEST_PAYLOAD_2;
        hsp_echo_msg.as_uint32[3] = HSP_MBOX_TEST_PAYLOAD_3;
    }
    else
    {
        hsp_echo_msg.as_uint32[1] = atoi(argv[1]);
        hsp_echo_msg.as_uint32[2] = atoi(argv[2]);
        hsp_echo_msg.as_uint32[3] = atoi(argv[3]);
    }

    //! Prepare send request
    hsp_send_recv_params.payload_buffer = &hsp_echo_msg;
    hsp_send_recv_params.buffer_size = sizeof(hsp_echo_msg);
    hsp_send_recv_params.cb = my_icc_base_send_recv_complete_notify;
    hsp_send_recv_params.cb_ctx = &hsp_send_recv_params;

    //! Send the payload & wait for response
    fpfw_status_t status = fpfw_icc_base_send_recv(icc_base_hsp_mbx_ctx, &hsp_send_recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[ECHO TEST]   %s->HSP Send Failed: Status[0x%x]\n", current_core_str, status);
    }
    else
    {

        FpFwCliPrint(
            "[ECHO TEST]   %s->HSP Send Initiated: Status[0x%x] CmdCode[0x%x] Payload[0x%x 0x%x 0x%x 0x%x]\n",
            current_core_str,
            status,
            GET_HSP_MBOX_CMD_CODE(hsp_echo_msg.as_uint32[0]),
            hsp_echo_msg.as_uint32[0],
            hsp_echo_msg.as_uint32[1],
            hsp_echo_msg.as_uint32[2],
            hsp_echo_msg.as_uint32[3]);
        //! Status is success, Set the flag to indicate the test is active
        is_echo_test_active = true;
        cli_status = CLI_SUCCESS;
    }
    return cli_status;
}

void my_icc_base_send_complete_notify(void* context, fpfw_status_t status)
{
    fpfw_icc_base_send_req_t* req_params = (fpfw_icc_base_send_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[SEND TEST]   %s->HSP Send Failed: Status[0x%x] Internal Status[0x%x]\n",
                     current_core_str,
                     status,
                     req_params->send_req.Output.Status);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[SEND TEST]   %s->HSP Send Complete: Status[0x%x] CmdCode[0x%x] Payload[0x%x 0x%x "
                     "0x%x 0x%x]\n",
                     current_core_str,
                     status,
                     GET_HSP_MBOX_CMD_CODE(hsp_send_msg.as_uint32[0]),
                     hsp_send_msg.as_uint32[0],
                     hsp_send_msg.as_uint32[1],
                     hsp_send_msg.as_uint32[2],
                     hsp_send_msg.as_uint32[3]);
    }
    is_send_test_active = false;
}

FPFW_CLI_STATUS hsp_mbox_send(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;

    //! Prevent overwriting of the send payload buffer
    if (is_send_test_active)
    {
        FpFwCliPrint("Send cmd: Test already active, please wait for completion\n");
        return cli_status;
    }

    if (argc != 5)
    {
        FpFwCliPrint("Send cmd: Insufficient Payload Args, Using default values\n");
        hsp_send_msg.as_uint32[0] = SET_HSP_MAILBOX_HEADER_ASUNIT32(HSP_MAILBOX_CMD_TEST_ECHO_REQ, 0, 0);
        hsp_send_msg.as_uint32[1] = HSP_MBOX_TEST_PAYLOAD_1;
        hsp_send_msg.as_uint32[2] = HSP_MBOX_TEST_PAYLOAD_2;
        hsp_send_msg.as_uint32[3] = HSP_MBOX_TEST_PAYLOAD_3;
    }
    else
    {
        hsp_send_msg.as_uint32[0] = atoi(argv[1]);
        hsp_send_msg.as_uint32[1] = atoi(argv[2]);
        hsp_send_msg.as_uint32[2] = atoi(argv[3]);
        hsp_send_msg.as_uint32[3] = atoi(argv[4]);
    }

    //! Prepare send request
    hsp_send_params.payload_buffer = &hsp_send_msg;
    hsp_send_params.buffer_size = sizeof(hsp_send_msg);
    hsp_send_params.cb = my_icc_base_send_complete_notify;
    hsp_send_params.cb_ctx = &hsp_send_params;

    //! Send the payload & wait for response
    fpfw_status_t status = fpfw_icc_base_send(icc_base_hsp_mbx_ctx, &hsp_send_params);

    //! print status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[SEND TEST]   %s->HSP Send Failed: Status[0x%x]\n", current_core_str, status);
    }
    else
    {
        FpFwCliPrint(
            "[SEND TEST]   %s->HSP Send Initiated: Status[0x%x] CmdCode[0x%x] Payload[0x%x 0x%x 0x%x 0x%x]\n",
            current_core_str,
            status,
            GET_HSP_MBOX_CMD_CODE(hsp_send_msg.as_uint32[0]),
            hsp_send_msg.as_uint32[0],
            hsp_send_msg.as_uint32[1],
            hsp_send_msg.as_uint32[2],
            hsp_send_msg.as_uint32[3]);
        //! Status is success, Set the flag to indicate the test is active
        is_send_test_active = true;
        cli_status = CLI_SUCCESS;
    }
    return cli_status;
}

void my_icc_base_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    fpfw_icc_base_recv_req_t* req_params = (fpfw_icc_base_recv_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[RECV TEST]   %s->HSP Recv Failed: Status[0x%x] CmdCode[0x%x]\n",
                     current_core_str,
                     status,
                     req_params->recv_cmd_code);
    }
    else
    {
        uint32_t* recv_payload_buffer = (uint32_t*)req_params->payload_buffer; // NOLINT
        //! verify success, output status
        FpFwCliPrint("[RECV TEST]   %s->HSP Recv Complete: Status[0x%x] ReceivedBytes[%d] CmdCode[0x%x] "
                     "Payload[0x%x 0x%x "
                     "0x%x 0x%x]\n",
                     current_core_str,
                     status,
                     output_size_bytes,
                     req_params->recv_cmd_code,
                     recv_payload_buffer[0],
                     recv_payload_buffer[1],
                     recv_payload_buffer[2],
                     recv_payload_buffer[3]);
    }
    is_recv_test_active = false;
}

FPFW_CLI_STATUS hsp_mbox_recv(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;

    if (is_recv_test_active)
    {
        FpFwCliPrint("Recv cmd: Test already active, please wait for completion\n");
        return cli_status;
    }

    if (argc < 2)
    {
        FpFwCliPrint("Recv cmd: Insufficient Args, Command Code Required\n");
        return cli_status;
    }
    uint32_t recv_cmd_code = atoi(argv[1]);

    //! @todo Check if the cmd code is valid, check from platform specific header file

    //! Prepare recv request
    memset(&hsp_recv_msg, 0, sizeof(hsp_recv_msg));
    hsp_recv_params.payload_buffer = &hsp_recv_msg;
    hsp_recv_params.buffer_size = sizeof(hsp_recv_msg);
    hsp_recv_params.recv_cmd_code = recv_cmd_code;
    hsp_recv_params.cb = my_icc_base_recv_complete_notify;
    hsp_recv_params.cb_ctx = &hsp_recv_params;

    //! Register for recv thru icc base
    fpfw_status_t status = fpfw_icc_base_recv(icc_base_hsp_mbx_ctx, &hsp_recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[RECV TEST]   %s->HSP Recv Failed: Status[0x%x] CmdCode[0x%x]\n", current_core_str, status, recv_cmd_code);
    }
    else
    {
        FpFwCliPrint("[RECV TEST]   %s->HSP Recv Initiated: Status[0x%x] CmdCode[0x%x]\n", current_core_str, status, recv_cmd_code);
        //! Status is success, Set the flag to indicate the test is active
        is_recv_test_active = true;
        cli_status = CLI_SUCCESS;
    }

    return cli_status;
}