//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc commands for hsp mbx.c
 */

/*------------- Includes -----------------*/
#include <DfwkStatus.h>                   // for DFWK_SUCCESS
#include <FpFwAssert.h>                   // for FPFW_RUNTIME_ASSERT
#include <FpFwCli.h>                      // for FpFwCliPrint, FPFW_CLI_STATUS
#include <FpFwLinkedList.h>               // for NULL_LIST_ENTRY
#include <FpFwUtils.h>                    // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <fpfw_icc_base.h>                // for fpfw_icc_base_recv_req_t
#include <fpfw_icc_dispatcher.h>          // for fpfw_icc_dispatch_table_entry
#include <fpfw_icc_transport_interface.h> // for FPFW_ICC_TRANSPORT_ASYNC_S...
#include <fpfw_status.h>                  // for fpfw_status_t
#include <icc_cli.h>                      // for ICC_CLI_HSP_MBX, icc_cli_i...
#include <silibs_mcp_top_regs.h>          // for MCP_TOP_MCP2HSP_MAILBOX_AD...
#include <silibs_scp_top_regs.h>          // for SCP_TOP_SCP2HSP_MAILBOX_AD...
#include <stdbool.h>                      // for false, bool, true
#include <stdint.h>                       // for uint32_t, uint8_t
#include <stdlib.h>                       // for atoi, NULL, size_t

/*-- Symbolic Constant Macros (defines) --*/
//! @todo Get the mailbox registers from the header files
#define MAX_ICC_MAILBOXES_INST    2
#define MAX_ICC_MAILBOX_REGS      8
#define HSP_MBX_FIFO_DEPTH        4
#define HSP_MAILBOX_TEST_CMD_CODE 0x0U
#define HSP_MBOX_TEST_PAYLOAD_1   0xAAAAAAAAUL
#define HSP_MBOX_TEST_PAYLOAD_2   0xBBBBBBBBUL
#define HSP_MBOX_TEST_PAYLOAD_3   0xCCCCCCCCUL

/*-------------- Typedefs ----------------*/
typedef struct _icc_mbx_ctx_t
{
    uint32_t mbx_base_address;
    uint8_t mbx_name[16];
} icc_mbx_ctx_t, *p_icc_mbx_ctx_t;

static icc_mbx_ctx_t s_mbx_types[MAX_ICC_MAILBOXES_INST] = {
    {SCP_TOP_SCP2HSP_MAILBOX_ADDRESS, "SCP2HSP"},
    {MCP_TOP_MCP2HSP_MAILBOX_ADDRESS, "MCP2HSP"},
};

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

static fpfw_icc_base_ctx_t* icc_base_ctx[ICC_CLI_MAX_TRANSPORT_TYPE] = {NULL};
static fpfw_icc_base_send_recv_req_t send_recv_params = {0};
static fpfw_icc_base_send_req_t send_params = {0};
static fpfw_icc_base_recv_req_t recv_params = {0};
static uint32_t payload_buffer[HSP_MBX_FIFO_DEPTH] = {0, 0, 0, 0};
static uint32_t send_payload_buffer[HSP_MBX_FIFO_DEPTH] = {0, 0, 0, 0};
static uint32_t recv_payload_buffer[HSP_MBX_FIFO_DEPTH] = {0, 0, 0, 0};
static bool is_echo_test_active = false;
static bool is_send_test_active = false;
static bool is_recv_test_active = false;

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS display_mbx_list(int argc, const char** argv);
static FPFW_CLI_STATUS display_mbx_register_status(int argc, const char** argv);
static FPFW_CLI_STATUS set_mbx_reg_val(int argc, const char** argv);
static FPFW_CLI_STATUS hsp_mbox_echo(int argc, const char** argv);
static FPFW_CLI_STATUS hsp_mbox_send(int argc, const char** argv);
static FPFW_CLI_STATUS hsp_mbox_recv(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND s_icc_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc", "mbx_list", display_mbx_list, "Displays the mailbox instances", "Usage: mbx_list (no arguments)"},
    {NULL_LIST_ENTRY, "icc", "mbx_reg_show", display_mbx_register_status, "Displays the mailbox register status", "Usage: mbx_reg_show <inst_id, SCP=0, MCP=1>"},
    {NULL_LIST_ENTRY, "icc", "mbx_reg_set", set_mbx_reg_val, "Sets the mailbox register value", "Usage: mbx_reg_set <inst_id (SCP=0, MCP=1)> <reg_id(0 to 7)> <val(uint32_t)>"},
    {NULL_LIST_ENTRY, "icc", "echo", hsp_mbox_echo, "Sends a mailbox mesg to HSP & receives one", "Usage: echo <(uint32_t) (uint32_t) (uint32_t) (uint32_t)>"},
    {NULL_LIST_ENTRY, "icc", "send", hsp_mbox_send, "Sends a mailbox mesg to HSP", "Usage: send <(uint32_t) (uint32_t) (uint32_t) (uint32_t)>"},
    {NULL_LIST_ENTRY, "icc", "recv", hsp_mbox_recv, "Expects to recv mailbox mesg from HSP", "Usage: recv <(cmd code)>"},
};

/*------------- Functions ----------------*/

void icc_cli_init(icc_cli_init_params_t* params)
{
    FPFW_RUNTIME_ASSERT(params != NULL);
    //! open interface for all the supported transports
    for (icc_cli_interface_type i = ICC_CLI_HSP_MBX; i < ICC_CLI_MAX_TRANSPORT_TYPE; i++)
    {
        if (params->icc_base_ctx[i] != NULL)
        {
            icc_base_ctx[i] = (fpfw_icc_base_ctx_t*)params->icc_base_ctx[i];
        }
    }
    //! register the icc commands
    FpFwCliRegisterTable(s_icc_cmd_list, FPFW_ARRAY_SIZE(s_icc_cmd_list));
}

static FPFW_CLI_STATUS display_mbx_list(int argc, const char** argv)
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

static FPFW_CLI_STATUS display_mbx_register_status(int argc, const char** argv)
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

static FPFW_CLI_STATUS set_mbx_reg_val(int argc, const char** argv)
{
    uint32_t old_value = 0;
    uint32_t new_value = 0;

    FPFW_UNUSED(argv);
    if (argc != 4)
    {
        FpFwCliPrint("ERROR! Insufficient Args\n");
        FpFwCliPrint("%s\n", s_icc_cmd_list[2].Usage);
        return CLI_ERROR;
    }
    int inst_id = atoi(argv[1]);
    int reg_id = atoi(argv[2]);
    int value = atoi(argv[3]);
    if (inst_id < 0 || inst_id >= MAX_ICC_MAILBOXES_INST || reg_id < 0 || reg_id >= MAX_ICC_MAILBOX_REGS)
    {
        FpFwCliPrint("ERROR! Invalid Arg\n");
        FpFwCliPrint("%s\n", s_icc_cmd_list[2].Usage);
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
        FpFwCliPrint("[ECHO TEST]   MSCP->HSP Recv Failed: Status[0x%x]\n", status);
    }
    else
    {
        uint32_t* recv_payload = (uint32_t*)req_params->recv_entry.payload_buffer; // NOLINT
        //! verify success, output status
        FpFwCliPrint(
            "[ECHO TEST]   MSCP->HSP Recv Complete: Status[0x%x] ReceivedBytes[%d] Payload[0x%x 0x%x "
            "0x%x 0x%x]\n",
            status,
            output_size_bytes,
            recv_payload[0],
            recv_payload[1],
            recv_payload[2],
            recv_payload[3]);
    }
    is_echo_test_active = false;
}

static FPFW_CLI_STATUS hsp_mbox_echo(int argc, const char** argv)
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
        payload_buffer[0] = HSP_MAILBOX_TEST_CMD_CODE;
        payload_buffer[1] = HSP_MBOX_TEST_PAYLOAD_1;
        payload_buffer[2] = HSP_MBOX_TEST_PAYLOAD_2;
        payload_buffer[3] = HSP_MBOX_TEST_PAYLOAD_3;
    }
    else
    {
        payload_buffer[0] = atoi(argv[1]);
        payload_buffer[1] = atoi(argv[2]);
        payload_buffer[2] = atoi(argv[3]);
        payload_buffer[3] = atoi(argv[4]);
    }

    //! Prepare send request
    send_recv_params.payload_buffer = payload_buffer;
    send_recv_params.buffer_size = sizeof(payload_buffer);
    send_recv_params.cb = my_icc_base_send_recv_complete_notify;
    send_recv_params.cb_ctx = &send_recv_params;

    //! Send the payload & wait for response
    fpfw_status_t status = fpfw_icc_base_send_recv(icc_base_ctx[ICC_CLI_HSP_MBX], &send_recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[ECHO TEST]   MSCP->HSP Send Failed: Status[0x%x]\n", status);
    }
    else
    {

        FpFwCliPrint("[ECHO TEST]   MSCP->HSP Send Initiated: Status[0x%x] Payload[0x%x 0x%x 0x%x 0x%x]\n",
                     status,
                     payload_buffer[0],
                     payload_buffer[1],
                     payload_buffer[2],
                     payload_buffer[3]);
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
        FpFwCliPrint("[ECHO TEST]   MSCP->HSP Send Failed: Status[0x%x] Internal Status[0x%x]\n",
                     status,
                     req_params->send_req.Output.Status);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[ECHO TEST]   MSCP->HSP Send Complete: Status[0x%x] Payload[0x%x 0x%x "
                     "0x%x 0x%x]\n",
                     status,
                     send_payload_buffer[0],
                     send_payload_buffer[1],
                     send_payload_buffer[2],
                     send_payload_buffer[3]);
    }
    is_send_test_active = false;
}

static FPFW_CLI_STATUS hsp_mbox_send(int argc, const char** argv)
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
        send_payload_buffer[0] = HSP_MAILBOX_TEST_CMD_CODE;
        send_payload_buffer[1] = HSP_MBOX_TEST_PAYLOAD_1;
        send_payload_buffer[2] = HSP_MBOX_TEST_PAYLOAD_2;
        send_payload_buffer[3] = HSP_MBOX_TEST_PAYLOAD_3;
    }
    else
    {
        send_payload_buffer[0] = atoi(argv[1]);
        send_payload_buffer[1] = atoi(argv[2]);
        send_payload_buffer[2] = atoi(argv[3]);
        send_payload_buffer[3] = atoi(argv[4]);
    }

    //! Prepare send request
    send_params.payload_buffer = send_payload_buffer;
    send_params.buffer_size = sizeof(send_payload_buffer);
    send_params.cb = my_icc_base_send_complete_notify;
    send_params.cb_ctx = &send_params;

    //! Send the payload & wait for response
    fpfw_status_t status = fpfw_icc_base_send(icc_base_ctx[ICC_CLI_HSP_MBX], &send_params);

    //! print status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[SEND TEST]   MSCP->HSP Send Failed: Status[0x%x]\n", status);
    }
    else
    {
        FpFwCliPrint("[SEND TEST]   MSCP->HSP Send Initiated: Status[0x%x] Payload[0x%x 0x%x 0x%x 0x%x]\n",
                     status,
                     payload_buffer[0],
                     payload_buffer[1],
                     payload_buffer[2],
                     payload_buffer[3]);
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
        FpFwCliPrint("[RECV TEST]   MSCP->HSP Recv Failed: Status[0x%x] CmdCode[0x%x]\n", status, req_params->recv_cmd_code);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[RECV TEST]   MSCP->HSP Recv Complete: Status[0x%x] ReceivedBytes[%d] CmdCode[0x%x] "
                     "Payload[0x%x 0x%x "
                     "0x%x 0x%x]\n",
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

static FPFW_CLI_STATUS hsp_mbox_recv(int argc, const char** argv)
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
    recv_params.payload_buffer = recv_payload_buffer;
    recv_params.buffer_size = sizeof(recv_payload_buffer);
    recv_params.recv_cmd_code = recv_cmd_code;
    recv_params.cb = my_icc_base_recv_complete_notify;
    recv_params.cb_ctx = &recv_params;

    //! Register for recv thru icc base
    fpfw_status_t status = fpfw_icc_base_recv(icc_base_ctx[ICC_CLI_HSP_MBX], &recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[RECV TEST]   MSCP->HSP Recv Failed: Status[0x%x] CmdCode[0x%x]\n", status, recv_cmd_code);
    }
    else
    {
        FpFwCliPrint("[RECV TEST]   MSCP->HSP Recv Initiated: Status[0x%x] CmdCode[0x%x]\n", status, recv_cmd_code);
        //! Status is success, Set the flag to indicate the test is active
        is_recv_test_active = true;
        cli_status = CLI_SUCCESS;
    }

    return cli_status;
}