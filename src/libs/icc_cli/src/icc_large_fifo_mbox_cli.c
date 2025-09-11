//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_large_fifo_mbox_cli.c
 *
 */

/*------------- Includes -----------------*/
#include "icc_large_fifo_mbox_cli.h"

#include "icc_cli.h"

#include <DfwkStatus.h>           // for DFWK_SUCCESS
#include <FpFwCli.h>              // for FpFwCliPrint, FPFW_CLI_STATUS
#include <FpFwUtils.h>            // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <fpfw_icc_base.h>        // for fpfw_icc_base_recv_req_t
#include <fpfw_status.h>          // for fpfw_status_t
#include <icc_platform_defines.h> // for D2D_MBOX_FIFO_DEPTH, HSP_...
#include <silibs_mcp_top_regs.h>  // for MCP_TOP_MCP2HSP_MAILBOX_AD...
#include <silibs_scp_top_regs.h>  // for SCP_TOP_SCP2HSP_MAILBOX_AD...
#include <stdbool.h>              // for false, bool, true
#include <stdint.h>               // for uint32_t, uint8_t
#include <stdlib.h>               // for atoi, NULL, size_t
#include <string.h>               // for memset
#include <utils.h>

//*-- Symbolic Constant Macros (defines) --*/

#define ICC_LARGE_FIFO_MBOX_LPBK_TRIGGER_CMD          0xF0F0
#define ICC_LARGE_FIFO_MBOX_LPBK_TRIGGER_RESPONSE_CMD 0xF0F1

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

//! externs populated by icc_cli_init
fpfw_icc_base_ctx_t* icc_base_sdm_mbx_ctx = NULL;
fpfw_icc_base_ctx_t* icc_base_cded_mbx_ctx = NULL;

//! hsp mbox message buffers for send/recv/echo
static large_fifo_cli_mailbox_msg accel_recv_msg;
static large_fifo_cli_mailbox_msg accel_send_msg;
static large_fifo_cli_mailbox_msg accel_echo_msg = {
    .hdr.cmd = 0xDEAD,
    .hdr.seq = 0,
    .hdr.context = 0,
    .hdr.flags = 0,
};
static large_fifo_cli_mailbox_msg loopback_send_msg;
static large_fifo_cli_mailbox_msg loopback_recv_msg;

//! icc base send, recv & send/recv params for hsp mbox
static fpfw_icc_base_send_recv_req_t accel_send_recv_params;
static fpfw_icc_base_send_req_t accel_send_params;
static fpfw_icc_base_recv_req_t accel_recv_params;
static fpfw_icc_base_send_req_t loopback_send_params;
static fpfw_icc_base_recv_req_t loopback_recv_params;

//! test flags to prevent overlapping of send/recv operations
static bool is_accel_echo_test_active = false;
static bool is_accel_send_test_active = false;
static bool is_accel_recv_test_active = false;
static bool is_accel_loopback_test_active = false;

// Variables to allow iterative loopback testing
static uint32_t loopback_rx_cntr = 0;
static uint32_t loopback_test_cntr = 0;
static fpfw_icc_base_ctx_t* curr_icc_ctx = NULL;

// Function forward declarations
static void my_icc_large_fifo_loopback_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status);
static void my_icc_large_fifo_loopback_send_complete_notify(void* context, fpfw_status_t status);

/*------------- Functions ----------------*/

static PLACED_CODE void my_icc_large_fifo_send_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{

    fpfw_icc_base_send_recv_req_t* req_params = (fpfw_icc_base_send_recv_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[ECHO TEST] Recv Failed: Status[0x%x]\n", status);
    }
    else
    {
        large_fifo_cli_mailbox_msg* recv_msg = (void*)req_params->recv_entry.payload_buffer; // NOLINT
        //! verify success, output status
        FpFwCliPrint("[ECHO TEST] Recv Complete: Status[0x%x] ReceivedBytes[%d] CmdCode[0x%x] "
                     "Payload[0x%x 0x%x "
                     "0x%x 0x%x]\n",
                     status,
                     output_size_bytes,
                     recv_msg->hdr.cmd,
                     recv_msg->data[0],
                     recv_msg->data[1],
                     recv_msg->data[2],
                     recv_msg->data[3]);
    }
    is_accel_echo_test_active = false;
}

PLACED_CODE FPFW_CLI_STATUS large_fifo_mbox_echo(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    fpfw_icc_base_ctx_t* icc_base;
    unsigned int i;

    FPFW_UNUSED(argc);

    //! Prevent overwriting of the payload buffer
    if (is_accel_echo_test_active)
    {
        FpFwCliPrint("Echo cmd: Test ongoing\n");
        return cli_status;
    }

    if (argc < 2)
    {
        FpFwCliPrint("Echo cmd: invalid Args\n");
        return cli_status;
    }

    if (strcmp(argv[0], "sdm_echo") == 0)
    {
        icc_base = icc_base_sdm_mbx_ctx;
    }
    else if (strcmp(argv[0], "cded_echo") == 0)
    {
        icc_base = icc_base_cded_mbx_ctx;
    }
    else
    {
        FpFwCliPrint("Invalid CLI %s\n", argv[0]);
        return cli_status;
    }

    accel_echo_msg.hdr.cmd = atoi(argv[1]);
    for (i = 0; i < LARGE_FIFO_MBOX_MAX_PAYLOAD_WORD; i++)
    {
        accel_echo_msg.data[i] = i;
    }

    //! Prepare send request
    accel_send_recv_params.payload_buffer = &accel_echo_msg;
    accel_send_recv_params.buffer_size = sizeof(accel_echo_msg);
    accel_send_recv_params.cb = my_icc_large_fifo_send_recv_complete_notify;
    accel_send_recv_params.cb_ctx = &accel_send_recv_params;

    //! Send the payload & wait for response
    fpfw_status_t status = fpfw_icc_base_send_recv(icc_base, &accel_send_recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[ECHO TEST] Send Failed: Status[0x%x]\n", status);
    }
    else
    {

        FpFwCliPrint("[ECHO TEST] Send Initiated: Status[0x%x] CmdCode[0x%x] Payload[0x%x 0x%x 0x%x 0x%x]\n",
                     status,
                     accel_echo_msg.hdr.cmd,
                     accel_echo_msg.data[0],
                     accel_echo_msg.data[1],
                     accel_echo_msg.data[2],
                     accel_echo_msg.data[3]);
        //! Status is success, Set the flag to indicate the test is active
        is_accel_echo_test_active = true;
        cli_status = CLI_SUCCESS;
    }

    return cli_status;
}

static PLACED_CODE void my_icc_large_fifo_send_complete_notify(void* context, fpfw_status_t status)
{
    fpfw_icc_base_send_req_t* req_params = (fpfw_icc_base_send_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[SEND TEST] Send Failed: Status[0x%x] Internal Status[0x%x]\n",
                     status,
                     req_params->send_req.Output.Status);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[SEND TEST] Send Complete: Status[0x%x] CmdCode[0x%x] Payload[0x%x 0x%x "
                     "0x%x 0x%x]\n",
                     status,
                     accel_send_msg.hdr.cmd,
                     accel_send_msg.data[0],
                     accel_send_msg.data[1],
                     accel_send_msg.data[2],
                     accel_send_msg.data[3]);
    }
    is_accel_send_test_active = false;
}

PLACED_CODE FPFW_CLI_STATUS large_fifo_mbox_send(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    fpfw_icc_base_ctx_t* icc_base;
    unsigned int i;

    FPFW_UNUSED(argc);

    //! Prevent overwriting of the send payload buffer
    if (is_accel_send_test_active)
    {
        FpFwCliPrint("Send cmd: Test ongoing\n");
        return cli_status;
    }

    if (argc < 2)
    {
        FpFwCliPrint("Send cmd: Invalid Args\n");
        return cli_status;
    }

    if (strcmp(argv[0], "sdm_send") == 0)
    {
        icc_base = icc_base_sdm_mbx_ctx;
    }
    else if (strcmp(argv[0], "cded_send") == 0)
    {
        icc_base = icc_base_cded_mbx_ctx;
    }
    else
    {
        FpFwCliPrint("Invalid CLI %s\n", argv[0]);
        return cli_status;
    }

    accel_send_msg.hdr.cmd = atoi(argv[1]);
    for (i = 0; i < LARGE_FIFO_MBOX_MAX_PAYLOAD_WORD; i++)
    {
        accel_send_msg.data[i] = i;
    }

    //! Prepare send request
    accel_send_params.payload_buffer = &accel_send_msg;
    accel_send_params.buffer_size = sizeof(accel_send_msg);
    accel_send_params.cb = my_icc_large_fifo_send_complete_notify;
    accel_send_params.cb_ctx = &accel_send_params;

    //! Send the payload & wait for response
    fpfw_status_t status = fpfw_icc_base_send(icc_base, &accel_send_params);

    //! print status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[SEND TEST] Send Failed: Status[0x%x]\n", status);
    }
    else
    {
        FpFwCliPrint("[SEND TEST] Send Initiated: Status[0x%x] CmdCode[0x%x] Payload[0x%x 0x%x 0x%x 0x%x]\n",
                     status,
                     accel_send_msg.hdr.cmd,
                     accel_send_msg.data[0],
                     accel_send_msg.data[1],
                     accel_send_msg.data[2],
                     accel_send_msg.data[3]);
        //! Status is success, Set the flag to indicate the test is active
        is_accel_send_test_active = true;
        cli_status = CLI_SUCCESS;
    }

    return cli_status;
}

static PLACED_CODE void my_icc_large_fifo_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    fpfw_icc_base_recv_req_t* req_params = (fpfw_icc_base_recv_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[RECV TEST] Recv Failed: Status[0x%x] CmdCode[0x%x]\n", status, req_params->recv_cmd_code);
    }
    else
    {
        uint32_t* recv_payload_buffer = (uint32_t*)req_params->payload_buffer; // NOLINT
        //! verify success, output status
        FpFwCliPrint("[RECV TEST] Recv Complete: Status[0x%x] ReceivedBytes[%d] CmdCode[0x%x] "
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
    is_accel_recv_test_active = false;
}

PLACED_CODE FPFW_CLI_STATUS large_fifo_mbox_recv(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    fpfw_icc_base_ctx_t* icc_base;

    if (is_accel_recv_test_active)
    {
        FpFwCliPrint("Recv cmd: Test ongoing\n");
        return cli_status;
    }

    if (argc < 2)
    {
        FpFwCliPrint("Recv cmd: Invalid Args\n");
        return cli_status;
    }

    if (strcmp(argv[0], "sdm_recv") == 0)
    {
        icc_base = icc_base_sdm_mbx_ctx;
    }
    else if (strcmp(argv[0], "cded_recv") == 0)
    {
        icc_base = icc_base_cded_mbx_ctx;
    }
    else
    {
        FpFwCliPrint("Invalid CLI %s\n", argv[0]);
        return cli_status;
    }

    uint32_t recv_cmd_code = atoi(argv[1]);

    //! Prepare recv request
    memset(&accel_recv_msg, 0, sizeof(accel_recv_msg));
    accel_recv_params.payload_buffer = &accel_recv_msg;
    accel_recv_params.buffer_size = sizeof(accel_recv_msg);
    accel_recv_params.recv_cmd_code = recv_cmd_code;
    accel_recv_params.cb = my_icc_large_fifo_recv_complete_notify;
    accel_recv_params.cb_ctx = &accel_recv_params;

    //! Register for recv thru icc base
    fpfw_status_t status = fpfw_icc_base_recv(icc_base, &accel_recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[RECV TEST] Recv Failed: Status[0x%x] CmdCode[0x%x]\n", status, recv_cmd_code);
    }
    else
    {
        FpFwCliPrint("[RECV TEST] Recv Initiated: Status[0x%x] CmdCode[0x%x]\n", status, recv_cmd_code);
        //! Status is success, Set the flag to indicate the test is active
        is_accel_recv_test_active = true;
        cli_status = CLI_SUCCESS;
    }

    return cli_status;
}

static PLACED_CODE fpfw_status_t my_icc_large_fifo_loopback_send_recv()
{
    //! Prepare send request
    loopback_send_params.payload_buffer = &loopback_send_msg;
    loopback_send_params.buffer_size = sizeof(loopback_send_msg);
    loopback_send_params.cb = my_icc_large_fifo_loopback_send_complete_notify;
    loopback_send_params.cb_ctx = &loopback_send_params;

    //! Prepare recv request
    memset(&loopback_recv_msg, 0, sizeof(loopback_recv_msg));
    loopback_recv_params.payload_buffer = &loopback_recv_msg;
    loopback_recv_params.buffer_size = sizeof(loopback_recv_msg);
    loopback_recv_params.recv_cmd_code = ICC_LARGE_FIFO_MBOX_LPBK_TRIGGER_RESPONSE_CMD;
    loopback_recv_params.cb = my_icc_large_fifo_loopback_recv_complete_notify;
    loopback_recv_params.cb_ctx = &loopback_recv_params;

    //! Register for recv thru icc base
    fpfw_status_t status = fpfw_icc_base_recv(curr_icc_ctx, &loopback_recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[LOOPBACK_RECV] [FAIL] Recv Failed: Status[0x%x] CmdCode[0x%x]\n",
                     status,
                     ICC_LARGE_FIFO_MBOX_LPBK_TRIGGER_RESPONSE_CMD);
        goto func_exit;
    }
    else
    {
        FpFwCliPrint("[LOOPBACK_RECV] Recv Initiated: Status[0x%x] CmdCode[0x%x]\n", status, ICC_LARGE_FIFO_MBOX_LPBK_TRIGGER_RESPONSE_CMD);
    }

    //! Send the payload & wait for response
    status = fpfw_icc_base_send(curr_icc_ctx, &loopback_send_params);

    //! print status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[LOOPBACK_SEND] [FAIL] Send Failed: Status[0x%x]\n", status);
    }
    else
    {
        FpFwCliPrint("[LOOPBACK_SEND] Send Initiated: Status[0x%x] CmdCode[0x%x]\n",
                     status,
                     loopback_send_msg.hdr.cmd);
        //! Status is success, Set the flag to indicate the test is active
        is_accel_loopback_test_active = true;
    }

func_exit:
    return status;
}

static PLACED_CODE void my_icc_large_fifo_loopback_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    fpfw_icc_base_recv_req_t* req_params = (fpfw_icc_base_recv_req_t*)context; // NOLINT
    bool mismatch = false;

    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[LOOPBACK_RECV] Recv Failed: Status[0x%x] CmdCode[0x%x]\n", status, req_params->recv_cmd_code);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[LOOPBACK_RECV] Recv Complete: Status[0x%x] ReceivedBytes[%d] CmdCode[0x%x]\n",
                     status,
                     output_size_bytes,
                     req_params->recv_cmd_code);
        // Compare the send and recv payloads
        large_fifo_cli_mailbox_msg* recv_msg = (large_fifo_cli_mailbox_msg*)req_params->payload_buffer; // NOLINT
        if (recv_msg->hdr.cmd != ICC_LARGE_FIFO_MBOX_LPBK_TRIGGER_RESPONSE_CMD)
        {
            FpFwCliPrint("[LOOPBACK_RECV] [FAIL] Invalid CmdCode: Expected[0x%x] Received[0x%x]\n",
                         ICC_LARGE_FIFO_MBOX_LPBK_TRIGGER_RESPONSE_CMD,
                         recv_msg->hdr.cmd);
        }
        else
        {
            for (unsigned int i = 0; i < LARGE_FIFO_MBOX_MAX_PAYLOAD_WORD; i++)
            {
                if (recv_msg->data[i] != loopback_send_msg.data[i])
                {
                    mismatch = true;
                    //! Data mismatch, print the error
                    FpFwCliPrint(
                        "[LOOPBACK_RECV] [FAIL] Data Mismatch at index %d: Expected[0x%x] Received[0x%x]\n",
                        i,
                        loopback_send_msg.data[i],
                        recv_msg->data[i]);
                }
            }

            if (!mismatch)
            {
                loopback_rx_cntr += 1;
                loopback_test_cntr -= 1;
                FpFwCliPrint("[LOOPBACK_RECV] Cnt:%d Data Match: All data received correctly\n", loopback_rx_cntr);
            }
        }
    }

    if (loopback_test_cntr == 0)
    {
        is_accel_loopback_test_active = false;
    }
    else
    {
        my_icc_large_fifo_loopback_send_recv();
    }
}

static PLACED_CODE void my_icc_large_fifo_loopback_send_complete_notify(void* context, fpfw_status_t status)
{
    fpfw_icc_base_send_req_t* req_params = (fpfw_icc_base_send_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[LOOPBACK_SEND] [FAIL] Send Failed: Status[0x%x] Internal Status[0x%x]\n",
                     status,
                     req_params->send_req.Output.Status);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[LOOPBACK_SEND] Send Complete: Status[0x%x] CmdCode[0x%x]\n",
                     status,
                     loopback_send_msg.hdr.cmd);
    }
}

PLACED_CODE FPFW_CLI_STATUS large_fifo_mbox_loopback(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    unsigned int i;

    FPFW_UNUSED(argc);

    //! Prevent overwriting of the send payload buffer
    if (is_accel_loopback_test_active)
    {
        FpFwCliPrint("[FAIL] Loopback cmd: Test ongoing\n");
        return cli_status;
    }

    if (argc < 2)
    {
        FpFwCliPrint("[FAIL] Loopback cmd: Need iteration count\n");
        return cli_status;
    }

    if (strcmp(argv[0], "sdm_loopback") == 0)
    {
        curr_icc_ctx = icc_base_sdm_mbx_ctx;
    }
    else if (strcmp(argv[0], "cded_loopback") == 0)
    {
        curr_icc_ctx = icc_base_cded_mbx_ctx;
    }
    else
    {
        FpFwCliPrint("[FAIL] Invalid CLI %s\n", argv[0]);
        return cli_status;
    }

    // Ensure test is run at least once
    loopback_test_cntr = (atoi(argv[1]) > 0) ? atoi(argv[1]) : 1;
    loopback_rx_cntr = 0;

    loopback_send_msg.hdr.cmd = ICC_LARGE_FIFO_MBOX_LPBK_TRIGGER_CMD;
    for (i = 0; i < LARGE_FIFO_MBOX_MAX_PAYLOAD_WORD; i++)
    {
        loopback_send_msg.data[i] = i;
    }

    if (my_icc_large_fifo_loopback_send_recv() == FPFW_STATUS_SUCCESS)
    {
        cli_status = CLI_SUCCESS;
    }

    return cli_status;
}