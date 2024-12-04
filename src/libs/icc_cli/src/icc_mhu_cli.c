//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_mhu_cli.c
 * Private implementation for icc mhu cli functionality.
 */

/*------------- Includes -----------------*/

#include "icc_cli_i.h"
#include "icc_mhu_cli_i.h"

#include <FpFwCli.h>
#include <FpFwUtils.h>
#include <fpfw_icc_base.h>
#include <icc_cli.h>
#include <icc_mhu.h>
#include <mhu_icc_transport.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

#define MAX_CLI_MHU_PAYLOAD_SIZE (512)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern const char* icc_cli_interface_type_strings[ICC_CLI_MAX_TRANSPORT_TYPE];

static bool s_pending_send_request = false;
static bool s_pending_recv_request = false;

static uint8_t s_icc_mhu_send_payload[MAX_CLI_MHU_PAYLOAD_SIZE] = {0};
static uint8_t s_icc_mhu_recv_payload[MAX_CLI_MHU_PAYLOAD_SIZE] = {0};

static fpfw_icc_base_send_req_t s_icc_base_send_params;
static fpfw_icc_base_recv_req_t s_icc_base_recv_params;

/*------------- Functions ----------------*/

static void mhu_icc_base_send_complete_notify(void* context, fpfw_status_t status)
{
    FpFwCliPrint("ICC CLI MHU Send Complete CB - status: 0x%08x\n", status);

    icc_mhu_packet_t* req = (icc_mhu_packet_t*)context;
    FpFwCliPrint("ICC CLI MHU Send Complete CB - command: 0x%08x\n", req->header.msg_header.command);
    FpFwCliPrint("ICC CLI MHU Send Complete CB - payload size: %d\n", req->header.msg_header.payload_size);
    uint8_t* payload = (uint8_t*)req->payload;
    for (int i = 0; i < req->header.msg_header.payload_size; i++)
    {
        FpFwCliPrint("ICC CLI MHU Send Complete CB - payload [%d] == 0x%x \n", i, payload[i]);
    }

    s_pending_send_request = false;
}

static void mhu_icc_base_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FpFwCliPrint("ICC CLI MHU Recv Complete CB - status: 0x%08x\n", status);

    icc_mhu_packet_t* req = (icc_mhu_packet_t*)context;
    FpFwCliPrint("ICC CLI MHU Recv Complete CB - command: 0x%08x\n", req->header.msg_header.command);
    FpFwCliPrint("ICC CLI MHU Recv Complete CB - payload size: %d\n", req->header.msg_header.payload_size);
    uint8_t* payload = (uint8_t*)req->payload;
    for (int i = 0; i < req->header.msg_header.payload_size; i++)
    {
        FpFwCliPrint("ICC CLI MHU Recv Complete CB - payload [%d] == 0x%x \n", i, payload[i]);
    }

    s_pending_recv_request = false;
}

FPFW_CLI_STATUS mhu_list_indices(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);

    icc_cli_init_params_t* cli_ctx = get_icc_cli_ctx();

    FpFwCliPrint("Listing the MHU indices -- START\n");
    for (icc_cli_interface_type i = ICC_CLI_MSCP_MHU; i < ICC_CLI_MAX_TRANSPORT_TYPE; i++)
    {
        FpFwCliPrint("Name: [%17ss] Index: [%02d] Initialized: [%d]\n",
                     icc_cli_interface_type_strings[i],
                     i,
                     cli_ctx->icc_base_ctx[i] != NULL);
    }
    FpFwCliPrint("Listing the MHU indices -- END\n");

    return CLI_SUCCESS;
}

FPFW_CLI_STATUS mhu_recv(int argc, const char** argv)
{
    /**
     * We can recv if all of the following are true:
     * 1. There is no pending recv request
     * 2. The index matches a valid index of an icc base context
     * 3. The context has been initialized
     */

    // expected arguments: recv <index> <command>
    if (argc < 3)
    {
        FpFwCliPrint("ERROR! Insufficient Args: recv\n");
        FpFwCliPrint("Usage: recv <index> <command>\n");
        return CLI_ERROR;
    }

    // Check if there is a pending request
    if (s_pending_recv_request)
    {
        FpFwCliPrint("ERROR! There is a pending request\n");
        return CLI_ERROR;
    }

    // Grab the index
    int index = strtoul(argv[1], NULL, 0);
    if (index < ICC_CLI_MSCP_MHU || index >= ICC_CLI_MAX_TRANSPORT_TYPE)
    {
        FpFwCliPrint("ERROR! Invalid index\n");
        return CLI_ERROR;
    }

    icc_cli_init_params_t* cli_ctx = get_icc_cli_ctx();

    // Check if the context has been initialized
    if (cli_ctx->icc_base_ctx[index] == NULL)
    {
        FpFwCliPrint("ERROR! Context for index not initialized\n");
        return CLI_ERROR;
    }

    // Update the payload with the data from the arguments
    memset(s_icc_mhu_recv_payload, 0, MAX_CLI_MHU_PAYLOAD_SIZE);

    // Build the request to recv
    s_icc_base_recv_params.recv_cmd_code = strtoul(argv[2], NULL, 0);
    // s_icc_base_recv_params. FPFW_ICC_DISPATCH_OPTION_UNUSED
    s_icc_base_recv_params.payload_buffer = s_icc_mhu_recv_payload;
    s_icc_base_recv_params.buffer_size = sizeof(s_icc_mhu_recv_payload);
    s_icc_base_recv_params.cb = mhu_icc_base_recv_complete_notify;
    s_icc_base_recv_params.cb_ctx = s_icc_mhu_recv_payload;

    // Send the payload
    fpfw_status_t status = fpfw_icc_base_recv(cli_ctx->icc_base_ctx[index], &s_icc_base_recv_params);
    FpFwCliPrint("ICC MHU recv status: 0x%08x\n", status);

    s_pending_recv_request = (status == FPFW_STATUS_SUCCESS);

    return CLI_SUCCESS;
}

FPFW_CLI_STATUS mhu_send(int argc, const char** argv)
{
    /**
     * We can send if all of the following are true:
     * 1. There is no pending send request
     * 2. The index matches a valid index of an icc base context
     * 3. The context has been initialized
     * 4. The size is <= MAX_CLI_MHU_PAYLOAD_SIZE
     */

    // expected arguments: send <index> <command> <size> <data 0> ... <data n-1>
    if (argc < 3)
    {
        FpFwCliPrint("ERROR! Insufficient Args: send\n");
        FpFwCliPrint("Usage: send <index> <command> <size> <data0> ... <data n-1>. Max size %d\n", MAX_CLI_MHU_PAYLOAD_SIZE);
        return CLI_ERROR;
    }

    // Check if there is a pending request
    if (s_pending_send_request)
    {
        FpFwCliPrint("ERROR! There is a pending request\n");
        return CLI_ERROR;
    }

    // Grab the index
    int index = strtoul(argv[1], NULL, 0);
    if (index < ICC_CLI_MSCP_MHU || index >= ICC_CLI_MAX_TRANSPORT_TYPE)
    {
        FpFwCliPrint("ERROR! Invalid index\n");
        return CLI_ERROR;
    }

    icc_cli_init_params_t* cli_ctx = get_icc_cli_ctx();

    // Check if the context has been initialized
    if (cli_ctx->icc_base_ctx[index] == NULL)
    {
        FpFwCliPrint("ERROR! Context for index not initialized\n");
        return CLI_ERROR;
    }

    int size = 0;
    if (argc > 3)
    {
        // Check if the size is <= MAX_CLI_MHU_PAYLOAD_SIZE
        size = strtoul(argv[3], NULL, 0);
        if (size > MAX_CLI_MHU_PAYLOAD_SIZE)
        {
            FpFwCliPrint("ERROR! Size %d exceeds max payload size %d\n", size, MAX_CLI_MHU_PAYLOAD_SIZE);
            return CLI_ERROR;
        }

        if (argc != (4 + size))
        {
            FpFwCliPrint("ERROR! Insufficient data arguments\n");
            FpFwCliPrint("Usage: send <index> <command> <size> <data0> ... <data n-1>. Max size %d\n", MAX_CLI_MHU_PAYLOAD_SIZE);
            return CLI_ERROR;
        }

        // Update the payload with the data from the arguments
        memset(s_icc_mhu_send_payload, 0, MAX_CLI_MHU_PAYLOAD_SIZE);
        for (int i = 0; i < size; i++)
        {
            s_icc_mhu_send_payload[sizeof(icc_mhu_header_t) + i] = strtoul(argv[4 + i], NULL, 0);
        }
    }

    // Build the request to send
    icc_mhu_packet_t* p_send_req = (icc_mhu_packet_t*)s_icc_mhu_send_payload;
    p_send_req->header.msg_header.command = strtoul(argv[2], NULL, 0);
    p_send_req->header.msg_header.payload_size = size;

    s_icc_base_send_params.payload_buffer = s_icc_mhu_send_payload;
    s_icc_base_send_params.buffer_size = sizeof(s_icc_mhu_send_payload);
    s_icc_base_send_params.cb = mhu_icc_base_send_complete_notify;
    s_icc_base_send_params.cb_ctx = s_icc_mhu_send_payload;

    // Send the payload
    fpfw_status_t status = fpfw_icc_base_send(cli_ctx->icc_base_ctx[index], &s_icc_base_send_params);
    FpFwCliPrint("ICC MHU send status: 0x%08x\n", status);

    s_pending_send_request = (status == FPFW_STATUS_SUCCESS);

    return CLI_SUCCESS;
}

FPFW_CLI_STATUS mhu_clear(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);

    memset(s_icc_mhu_recv_payload, 0, sizeof(s_icc_mhu_recv_payload));
    memset(&s_icc_base_recv_params, 0, sizeof(s_icc_base_recv_params));
    s_pending_recv_request = false;

    memset(s_icc_mhu_send_payload, 0, sizeof(s_icc_mhu_recv_payload));
    memset(&s_icc_base_send_params, 0, sizeof(s_icc_base_recv_params));
    s_pending_send_request = false;

    return CLI_SUCCESS;
}
