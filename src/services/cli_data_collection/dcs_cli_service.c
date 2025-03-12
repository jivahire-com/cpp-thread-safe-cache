//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcs_cli_service.c
 * Implementation of the Data Collection Service CLI handling.
 */

/*------------- Includes -----------------*/

#include "FpFwLinkedList.h" // for NULL_LIST_ENTRY
#include "fpfw_status.h"    // for FPFW_STATUS_SUCCEEDED, fpf...

#include <FpFwCli.h>   // for FpFwCliPrint, FPFW_CLI_COM...
#include <FpFwUtils.h> // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <data_collection_service.h>
#include <data_collection_service_i.h>
#include <stdbool.h> // for bool
#include <stdint.h>  // for uint32_t, uint16_t
#include <stdio.h>   // for NULL
#include <stdlib.h>  // for strtoul
#include <telemetry_relay_protocol.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static FPFW_CLI_STATUS trp_send(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND cli_dcs_commands[] = {
    {NULL_LIST_ENTRY, "dcs", "trp_send", trp_send, "Send TRP Message: byte little endian", "Usage: trp_send <byte_0> ... <byte_n>"}};

/*------------- Functions ----------------*/
void trp_diag_handler(p_trp_msg_t trp_msg);

void dcs_cli_svc_initialize(void)
{
    FpFwCliRegisterTable(&cli_dcs_commands[0], FPFW_ARRAY_SIZE(cli_dcs_commands));
}

static FPFW_CLI_STATUS trp_send(int Argc, const char** Argv)
{
    if (Argc < (int)(sizeof(trp_msg_hdr_t) + 1))
    {
        FpFwCliPrint("\nERROR:: %s\n", cli_dcs_commands[0].Usage);
        FpFwCliPrint("Size must be at least the size of the trp header\n");
        return CLI_ERROR;
    }

    uint16_t payload_size = strtoul(Argv[18], NULL, 0) << 8 | strtoul(Argv[19], NULL, 0);
    uint16_t total_size = sizeof(trp_msg_hdr_t) + payload_size;

    if (Argc != total_size + 1)
    {
        FpFwCliPrint("\nERROR:: %s\n", cli_dcs_commands[0].Usage);
        FpFwCliPrint("Size must be equal to the total size of the trp message, args=%d, cmd size=%d\n", Argc, total_size);
        return CLI_ERROR;
    }
    trp_msg_t send_msg;
    uint8_t* byte_msg = (uint8_t*)&send_msg;

    uint16_t byte_val;
    for (int i = 0; i < total_size; i++)
    {
        byte_val = strtoul(Argv[i + 1], NULL, 0);
        if (byte_val > UINT8_MAX)
        {
            FpFwCliPrint("\nERROR:: Byte value out of range: index %d, value %d\n", i, byte_val);
            return CLI_ERROR;
        }
        byte_msg[i] = (uint8_t)byte_val;
    }

    send_msg.hdr.source_seq_num.init_cmd = 1;
    send_msg.hdr.source_seq_num.cpu_id = DIAG_SEQ_NUM_CPU;
    send_msg.hdr.source_seq_num.die_id = DIAG_SEQ_NUM_DIE;
    send_msg.hdr.incoming_endpt = NULL;

    dcs_register_diag_trp_handler(trp_diag_handler);

    dcs_client_send_new_trp_msg(&send_msg);

    return CLI_SUCCESS;
}

void trp_diag_handler(p_trp_msg_t trp_msg)
{
    uint16_t total_bytes = trp_msg->hdr.payload_size + sizeof(trp_msg_hdr_t);
    uint8_t* byte_msg = (uint8_t*)trp_msg;

    FpFwCliPrint("\nRsp: ");
    for (uint16_t i = 0; i < total_bytes; i++)
    {
        FpFwCliPrint("%d ", byte_msg[i]);
    }
    FpFwCliPrint("\nTrpRx\n");
}
