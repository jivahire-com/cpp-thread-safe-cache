/**
 * @file icc_rmss_d2d_mbox_cli.c
 *
 */

/*------------- Includes -----------------*/
#include "icc_rmss_d2d_mbox_cli.h"

#include <DfwkStatus.h>           // for DFWK_SUCCESS
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
#define D2D_MBOX_TEST_PAYLOAD 0x11111111UL
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

//! externs populated by icc_cli_init
uint32_t current_die_id = DIE_0;
fpfw_icc_base_ctx_t* icc_base_rmss_d2d_mbx_ctx = NULL;

//! rmss d2d mbox message buffers for send/recv/echo
static rmss_d2d_mailbox_msg d2d_recv_msg;
static rmss_d2d_mailbox_msg d2d_send_msg;
static rmss_d2d_mailbox_msg d2d_echo_msg;

//! icc base send, recv & send/recv params for rmss d2d mbox
static fpfw_icc_base_send_recv_req_t d2d_send_recv_params;
static fpfw_icc_base_send_req_t d2d_send_params;
static fpfw_icc_base_recv_req_t d2d_recv_params;

//! test flags to prevent overlapping of send/recv operations
static bool is_d2d_echo_test_active = false;
static bool is_d2d_send_test_active = false;
static bool is_d2d_recv_test_active = false;
/*------------- Functions ----------------*/
void my_d2d_icc_base_send_complete_notify(void* context, fpfw_status_t status)
{
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    fpfw_icc_base_send_req_t* req_params = (fpfw_icc_base_send_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[D2D SEND TEST]   SCP[%d]->SCP[%d] Send Failed: Status[0x%x] Internal Status[0x%x]\n",
                     current_die_id,
                     remote_die_id,
                     status,
                     req_params->send_req.Output.Status);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[D2D SEND TEST]   SCP[%d]->SCP[%d] Send Complete: Status[0x%x] CmdCode [0x%x] Payload[",
                     current_die_id,
                     remote_die_id,
                     status,
                     GET_RMSS_D2D_MBOX_CMD_CODE(d2d_echo_msg.as_uint32[0]));
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", d2d_send_msg.as_uint32[i]);
        }
        FpFwCliPrint("]\n");
    }
    is_d2d_send_test_active = false;
}

FPFW_CLI_STATUS d2d_mbox_send(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    //! Prevent overwriting of the send payload buffer
    if (is_d2d_send_test_active)
    {
        FpFwCliPrint("D2D Send cmd: Test already active, please wait for completion\n");
        return cli_status;
    }

    //! reset mbox packet
    memset(&d2d_send_msg, 0, sizeof(d2d_send_msg));

    if (argc < 17)
    {
        FpFwCliPrint("D2D Send cmd: Insufficient Payload Args, Using default values\n");
        d2d_send_msg.as_uint32[0] = SET_RMSS_D2D_MAILBOX_HEADER_ASUNIT32(RMSS_D2D_MAILBOX_MSG_ECHO_REQ, 0, 0);
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
        FpFwCliPrint("[D2D SEND TEST]   SCP[%d]->SCP[%d] Send Failed: Status[0x%x]\n", current_die_id, remote_die_id, status);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[D2D SEND TEST]   SCP[%d]->SCP[%d] Send Initiated: Status[0x%x] Payload[", current_die_id, remote_die_id, status);
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

void d2d_handle_echo_test(rmss_d2d_mailbox_msg* echo_mesg)
{
    if (echo_mesg->header.cmd != RMSS_D2D_MAILBOX_MSG_ECHO_REQ)
    {
        return;
    }
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    FpFwCliPrint("[D2D ECHO TEST] SCP[%d]<-SCP[%d] Echo Test Data Received, Prepare to Echo Back!\n", current_die_id, remote_die_id);
    //! update the header to reflect response.
    echo_mesg->header.cmd = RMSS_D2D_MAILBOX_MSG_ECHO_RSP;
    //! call d2d_mbox_send
    char args[17][14];
    int arg_count = 17;
    memcpy(args[0], "d2d_mbox_send", sizeof("d2d_mbox_send"));

    for (uint8_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
    {
        snprintf(args[i + 1], sizeof(args[i + 1]), "%" PRIu32 "", echo_mesg->as_uint32[i]); // NOLINT
    }

    // Call the function with the arguments
    FPFW_CLI_STATUS status = d2d_mbox_send(arg_count, (const char**)args);

    // Check the status
    if (status == CLI_SUCCESS)
    {
        FpFwCliPrint("[D2D ECHO TEST] SCP[%d]->SCP[%d] Data Echoed Back Successfully\n", current_die_id, remote_die_id);
    }
    else
    {
        FpFwCliPrint("[D2D ECHO TEST] SCP[%d]->SCP[%d] Data Echo Failed\n", current_die_id, remote_die_id);
    }
}

void my_d2d_icc_base_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    fpfw_icc_base_recv_req_t* req_params = (fpfw_icc_base_recv_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[D2D RECV TEST]   SCP[%d]->SCP[%d] Recv Failed: Status[0x%x] CmdCode[0x%x]\n",
                     current_die_id,
                     remote_die_id,
                     status,
                     req_params->recv_cmd_code);
    }
    else
    {
        //! verify success, output status
        FpFwCliPrint("[D2D RECV TEST]   SCP[%d]->SCP[%d] Recv Complete: Status[0x%x] ReceivedBytes[%d] "
                     "CmdCode[0x%x] Payload[",
                     current_die_id,
                     remote_die_id,
                     status,
                     output_size_bytes,
                     req_params->recv_cmd_code);
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", d2d_recv_msg.as_uint32[i]);
        }
        FpFwCliPrint("]\n");
    }
    //! Check and handle if we received an echo request
    d2d_handle_echo_test(&d2d_recv_msg);
    is_d2d_recv_test_active = false;
}

FPFW_CLI_STATUS d2d_mbox_recv(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);

    if (is_d2d_recv_test_active)
    {
        FpFwCliPrint("D2D Recv cmd: Test already active, please wait for completion\n");
        return cli_status;
    }

    if (argc < 2)
    {
        FpFwCliPrint("D2D Recv cmd: Insufficient Args, Command Code Required\n");
        return cli_status;
    }
    uint32_t recv_cmd_code = atoi(argv[1]);

    //! @todo Check if the cmd code is valid, check from platform specific header file

    //! Prepare recv request
    memset(&d2d_recv_msg, 0, sizeof(d2d_recv_msg));
    d2d_recv_params.payload_buffer = &d2d_recv_msg;
    d2d_recv_params.buffer_size = sizeof(d2d_recv_msg);
    d2d_recv_params.recv_cmd_code = recv_cmd_code;
    d2d_recv_params.cb = my_d2d_icc_base_recv_complete_notify;
    d2d_recv_params.cb_ctx = &d2d_recv_params;

    //! Register for recv thru icc base
    fpfw_status_t status = fpfw_icc_base_recv(icc_base_rmss_d2d_mbx_ctx, &d2d_recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[D2D RECV TEST]   SCP[%d]->SCP[%d] Recv Failed: Status[0x%x] CmdCode[0x%x]\n",
                     current_die_id,
                     remote_die_id,
                     status,
                     recv_cmd_code);
    }
    else
    {
        FpFwCliPrint("[D2D RECV TEST]   SCP[%d]->SCP[%d] Recv Initiated: Status[0x%x] CmdCode[0x%x]\n",
                     current_die_id,
                     remote_die_id,
                     status,
                     recv_cmd_code);
        //! Status is success, Set the flag to indicate the test is active
        is_d2d_recv_test_active = true;
        cli_status = CLI_SUCCESS;
    }

    return cli_status;
}

void my_d2d_icc_base_send_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);
    fpfw_icc_base_send_recv_req_t* req_params = (fpfw_icc_base_send_recv_req_t*)context; // NOLINT
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[D2D ECHO TEST]   SCP[%d]->SCP[%d] Recv Failed: Status[0x%x]\n", current_die_id, remote_die_id, status);
    }
    else
    {
        uint32_t* recv_payload = (uint32_t*)req_params->recv_entry.payload_buffer; // NOLINT
        //! verify success, output status
        FpFwCliPrint("[D2D ECHO TEST]   SCP[%d]->SCP[%d] Recv Complete: Status[0x%x] ReceivedBytes[%d] "
                     "CmdCode[0x%x] Payload[",
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
    }
    is_d2d_echo_test_active = false;
}

FPFW_CLI_STATUS d2d_mbox_echo(int argc, const char** argv)
{
    FPFW_CLI_STATUS cli_status = CLI_ERROR;
    uint32_t remote_die_id = ((current_die_id == DIE_0) ? DIE_1 : DIE_0);

    //! Prevent overwriting of the payload buffer
    if (is_d2d_echo_test_active)
    {
        FpFwCliPrint("D2D Echo cmd: Test already active, please wait for completion\n");
        return cli_status;
    }

    //! reset mbox packet
    memset(&d2d_echo_msg, 0, sizeof(d2d_echo_msg));

    //! Set the designated cmd code for echo
    d2d_echo_msg.as_uint32[0] = SET_RMSS_D2D_MAILBOX_HEADER_ASUNIT32(RMSS_D2D_MAILBOX_MSG_ECHO_REQ, 0, 0);

    if (argc < 17)
    {
        FpFwCliPrint("D2D Echo cmd: Insufficient Payload Args, Using default values\n");
        for (uint8_t i = 1; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            d2d_echo_msg.as_uint32[i] = D2D_MBOX_TEST_PAYLOAD * (i);
        }
    }
    else
    {
        for (uint8_t i = 1; ((i < argc) && (i < D2D_MBOX_FIFO_DEPTH)); i++)
        {
            d2d_echo_msg.as_uint32[i] = atoi(argv[i]);
        }
    }

    //! Prepare send request
    d2d_send_recv_params.payload_buffer = &d2d_echo_msg;
    d2d_send_recv_params.buffer_size = sizeof(d2d_echo_msg);
    d2d_send_recv_params.cb = my_d2d_icc_base_send_recv_complete_notify;
    d2d_send_recv_params.cb_ctx = &d2d_send_recv_params;

    //! Send the payload & wait for response
    fpfw_status_t status = fpfw_icc_base_send_recv(icc_base_rmss_d2d_mbx_ctx, &d2d_send_recv_params);

    //! Print the status message
    if (status != DFWK_SUCCESS)
    {
        FpFwCliPrint("[D2D ECHO TEST]   SCP[%d]->SCP[%d] Send Failed: Status[0x%x]\n", current_die_id, remote_die_id, status);
    }
    else
    {

        //! verify success, output status
        FpFwCliPrint(
            "[D2D ECHO TEST]   SCP[%d]->SCP[%d] Send Initiated: Status[0x%x] CmdCode [0x%x] Payload[",
            current_die_id,
            remote_die_id,
            status,
            GET_RMSS_D2D_MBOX_CMD_CODE(d2d_echo_msg.as_uint32[0]));
        for (uint32_t i = 0; i < D2D_MBOX_FIFO_DEPTH; i++)
        {
            FpFwCliPrint("0x%x ", d2d_send_msg.as_uint32[i]);
        }
        FpFwCliPrint("]\n");

        is_d2d_echo_test_active = true;
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
    FPFW_CLI_STATUS cli_status = CLI_ERROR;

    if (argc != 4)
    {
        FpFwCliPrint("D2D Sync: Using Defaults:\n");
        my_d2d_sync_point.local_write_addr = d2d_sync_point.local_write_addr;
        my_d2d_sync_point.remote_write_addr = d2d_sync_point.remote_write_addr;
        my_d2d_sync_point.value = d2d_sync_point.value;
    }
    else
    {
        FpFwCliPrint("D2D Sync: Override Defaults:\n");
        my_d2d_sync_point.local_write_addr = (uintptr_t)atoi(argv[1]);
        my_d2d_sync_point.remote_write_addr = (uintptr_t)atoi(argv[2]);
        my_d2d_sync_point.value = (uint32_t)atoi(argv[3]);
    }
    FpFwCliPrint("Local Write Addr [0x%x]\nRemote Write Addr [0x%x]\nValue [0x%x]\n",
                 my_d2d_sync_point.local_write_addr,
                 my_d2d_sync_point.remote_write_addr,
                 my_d2d_sync_point.value);

    //! Get the die number to perform sync test
    FpFwCliPrint("D2D Sync Test Die [%d]\n", current_die_id);
    int status = mscp_exp_spi_synchronize_dies(my_d2d_sync_point, current_die_id);
    if (status != 0)
    {
        FpFwCliPrint("D2D Sync Test Failed\n");
    }
    else
    {
        FpFwCliPrint("D2D Sync Test Passed\n");
        cli_status = CLI_SUCCESS;
    }
    return cli_status;
}
