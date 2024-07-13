//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file scmi_prim.c
 *  This modules supports scmi functions as primitives
 */

/*------------- Includes -----------------*/
#include "scmi_prim_i.h"

#include <FpFwAssert.h>
#include <FpFwCli.h> // for FpFwCliPrint, _FPFW_CLI_STATUS, FPF...
#include <debug.h>
#include <icc_mhu_trans_prim.h>
#include <kng_icc_shared.h>
#include <kng_scmi_shared.h>
#include <scmi_prim.h>
#include <string.h>

// Local data structures
typedef struct
{
    smt_header_t smt_header;
    scmi_message_header_t header;
    uint32_t payload[12];
} scmi_local_packet_t;

typedef struct
{
    uint32_t command;
    uint16_t size;
    uint8_t data[64];
} scmi_transport_message_t;

static scmi_transport_message_t scmi_message;
static uint8_t scmi_debug = 0;
void scmi_set_debug_mode(uint8_t mode)
{
    scmi_debug = mode;
}

int scmi_power_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size)
{
    int status = SCMI_PROTOCOL_CMD_SUCCESS;

    SCMI_LOG_INFO("Processing SCMI Power Protocol Cmd: %x\n", cmd_code);
    if (size != 0 && ((scmi_debug & 0x04) != 0))
    {
        for (uint8_t data_count = 0; data_count < size; data_count++)
        {
            SCMI_LOG_INFO(" scmi_message data[%d]: %x\n", data_count, payload[data_count]);
        }
    }

    // Call the Power domian request from here if the command is valid
    switch (cmd_code)
    {

    case SCMI_PROTOCOL_VERSION: {
        scmi_protocol_version_resp_t resp;
        resp.status = 0;
        resp.version = 0x2000;
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTOCOL_VERSION, (uint8_t*)&resp, sizeof(resp));
        SCMI_LOG_INFO("SCMI_PWR_PROTOCOL_VERSION: %x\n", cmd_code);
        break;
    }
    case SCMI_PWR_STATE_SET_MSG: {
        SCMI_LOG_INFO("SCMI_PWR_STATE_SET_MSG: %x\n", cmd_code);
        int32_t status = 0;
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&status, sizeof(status));
        break;
    }
    case SCMI_PWR_STATE_GET_MSG:
        SCMI_LOG_INFO("SCMI_PWR_STATE_GET_MSG: %x\n", cmd_code);
        scmi_pwr_domain_get_state_resp_t resp;
        resp.status = 0;
        resp.power_state = 1;
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PWR_STATE_GET_MSG, (uint8_t*)&resp, sizeof(resp));
        break;

    default:
        SCMI_LOG_INFO("Unknown Power Domain Protocol Command: %x \n", cmd_code);
        status = SCMI_PROTOCOL_CMD_UKNOWN;
        break;
    }

    return status;
}

int scmi_sys_pwr_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size)
{
    int status = SCMI_PROTOCOL_CMD_SUCCESS;
    SCMI_LOG_INFO("Processing SCMI Power Protocol Cmd: %x\n", cmd_code);
    if (size != 0 && ((scmi_debug & 0x04) != 0))
    {
        for (uint8_t data_count = 0; data_count < size; data_count++)
        {
            SCMI_LOG_INFO(" scmi_message data[%d]: %x\n", data_count, payload[data_count]);
        }
    }

    // Call the Sys Power request from here if the command is valid
    switch (cmd_code)
    {

    case SCMI_PROTOCOL_VERSION: {
        scmi_protocol_version_resp_t resp;
        resp.status = 0;
        resp.version = 0x2000;
        scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PROTOCOL_VERSION, (uint8_t*)&resp, sizeof(resp));
        SCMI_LOG_INFO("SCMI_SYS_PWR_PROTOCOL_VERSION: %x\n", cmd_code);
        break;
    }
    case SCMI_SYS_PWR_STATE_SET_MSG: {
        SCMI_LOG_INFO("SCMI_SYS_PWR_STATE_SET_MSG: %x\n", cmd_code);
        int32_t status = 0;
        scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&status, sizeof(status));
        break;
    }
    case SCMI_SYS_PWR_STATE_GET_MSG: {
        SCMI_LOG_INFO("SCMI_SYS_PWR_STATE_GET_MSG: %x\n", cmd_code);

        // As an example, return system power up 0x3 as the system state
        scmi_sys_pwr_get_state_resp_t resp;
        resp.status = 0;
        resp.system_state = 3;
        scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_SYS_PWR_STATE_GET_MSG, (uint8_t*)&resp, sizeof(resp));
        break;
    }
    default:
        SCMI_LOG_INFO("Unknown Sys Power Command: %x \n", cmd_code);
        status = SCMI_PROTOCOL_CMD_UKNOWN;
        break;
    }

    return status;
}

int scmi_ap_core_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size)
{
    int status = SCMI_PROTOCOL_CMD_SUCCESS;
    // Parse the command code
    SCMI_LOG_INFO("Processing AP Core Protocol Cmd: %d\n", cmd_code);
    if (size != 0 && ((scmi_debug & 0x04) != 0))
    {
        for (uint8_t data_count = 0; data_count < size; data_count++)
        {
            SCMI_LOG_INFO(" scmi_message data[%d]: %x\n", data_count, payload[data_count]);
        }
    }

    // Call the  AP Core request from here if the command is valid
    switch (cmd_code)
    {
    case SCMI_PROTOCOL_VERSION: {
        scmi_protocol_version_resp_t resp;
        resp.status = 0;
        resp.version = 0x2000;
        scmi_send_resp(SCMI_AP_CORE_PROTO_ID, SCMI_PROTOCOL_VERSION, (uint8_t*)&resp, sizeof(resp));
        SCMI_LOG_INFO("SCMI_AP_CORE_PROTOCOL_VERSION: %x\n", cmd_code);
        break;
    }
    case SCMI_AP_CORE_RESET_ADDR_SET_MSG:
        SCMI_LOG_INFO("SCMI_AP_CORE_RESET_ADDR_SET_MSG: %x\n", cmd_code);
        break;

    case SCMI_AP_CORE_RESET_ADDR_GET_MSG:
        SCMI_LOG_INFO("SCMI_AP_CORE_RESET_ADDR_GET_MSG: %x\n", cmd_code);
        break;

    default:
        SCMI_LOG_INFO("Unknown AP Core Command: %x \n", cmd_code);
        status = SCMI_PROTOCOL_CMD_UKNOWN;
        break;
    }
    return status;
}

int scmi_check_message(scmi_icc_packet_t* packet)
{
    int status = SCMI_PROTOCOL_CMD_SUCCESS;
    if ((scmi_debug & 1) != 0)
    {
        // Spew SCMI message
        SCMI_LOG_INFO("SMT Status      : %d\n", (int)packet->smt_header.status);
        SCMI_LOG_INFO("SMT Flags       : %d\n", (int)packet->smt_header.flags);
        SCMI_LOG_INFO("SMT Payload Size: %d\n", (int)packet->smt_header.payload_size);
        SCMI_LOG_INFO("SCMI Protocol ID: 0x %x\n", (int)packet->header.protocol_id);
        SCMI_LOG_INFO("SCMI Message  ID: 0x %x\n", (int)packet->header.cmd_code);
    }

    // Check for the appropriate action
    switch (packet->header.protocol_id)
    {

    case SCMI_PWR_DMN_PROTO_ID:
        status = scmi_power_protocol_cmds(packet->header.cmd_code,
                                          (uint8_t*)packet->payload,
                                          (packet->smt_header.payload_size - sizeof(packet->header)));
        break;

    case SCMI_SYS_PWR_PROTO_ID:
        status = scmi_sys_pwr_protocol_cmds(packet->header.cmd_code,
                                            (uint8_t*)packet->payload,
                                            (packet->smt_header.payload_size - sizeof(packet->header)));
        break;

    case SCMI_AP_CORE_PROTO_ID:
        status = scmi_ap_core_protocol_cmds(packet->header.cmd_code,
                                            (uint8_t*)packet->payload,
                                            (packet->smt_header.payload_size - sizeof(packet->header)));
        break;

    default:
        SCMI_LOG_INFO("SCMI Protocol  ID not supported \n");
        status = SCMI_PROTOCOL_NOT_SUPPORTED;
        break;
    }

    return status;
}

int scmi_poll_message()
{
    scmi_message.command = ICC_SCMI_HOST_MODULE_SEND;
    scmi_message.size = sizeof(scmi_message.data);
    int status = icc_mhu_trans_get_msg_from_index(ICC_MHU_TRANS_SCMI_RECEIVE_INDEX,
                                                  (icc_mhu_transport_message_t*)&scmi_message);
    if (status == ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE)
    {
        if ((scmi_debug & 2) != 0)
        {
            SCMI_LOG_INFO("SCMI ICC Message: %x\n", (int)scmi_message.command);
            SCMI_LOG_INFO("SCMI ICC Size: %x\n", (int)scmi_message.size);

            if (scmi_message.size != 0 && (scmi_debug & 8) != 0)
            {
                for (uint8_t data_count = 0; data_count < scmi_message.size; data_count++)
                {
                    SCMI_LOG_INFO("  scmi_message data[%d]: %x\n", data_count, scmi_message.data[data_count]);
                }
            }
        }

        // Process the message
        scmi_check_message((scmi_icc_packet_t*)scmi_message.data);
    }
    return status;
}

int scmi_send_resp(uint8_t protocol_id, uint8_t cmd_id, uint8_t* payload, size_t size)
{

    // Send the SCMI Response
    static scmi_local_packet_t local_packet;
    local_packet.smt_header.flags = 1;
    local_packet.smt_header.status = 1;
    local_packet.smt_header.payload_size = size + sizeof(local_packet.header);
    local_packet.header.protocol_id = protocol_id;
    local_packet.header.msg_type = cmd_id;

    memcpy(local_packet.payload, payload, size);
    size_t icc_size = local_packet.smt_header.payload_size + sizeof(local_packet.smt_header);
    return icc_mhu_trans_send_message_idx(ICC_MHU_TRANS_SCMI_RESP_INDEX, ICC_SCMI_CLIENT_MOD_RESP, (uint8_t*)&local_packet, icc_size);
}