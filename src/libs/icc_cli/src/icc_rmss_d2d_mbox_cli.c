//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_rmss_d2d_mbox_cli.c
 *
 */

/*------------- Includes -----------------*/
#include "icc_rmss_d2d_mbox_cli.h"

#include <DfwkStatus.h> // for DFWK_SUCCESS
#include <FPFwInterrupts.h>
#include <FpFwAssert.h>           // for FPFW_RUNTIME_ASSERT
#include <FpFwCli.h>              // for FpFwCliPrint, FPFW_CLI_STATUS
#include <FpFwUtils.h>            // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <fpfw_icc_base.h>        // for fpfw_icc_base_recv_req_t
#include <fpfw_status.h>          // for fpfw_status_t
#include <icc_platform_defines.h> // for D2D_MBOX_FIFO_DEPTH, HSP_...
#include <idsw_kng.h>             // for PLATFORM_FPGA_LARGE
#include <inttypes.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <stdbool.h> // for false, bool, true
#include <stdint.h>  // for uint32_t, uint8_t
#include <stdlib.h>  // for atoi, NULL, size_t
#include <string.h>  // for memset

/*-- Symbolic Constant Macros (defines) --*/
#define D2D_MBOX_TEST_PAYLOAD      0x11111111UL
#define D2D_RMSS_RMBOX_SCP_IRQ_NUM 226
/*------------- Typedefs -----------------*/
/**
 * @brief List of d2d tests
 */
typedef enum _d2d_mbx_test_type
{
    SEND_TEST_ID,
    RECV_TEST_ID,
    ECHO_SERVER_TEST_ID,
    ECHO_CLIENT_TEST_ID,
    SYNC_TEST_ID,
    MAX_TEST_ID, //! keep last
} d2d_mbx_test_type;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
//! externs populated by icc_cli_init
uint32_t current_die_id = DIE_0;
fpfw_icc_base_ctx_t* icc_base_rmss_d2d_mbx_ctx = NULL;

//! rmss d2d mbox message buffers for send/recv/echo
static rmss_d2d_mailbox_msg d2d_recv_msg;
static rmss_d2d_mailbox_msg d2d_send_msg;
static rmss_d2d_mailbox_msg d2d_client_echo_msg;
static rmss_d2d_mailbox_msg d2d_serv_echo_recv_msg;
static rmss_d2d_mailbox_msg d2d_serv_echo_send_rsp_msg;

//! icc base send, recv & send/recv params for rmss d2d mbox
static fpfw_icc_base_send_recv_req_t d2d_send_recv_params; //! Used for echo client send/recv
static fpfw_icc_base_send_req_t d2d_send_params;           //! Generic send
static fpfw_icc_base_recv_req_t d2d_recv_params;           //! Generic recv
static fpfw_icc_base_recv_req_t d2d_echo_recv_params;      //! Used by echo serv to recv
static fpfw_icc_base_send_rsp_t d2d_send_echo_rsp_params;  //! Uesd by echo serv to send resp

//! test flags to prevent overlapping of send/recv operations
static bool is_d2d_echo_client_test_active = false;
static bool is_d2d_echo_serv_test_active = false;
static bool is_d2d_send_test_active = false;
static bool is_d2d_recv_test_active = false;

/**
 * @brief  List of test type names
 */
static const char* d2d_test_identifier_str[MAX_TEST_ID] =
    {[SEND_TEST_ID] = "D2D SEND TEST", [RECV_TEST_ID] = "D2D RECV TEST", [ECHO_SERVER_TEST_ID] = "D2D ECHO SERVER TEST", [ECHO_CLIENT_TEST_ID] = "D2D ECHO CLIENT TEST", [SYNC_TEST_ID] = "D2D SYNC TEST"};

/*----------------------------------------------- Callback Functions -------------------------------------------*/
/**
 * @brief Callback for generic send test
 */
void my_d2d_icc_base_send_complete_notify(void* context, fpfw_status_t status)
{
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    fpfw_icc_base_send_req_t* req_params = (fpfw_icc_base_send_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s]   SCP[%d]->SCP[%d] Send Failed: Status[0x%x] Internal Status[0x%x]\n",
                     d2d_test_identifier_str[SEND_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     status,
                     req_params->send_req.Output.Status);
    }
    else
    {
        uint32_t* send_payload = (uint32_t*)req_params->payload_buffer;
        uint32_t cmd_code = GET_RMSS_D2D_MBOX_CMD_CODE(send_payload[0]);
        //! verify success, output status
        FpFwCliPrint("[%s]   SCP[%d]->SCP[%d] Send Complete: Status[0x%x] CmdCode [0x%x] Payload[",
                     d2d_test_identifier_str[SEND_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     status,
                     cmd_code);
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", send_payload[i]);
        }
        FpFwCliPrint("]\n");
    }
    is_d2d_send_test_active = false;
}

/**
 * @brief Callback for echo server send response
 */
void my_d2d_icc_base_send_resp_complete_notify(void* context, fpfw_status_t status)
{
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    fpfw_icc_base_send_rsp_t* req_params = (fpfw_icc_base_send_rsp_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s][ASYNC]   SCP[%d]->SCP[%d] Send Resp Failed: Status[0x%x] Internal Status[0x%x]\n",
                     d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     status,
                     req_params->send_req.Output.Status);
    }
    else
    {
        uint32_t* send_payload = (uint32_t*)req_params->send_payload_buffer;
        uint32_t cmd_code = GET_RMSS_D2D_MBOX_CMD_CODE(send_payload[0]);
        //! verify success & print status
        FpFwCliPrint(
            "[%s][ASYNC]   SCP[%d]->SCP[%d] Send Resp Complete: Status[0x%x] CmdCode [0x%x] Payload[",
            d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
            current_die_id,
            remote_die_id,
            status,
            cmd_code);
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", send_payload[i]);
        }
        FpFwCliPrint("]\n");
        is_d2d_echo_serv_test_active = false;
    }
}

/**
 * @brief Common Callback for generic receive & echo server recv test
 */
void my_d2d_icc_base_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    fpfw_icc_base_recv_req_t* req_params = (fpfw_icc_base_recv_req_t*)context; // NOLINT
    uint32_t recv_cmd_code = req_params->recv_cmd_code;
    int32_t test_id = (recv_cmd_code == RMSS_D2D_MAILBOX_MSG_ECHO_REQ) ? ECHO_SERVER_TEST_ID : RECV_TEST_ID;

    //! Verify success & print
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s]   SCP[%d]->SCP[%d] Recv Failed: Status[0x%x] CmdCode[0x%x]\n",
                     d2d_test_identifier_str[test_id],
                     current_die_id,
                     remote_die_id,
                     status,
                     recv_cmd_code);

        if (test_id == RECV_TEST_ID)
        {
            is_d2d_recv_test_active = false;
        }
        else
        {
            is_d2d_echo_serv_test_active = false;
        }
        return;
    }

    uint32_t* recv_payload = (uint32_t*)req_params->payload_buffer; // NOLINT
    FpFwCliPrint("[%s]   SCP[%d]->SCP[%d] Recv Complete: Status[0x%x] ReceivedBytes[%d] "
                 "CmdCode[0x%x] Payload[",
                 d2d_test_identifier_str[test_id],
                 current_die_id,
                 remote_die_id,
                 status,
                 output_size_bytes,
                 recv_cmd_code);
    for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
    {
        FpFwCliPrint("0x%x ", recv_payload[i]);
    }
    FpFwCliPrint("]\n");

    //! Specific actions as per the current test
    if (test_id == RECV_TEST_ID)
    {
        //! generic receive is done, nothing else to do, reset test flag & return
        is_d2d_recv_test_active = false;
        return;
    }

    //! Further handling for echo server recv
    //! copy over the received payload to send rsp in order to echo back
    memcpy(&d2d_serv_echo_send_rsp_msg, &d2d_serv_echo_recv_msg, sizeof(rmss_d2d_mailbox_msg));
    //! update response cmd code in header
    d2d_serv_echo_send_rsp_msg.header.cmd = RMSS_D2D_MAILBOX_MSG_ECHO_RSP;

    //! Prepare send response post receiving command
    d2d_send_echo_rsp_params.recv_payload_buffer = req_params->payload_buffer;
    d2d_send_echo_rsp_params.recv_buffer_size = req_params->buffer_size;
    d2d_send_echo_rsp_params.send_payload_buffer = &d2d_serv_echo_send_rsp_msg;
    d2d_send_echo_rsp_params.send_buffer_size = sizeof(d2d_serv_echo_send_rsp_msg);
    d2d_send_echo_rsp_params.cb = my_d2d_icc_base_send_resp_complete_notify;
    d2d_send_echo_rsp_params.cb_ctx = &d2d_send_echo_rsp_params;

    //! Send the payload & wait for response
    fpfw_status_t send_status = fpfw_icc_base_send_resp(icc_base_rmss_d2d_mbx_ctx, &d2d_send_echo_rsp_params);

    //! print status message
    if (send_status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s][ASYNC]   SCP[%d]->SCP[%d] Send Resp Failed: Status[0x%x]\n",
                     d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     send_status);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[%s][ASYNC]   SCP[%d]->SCP[%d] Send Resp Initiated: Status[0x%x] Payload[",
                     d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     send_status);
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", d2d_serv_echo_send_rsp_msg.as_uint32[i]);
        }
        FpFwCliPrint("]\n");
    }
}

void my_d2d_icc_base_send_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    fpfw_icc_base_send_recv_req_t* req_params = (fpfw_icc_base_send_recv_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s][ASYNC]   SCP[%d]->SCP[%d] Recv Failed: Status[0x%x]\n",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     status);
    }
    else
    {
        uint32_t* recv_payload = (uint32_t*)req_params->recv_entry.payload_buffer; // NOLINT
        //! verify success, output status
        FpFwCliPrint("[%s][ASYNC]   SCP[%d]->SCP[%d] Recv Complete: Status[0x%x] ReceivedBytes[%d] "
                     "CmdCode[0x%x] Payload[",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     status,
                     output_size_bytes,
                     GET_RMSS_D2D_MBOX_CMD_CODE(recv_payload[0]));
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", recv_payload[i]);
        }
        FpFwCliPrint("]\n");

        //! Compare the received payload with the sent payload, excluding the header
        if (memcmp(&d2d_client_echo_msg.as_uint32[1], &recv_payload[1], sizeof(rmss_d2d_mailbox_msg) - 4) == 0)
        {
            FpFwCliPrint("[%s][ASYNC]   SCP[%d]->SCP[%d] Echo Test Passed: Received data matches sent data\n",
                         d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
                         current_die_id,
                         remote_die_id);
        }
        else
        {
            FpFwCliPrint(
                "[%s][ASYNC]   SCP[%d]->SCP[%d] Echo Test Failed: Received data does not match sent data\n",
                d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
                current_die_id,
                remote_die_id);

            FpFwCliPrint("Received Payload: ");
            for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
            {
                FpFwCliPrint("0x%x ", d2d_client_echo_msg.as_uint32[i]);
            }
            FpFwCliPrint("\n");
        }
    }
    is_d2d_echo_client_test_active = false;
}

/*---------------------------------------------- Helper Functions ---------------------------------------------*/
fpfw_status_t d2d_mbox_recv_common(fpfw_icc_base_recv_req_t* recv_param, rmss_d2d_mailbox_msg* msg, uint32_t recv_cmd_code, d2d_mbx_test_type type)
{
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);

    //! Prepare recv request
    memset(msg, 0, sizeof(rmss_d2d_mailbox_msg));
    recv_param->payload_buffer = msg;
    recv_param->buffer_size = sizeof(rmss_d2d_mailbox_msg);
    recv_param->recv_cmd_code = recv_cmd_code;
    recv_param->cb = my_d2d_icc_base_recv_complete_notify;
    recv_param->cb_ctx = recv_param;

    //! Register for recv thru icc base
    fpfw_status_t status = fpfw_icc_base_recv(icc_base_rmss_d2d_mbx_ctx, recv_param);

    //! Print the status message
    FpFwCliPrint("[%s]   SCP[%d]->SCP[%d] Recv %s: Status[0x%x] CmdCode[0x%x]\n",
                 d2d_test_identifier_str[type],
                 current_die_id,
                 remote_die_id,
                 (status == DFWK_SUCCESS ? "Initiated" : "Failed"),
                 status,
                 recv_cmd_code);

    return status;
}

/*---------------------------------------------- CLI Test Functions ------------------------------------------*/
FPFW_CLI_STATUS d2d_mbox_send(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    //! Prevent overwriting of the send payload buffer
    if (is_d2d_send_test_active)
    {
        FpFwCliPrint("[%s]: Test already active, please wait for completion\n", d2d_test_identifier_str[SEND_TEST_ID]);
        return cli_status;
    }

    //! reset mbox packet
    memset(&d2d_send_msg, 0, sizeof(d2d_send_msg));

    if (argc < 17)
    {
        FpFwCliPrint("[%s]: Insufficient Payload Args, Using default values\n", d2d_test_identifier_str[SEND_TEST_ID]);
        d2d_send_msg.as_uint32[0] = SET_RMSS_D2D_MAILBOX_HEADER_ASUNIT32(RMSS_D2D_MAILBOX_MSG_TEST_REQ, 0, 0);
        for (uint8_t i = 1; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            d2d_send_msg.as_uint32[i] = D2D_MBOX_TEST_PAYLOAD * (i);
        }
    }
    else
    {
        for (uint8_t i = 0; ((i < argc) && (i < D2D_MBOX_FIFO_DEPTH)); i++)
        {
            d2d_send_msg.as_uint32[i] = atoi(argv[i + 1]);
        }
    }

    //! Prepare send request
    d2d_send_params.payload_buffer = &d2d_send_msg;
    d2d_send_params.buffer_size = sizeof(d2d_send_msg);
    d2d_send_params.cb = my_d2d_icc_base_send_complete_notify;
    d2d_send_params.cb_ctx = &d2d_send_params;

    //! Send the payload & wait for response
    fpfw_status_t status = fpfw_icc_base_send(icc_base_rmss_d2d_mbx_ctx, &d2d_send_params);

    //! print status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s]   SCP[%d]->SCP[%d] Send Failed: Status[0x%x]\n",
                     d2d_test_identifier_str[SEND_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     status);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[%s]   SCP[%d]->SCP[%d] Send Initiated: Status[0x%x] Payload[",
                     d2d_test_identifier_str[SEND_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     status);
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", d2d_send_msg.as_uint32[i]);
        }
        FpFwCliPrint("]\n");

        is_d2d_send_test_active = true;
        cli_status = CLI_SUCCESS;
    }
    return cli_status;
}

FPFW_CLI_STATUS d2d_mbox_recv(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    uint16_t recv_cmd_code = 0;

    if (is_d2d_recv_test_active)
    {
        FpFwCliPrint("[%s]: Test already active, please wait for completion\n", d2d_test_identifier_str[RECV_TEST_ID]);
        return cli_status;
    }

    if ((argc < 2) || (atoi(argv[1]) < RMSS_D2D_MAILBOX_MSG_ECHO_RSP))
    {
        FpFwCliPrint("[%s]: Insufficient Args or Invalid cmd code, Using default Command code 0x2 -> "
                     "RMSS_D2D_MAILBOX_MSG_TEST_REQ\n",
                     d2d_test_identifier_str[RECV_TEST_ID]);
        recv_cmd_code = RMSS_D2D_MAILBOX_MSG_TEST_REQ;
    }
    else
    {
        recv_cmd_code = atoi(argv[1]);
    }

    //! call in common recv handler
    fpfw_status_t status = d2d_mbox_recv_common(&d2d_recv_params, &d2d_recv_msg, recv_cmd_code, RECV_TEST_ID);
    if (status == DFWK_SUCCESS)
    {
        is_d2d_recv_test_active = true;
        cli_status = CLI_SUCCESS;
    }

    return cli_status;
}

FPFW_CLI_STATUS d2d_mbox_echo_server(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);

    FPFW_CLI_STATUS cli_status = CLI_ERROR;

    if (is_d2d_echo_serv_test_active)
    {
        FpFwCliPrint("[%s][ASYNC]: Test already active, please wait for completion\n",
                     d2d_test_identifier_str[ECHO_SERVER_TEST_ID]);
        return cli_status;
    }

    //! call in common recv handler with dedicated cmd code
    fpfw_status_t status =
        d2d_mbox_recv_common(&d2d_echo_recv_params, &d2d_serv_echo_recv_msg, RMSS_D2D_MAILBOX_MSG_ECHO_REQ, ECHO_SERVER_TEST_ID);
    if (status == DFWK_SUCCESS)
    {
        is_d2d_echo_serv_test_active = true;
        cli_status = CLI_SUCCESS;
    }

    return cli_status;
}

FPFW_CLI_STATUS d2d_mbox_echo_client(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);

    //! Prevent overwriting of the payload buffer
    if (is_d2d_echo_client_test_active)
    {
        FpFwCliPrint("[%s][ASYNC]: Client Side Test already active, please wait for completion\n",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID]);
        return cli_status;
    }

    //! reset echo mbox packet to send
    memset(&d2d_client_echo_msg, 0, sizeof(d2d_client_echo_msg));

    //! Set the designated cmd code for echo
    d2d_client_echo_msg.as_uint32[0] = SET_RMSS_D2D_MAILBOX_HEADER_ASUNIT32(RMSS_D2D_MAILBOX_MSG_ECHO_REQ, 0, 0);

    if (argc < 17)
    {
        FpFwCliPrint("[%s][ASYNC]: Insufficient Payload Args, Using default values\n",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID]);
        for (uint8_t i = 1; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            d2d_client_echo_msg.as_uint32[i] = D2D_MBOX_TEST_PAYLOAD * (i);
        }
    }
    else
    {
        for (uint8_t i = 1; ((i < argc) && (i < D2D_MBOX_FIFO_DEPTH)); i++)
        {
            d2d_client_echo_msg.as_uint32[i] = atoi(argv[i]);
        }
    }

    //! Prepare send request
    d2d_send_recv_params.payload_buffer = &d2d_client_echo_msg;
    d2d_send_recv_params.buffer_size = sizeof(d2d_client_echo_msg);
    d2d_send_recv_params.cb = my_d2d_icc_base_send_recv_complete_notify;
    d2d_send_recv_params.cb_ctx = &d2d_send_recv_params;

    //! Send the payload & wait for response
    fpfw_status_t status = fpfw_icc_base_send_recv(icc_base_rmss_d2d_mbx_ctx, &d2d_send_recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s][ASYNC]   SCP[%d]->SCP[%d] Send Failed: Status[0x%x]\n",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     status);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[%s][ASYNC]   SCP[%d]->SCP[%d] Send Initiated: Status[0x%x] CmdCode [0x%x] Payload[",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     status,
                     GET_RMSS_D2D_MBOX_CMD_CODE(d2d_client_echo_msg.as_uint32[0]));
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", d2d_client_echo_msg.as_uint32[i]);
        }
        FpFwCliPrint("]\n");

        is_d2d_echo_client_test_active = true;
        cli_status = CLI_SUCCESS;
    }
    return cli_status;
}

// KNG SOC Address Map > MSCP_EXP Address Map
// +-------------+-------------+-------+---------------------+
// | Addr Start  | Addr End    | Size  | Name                |
// +-------------+-------------+-------+---------------------+
// | 0x0169_0000 | 0x0169_FFFF | 64kiB | MSCP_EXP/SPI Host   |
// | 0x016A_0000 | 0x016A_FFFF | 64kiB | MSCP_EXP/SPI Bridge |
// +-------------+-------------+-------+---------------------+
// KNG SOC Address Map->MSCP_EXP Address Map
// 0x0120_0000    0x012F_FFFF    1024kiB    MSCP_EXP/RAM0    SRAM0
// 0x0130_0000    0x013F_FFFF    1024kiB    MSCP_EXP/RAM1    SRAM1

//! Defaults for the test
// #define LOCAL_WRITE_ADDR     MSCP_EXP_RAM0
// #define REMOTE_WRITE_ADDR    (LOCAL_WRITE_ADDR + 4)
// #define SYNC_VALUE           (0xD2DB600D) // "die to die be good"

static mscp_exp_spi_sync_point_t my_d2d_sync_point;

/**
 * Check for synchronization if the dies using the SPI. The synchronization will happen
 * through SRAM0 in die 1. Die 0 will use the SPI to access this memory. Die 1
 * will use local access to this memory. SRAM0 in DIE1 will be the memory used.
 */
FPFW_CLI_STATUS d2d_sync_test(int argc, const char** argv)
{
    if (argc != 4)
    {
        FpFwCliPrint("[%s]: No args provided, Using Defaults:\n", d2d_test_identifier_str[SYNC_TEST_ID]);
        my_d2d_sync_point.local_write_addr = d2d_sync_point.local_write_addr;
        my_d2d_sync_point.remote_write_addr = d2d_sync_point.remote_write_addr;
        my_d2d_sync_point.value = d2d_sync_point.value;
    }
    else
    {
        FpFwCliPrint("[%s]: Override Defaults:\n", d2d_test_identifier_str[SYNC_TEST_ID]);
        my_d2d_sync_point.local_write_addr = (uintptr_t)atoi(argv[1]);
        my_d2d_sync_point.remote_write_addr = (uintptr_t)atoi(argv[2]);
        my_d2d_sync_point.value = (uint32_t)atoi(argv[3]);
    }
    FpFwCliPrint("[%s]  Die[%d] Local Write Addr [0x%x]\nRemote Write Addr [0x%x]\nValue [0x%x]\n",
                 d2d_test_identifier_str[SYNC_TEST_ID],
                 current_die_id,
                 my_d2d_sync_point.local_write_addr,
                 my_d2d_sync_point.remote_write_addr,
                 my_d2d_sync_point.value);

    int status = mscp_exp_spi_synchronize_dies(my_d2d_sync_point, current_die_id);
    FpFwCliPrint("[%s] %s\n", d2d_test_identifier_str[SYNC_TEST_ID], (status != 0 ? "Failed" : "Passed"));
    return status;
}

/**
 * @brief Cli option to unblock tests, Only to be used for debugging cli testing
 * Must not be a part of FSE/functional testing
 *
 * @example if `echo_client` was called on die 0, before `echo serv` on die 1,
 * echo_client will never get the recv complete (msg was likely dropped on die1, no was listening)
 * and this will prevent us from trying to call echo_client again on die 0 as the
 * prev test sequence isn't finished. The reset command will come handy in such situation
 */
FPFW_CLI_STATUS d2d_reset_test(int argc, const char** argv)
{
    if (argc < 2)
    {
        //! unblocks all active tests to override wait for completion
        is_d2d_echo_client_test_active = false;
        is_d2d_echo_serv_test_active = false;
        is_d2d_send_test_active = false;
        is_d2d_recv_test_active = false;
        FpFwCliPrint("No arg, by default all tests were Reset\n");
    }
    else
    {
        d2d_mbx_test_type test_id = atoi(argv[1]);
        if (test_id >= MAX_TEST_ID)
        {
            FpFwCliPrint("Invalid test ID, No tests were Reset\n");
            return CLI_ERROR;
        }
        switch (test_id)
        {
        case SEND_TEST_ID:
            is_d2d_send_test_active = false;
            break;
        case RECV_TEST_ID:
            is_d2d_recv_test_active = false;
            break;
        case ECHO_SERVER_TEST_ID:
            is_d2d_echo_serv_test_active = false;
            break;
        case ECHO_CLIENT_TEST_ID:
            is_d2d_echo_client_test_active = false;
            break;
        default:
            FpFwCliPrint("Unsupported Arg, No tests were Reset\n");
            return CLI_ERROR;
        }
        FpFwCliPrint("%s Reset Successfully\n", d2d_test_identifier_str[test_id]);
    }
    return CLI_SUCCESS;
}

FPFW_CLI_STATUS d2d_mbox_echo_server_sync(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);

    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    size_t recv_msg_size_bytes = 0;
    rmss_d2d_mailbox_msg client_echo_msg = {
        .header.cmd = RMSS_D2D_MAILBOX_MSG_ECHO_REQ,
    };
    rmss_d2d_mailbox_msg client_echo_rsp_msg = {0};

    //! block until recv sync completes
    FpFwCliPrint("[%s][SYNC]: SCP[%d]->SCP[%d] Wait Until Command Received\n",
                 d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
                 remote_die_id,
                 current_die_id);

    //! Dispatcher is actively listening for recv requests at runtime, disable the recv interrupt for sync operation
    FPFwCoreInterruptDisableVector(D2D_RMSS_RMBOX_SCP_IRQ_NUM);
    fpfw_status_t icc_status =
        fpfw_icc_base_recv_sync(icc_base_rmss_d2d_mbx_ctx, &client_echo_msg, sizeof(rmss_d2d_mailbox_msg), &recv_msg_size_bytes);
    FPFwCoreInterruptEnableVector(D2D_RMSS_RMBOX_SCP_IRQ_NUM);

    if (icc_status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s][SYNC]   SCP[%d]->SCP[%d] Recv Req Failed: Status[0x%x] CmdCode[0x%x]\n",
                     d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
                     remote_die_id,
                     current_die_id,
                     icc_status,
                     client_echo_msg.header.cmd);

        return cli_status;
    }

    FpFwCliPrint("[%s][SYNC]   SCP[%d]->SCP[%d] Command Received : ReceivedBytes[%d] "
                 "CmdCode[0x%x] Payload[",
                 d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
                 remote_die_id,
                 current_die_id,
                 recv_msg_size_bytes,
                 client_echo_msg.rsp.header.cmd);

    for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
    {
        FpFwCliPrint("0x%x ", client_echo_msg.as_uint32[i]);
    }
    FpFwCliPrint("]\n");

    //! copy over recv payload into send resp payload to echo back
    memcpy(&client_echo_rsp_msg, &client_echo_msg, sizeof(rmss_d2d_mailbox_msg));
    //! Set the cmd code for resp
    client_echo_rsp_msg.header.cmd = RMSS_D2D_MAILBOX_MSG_ECHO_RSP;

    //! block until send response sync completes
    FpFwCliPrint("[%s][SYNC]: SCP[%d]->SCP[%d] Send Resp, block until success\n",
                 d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
                 current_die_id,
                 remote_die_id);

    icc_status = fpfw_icc_base_send_resp_sync(icc_base_rmss_d2d_mbx_ctx,
                                              &client_echo_msg,
                                              sizeof(rmss_d2d_mailbox_msg),
                                              &client_echo_rsp_msg,
                                              sizeof(rmss_d2d_mailbox_msg));
    if (icc_status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s][SYNC]   SCP[%d]->SCP[%d] Send Resp Failed: Status[0x%x]\n",
                     d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     icc_status);
    }
    else
    {
        //! verify success & print status
        FpFwCliPrint("[%s][SYNC]   SCP[%d]->SCP[%d] Send Resp Complete: CmdCode [0x%x] Payload[",
                     d2d_test_identifier_str[ECHO_SERVER_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     client_echo_rsp_msg.header.cmd);
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", client_echo_rsp_msg.as_uint32[i]);
        }
        FpFwCliPrint("]\n");
        cli_status = CLI_SUCCESS;
    }
    return cli_status;
}

FPFW_CLI_STATUS d2d_mbox_echo_client_sync(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    size_t recv_msg_size_bytes = 0;
    rmss_d2d_mailbox_msg client_echo_msg = {
        .header.cmd = RMSS_D2D_MAILBOX_MSG_ECHO_REQ,
    };
    rmss_d2d_mailbox_msg compare_buffer = {0};

    if (argc < 17)
    {
        FpFwCliPrint("[%s][SYNC]: Insufficient Payload Args, Using default values\n",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID]);
        for (uint8_t i = 1; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            client_echo_msg.as_uint32[i] = D2D_MBOX_TEST_PAYLOAD * (i);
        }
    }
    else
    {
        for (uint8_t i = 1; ((i < argc) && (i < D2D_MBOX_FIFO_DEPTH)); i++)
        {
            client_echo_msg.as_uint32[i] = atoi(argv[i]);
        }
    }

    //! Send the message to remote & get response, blocking call
    FpFwCliPrint(
        "[%s][SYNC]   SCP[%d]->SCP[%d] Send Mesg & Block Until Resp Received: CmdCode [0x%x] Payload[",
        d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
        current_die_id,
        remote_die_id,
        client_echo_msg.header.cmd);
    for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
    {
        FpFwCliPrint("0x%x ", client_echo_msg.as_uint32[i]);
    }
    FpFwCliPrint("]\n");

    //! store a copy of the mesg into temp buffer
    memcpy(&compare_buffer, &client_echo_msg, sizeof(rmss_d2d_mailbox_msg));

    //! Dispatcher is actively listening for recv requests in runtime, disable the recv interrupt for sync operation
    FPFwCoreInterruptDisableVector(D2D_RMSS_RMBOX_SCP_IRQ_NUM);
    //! Call the actual FUT
    fpfw_status_t icc_status =
        fpfw_icc_base_send_recv_sync(icc_base_rmss_d2d_mbx_ctx, &client_echo_msg, sizeof(rmss_d2d_mailbox_msg), &recv_msg_size_bytes);
    FPFwCoreInterruptEnableVector(D2D_RMSS_RMBOX_SCP_IRQ_NUM);

    if (icc_status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[%s][SYNC]   SCP[%d]->SCP[%d] Send/Recv Failed: Status[0x%x] CmdCode[0x%x]\n",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     icc_status,
                     client_echo_msg.header.cmd);
    }
    else
    {
        FpFwCliPrint("[%s][SYNC]   SCP[%d]->SCP[%d] Resp Received : ReceivedBytes[%d] "
                     "CmdCode[0x%x] Payload[",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
                     current_die_id,
                     remote_die_id,
                     recv_msg_size_bytes,
                     client_echo_msg.rsp.header.cmd);

        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", client_echo_msg.as_uint32[i]);
        }
        FpFwCliPrint("]\n");
        cli_status = CLI_SUCCESS;
    }

    //! compare the received payload data with the send payload data in the temp buffer, excluding the metadata
    if (memcmp(&client_echo_msg.as_uint32[1], &compare_buffer.as_uint32[1], sizeof(rmss_d2d_mailbox_msg) - 4) == 0)
    {
        FpFwCliPrint("[%s][SYNC]   SCP[%d]->SCP[%d] Echo Test Passed: Received data matches sent data\n",
                     d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
                     current_die_id,
                     remote_die_id);
    }
    else
    {
        FpFwCliPrint(
            "[%s][SYNC]   SCP[%d]->SCP[%d] Echo Test Failed: Received data does not match sent data\n",
            d2d_test_identifier_str[ECHO_CLIENT_TEST_ID],
            current_die_id,
            remote_die_id);
        cli_status = CLI_ERROR;
    }
    return cli_status;
}