//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file scmi_prim.c
 *  This modules supports scmi functions as primitives
 */

/*------------- Includes -----------------*/
#include "scmi_init.h"
#include "scmi_prim_i.h"

#include <DfwkClient.h>
#include <FpFwAssert.h>
#include <FpFwCli.h> // for SCMI_LOG_INFO, _FPFW_CLI_STATUS, FPF...
#include <ap_core.h>
#include <arm_intrinsic.h>
#include <atu_api.h>
#include <cmsis_m7.h>
#include <debug.h>
#include <fpfw_icc_transport_interface.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <icc_mhu.h>
#include <inttypes.h>
#include <kng_icc_shared.h>
#include <kng_scmi_shared.h>
#include <kng_scp_tfa_shared.h>
#include <mhu_icc_transport.h>
#include <scmi_prim.h>
#include <stdint.h>
#include <string.h>

#define BIT_32(nr) (((uint32_t)(1U)) << (nr))
#define SMT_CHANNEL_FREE \
    BIT_32(0) //! Set 0th bit of status to indicate channel is free ie when SMT does not contain pending message

#define SET_SCMI_SMT_CHANNEL_FREE(status) ((status) |= BIT_32(0))
#define SCMI_SMT_IS_CHANNEL_FREE(status)  ((status) & BIT_32(0))

// Local data structures
typedef struct
{
    smt_header_t smt_header;
    scmi_message_header_t header;
    uint32_t payload[12];
} scmi_local_packet_t;

typedef struct
{
    icc_mhu_header_t header;
    uint8_t data[64];
} scmi_transport_message_t;

static DFWK_INTERFACE_HEADER* p_apcore_interface = NULL;
static ap_core_asynchronous_request_t apcore_request;
static uint8_t scmi_debug = 0x0;

static mhu_icc_transport_intrf_t* s_icc_mscp2tfa = NULL;
static volatile scmi_transport_message_t* scmi_recv_message;
static volatile scmi_transport_message_t* scmi_send_message;
static FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST recv_request;

void scmi_set_debug_mode(uint8_t mode)
{
    scmi_debug = mode;
}

void ap_core_power_complete(int32_t status)
{
    scmi_pd_power_state_notify_p2a_t resp;
    resp.status = status;
    scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
    SCMI_LOG_INFO("Sent SCMI_PWR_STATE_SET_MSG response: %d\n", (int)resp.status);
}

void ap_core_power_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(p_completion_context);
    ap_core_power_complete(SCMI_STATUS_SUCCESS);
}

void ap_core_power(uint32_t power_domain, uint32_t power_state)
{
    // ensure the apcore interface was set
    FPFW_RUNTIME_ASSERT(p_apcore_interface != NULL);

    DfwkAsyncRequestInitialize(&apcore_request.header, sizeof(apcore_request));
    if ((power_state & SCMI_PD_CORE_STATE_MASK) == (SCMI_PD_CORE_STATE_OFF))
    {
        ap_core_core_power_off(p_apcore_interface, &apcore_request, power_domain, ap_core_power_completion, NULL);
    }
    else if ((power_state & SCMI_PD_CORE_STATE_MASK) == (SCMI_PD_CORE_STATE_ON))
    {
        ap_core_core_power_on(p_apcore_interface, &apcore_request, power_domain, ap_core_power_completion, NULL);
    }
    else
    {
        SCMI_LOG_INFO("Invalid Power State: %" PRIx32 "\n", power_state);
        ap_core_power_complete(SCMI_STATUS_INVALID_PARAMETERS);
    }
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
    case SCMI_PROTO_MSG_ATTR_MSG: {
        scmi_protocol_message_attributes_a2p_t* p_message_attr = (scmi_protocol_message_attributes_a2p_t*)payload;
        SCMI_LOG_INFO("SCMI_PROTO_ATTR_MSG: %x message_id %" PRIx32 "\n", cmd_code, p_message_attr->message_id);
        scmi_protocol_message_attributes_p2a_t resp;
        resp.status = SCMI_STATUS_SUCCESS;
        resp.attributes = 0;
        switch (p_message_attr->message_id)
        {
        // supported messages here
        case SCMI_PWR_STATE_SET_MSG:
            SCMI_LOG_INFO("SCMI_PWR_STATE_SET_MSG\n");
            break;
        // everything else is unsupported
        default:
            resp.status = SCMI_STATUS_NOT_FOUND;
            break;
        }
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: returning %" PRId32 " (attr: %" PRIx32 ")\n", resp.status, resp.attributes);
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTO_MSG_ATTR_MSG, (uint8_t*)&resp, sizeof(resp));
        break;
    }
    case SCMI_PWR_STATE_SET_MSG: {
        SCMI_LOG_INFO("SCMI_PWR_STATE_SET_MSG: %x\n", cmd_code);
        scmi_pd_power_state_set_a2p_t power_state;
        // because alignment :(
        memcpy(&power_state, payload, sizeof(power_state));
        ap_core_power(power_state.domain_id, power_state.power_state);
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
    case SCMI_PROTO_MSG_ATTR_MSG: {
        scmi_protocol_message_attributes_a2p_t* p_message_attr = (scmi_protocol_message_attributes_a2p_t*)payload;
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: %x message_id %" PRIx32 "\n", cmd_code, p_message_attr->message_id);
        scmi_protocol_message_attributes_p2a_t resp;
        resp.status = SCMI_STATUS_SUCCESS;
        resp.attributes = 0;
        switch (p_message_attr->message_id)
        {
        // supported messages here
        case SCMI_SYS_PWR_STATE_SET_MSG:
            break;
        // everything else is unsupported
        default:
            resp.status = SCMI_STATUS_NOT_FOUND;
            break;
        }
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: returning %" PRId32 " (attr: %" PRIx32 ")\n", resp.status, resp.attributes);
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTO_MSG_ATTR_MSG, (uint8_t*)&resp, sizeof(resp));
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

void ap_core_reset_addr_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(p_completion_context);
    scmi_apcore_reset_address_set_p2a_t resp;
    resp.status = SCMI_STATUS_SUCCESS;
    scmi_send_resp(SCMI_AP_CORE_PROTO_ID, SCMI_AP_CORE_RESET_ADDR_SET_MSG, (uint8_t*)&resp, sizeof(resp));
    SCMI_LOG_INFO("Sent SCMI_AP_CORE_RESET_ADDR_SET_MSG response: %d\n", (int)resp.status);
}

void ap_core_reset_addr_set(uint64_t reset_address)
{
    // ensure the apcore interface was set
    FPFW_RUNTIME_ASSERT(p_apcore_interface != NULL);

    DfwkAsyncRequestInitialize(&apcore_request.header, sizeof(apcore_request));
    ap_core_set_rvbaraddr(p_apcore_interface, &apcore_request, reset_address, ap_core_reset_addr_completion, NULL);
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
    case SCMI_PROTO_MSG_ATTR_MSG: {
        scmi_protocol_message_attributes_a2p_t* p_message_attr = (scmi_protocol_message_attributes_a2p_t*)payload;
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: %x message_id %" PRIx32 "\n", cmd_code, p_message_attr->message_id);
        scmi_protocol_message_attributes_p2a_t resp;
        resp.status = SCMI_STATUS_SUCCESS;
        resp.attributes = 0;
        switch (p_message_attr->message_id)
        {
        // supported messages here
        case SCMI_AP_CORE_RESET_ADDR_SET_MSG:
            break;
        // everything else is unsupported
        default:
            resp.status = SCMI_STATUS_NOT_FOUND;
            break;
        }
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: returning %d (attr: %" PRIx32 ")\n", (int)resp.status, resp.attributes);
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTO_MSG_ATTR_MSG, (uint8_t*)&resp, sizeof(resp));
        break;
    }
    case SCMI_AP_CORE_RESET_ADDR_SET_MSG: {
        scmi_apcore_reset_address_set_a2p_t reset_address;
        memcpy(&reset_address, payload, sizeof(reset_address));
        SCMI_LOG_INFO("SCMI_AP_CORE_RESET_ADDR_SET_MSG: %x high: %08" PRIx32 " low: %08" PRIx32
                      " attr: %08" PRIx32 "\n",
                      cmd_code,
                      reset_address.reset_address_high,
                      reset_address.reset_address_low,
                      reset_address.attributes);
        ap_core_reset_addr_set(((uint64_t)reset_address.reset_address_high << 32) | reset_address.reset_address_low);
    }
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

int scmi_send_resp(uint8_t protocol_id, uint8_t cmd_id, uint8_t* payload, size_t size)
{
    scmi_send_message->header.msg_header.command = ICC_SCMI_CLIENT_MOD_RESP;
    scmi_send_message->header.msg_header.payload_size = sizeof(scmi_local_packet_t);
    // Send the SCMI Response
    scmi_local_packet_t* local_packet = (scmi_local_packet_t*)scmi_send_message->data;
    local_packet->smt_header.flags = 1;
    local_packet->smt_header.status = 1;
    local_packet->smt_header.payload_size = size + sizeof(local_packet->header);
    local_packet->header.protocol_id = protocol_id;
    local_packet->header.msg_type = cmd_id;
    local_packet->header.token = 0;

    memcpy(local_packet->payload, payload, size);
    __DSB();

    fpfw_status_t status = fpfw_icc_transport_try_send_sync_req(&(s_icc_mscp2tfa->base_interface),
                                                                (void*)scmi_send_message,
                                                                sizeof(scmi_transport_message_t));

    return status;
}

void scmi_set_apcore_interface(DFWK_INTERFACE_HEADER* p_interface)
{
    // save the interface off for later use
    p_apcore_interface = p_interface;
    DfwkClientInterfaceOpen(p_interface);
}

static void scmi_async_recv_completion(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    FPFW_UNUSED(Context);
    FPFW_RUNTIME_ASSERT(NULL != Request);

    PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST request = (PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST)Request;
    volatile scmi_transport_message_t* recv_msg = (volatile scmi_transport_message_t*)request->Input.PayloadBuffer;
    volatile scmi_local_packet_t* local_packet = (volatile scmi_local_packet_t*)recv_msg->data;

    SCMI_LOG_INFO("SCMI Async Recv Completion Raised!\n");
    //! Ensure that the channel is busy here, tfa marks the scmi channel busy
    //! before it sends icc message to scp
    FPFW_RUNTIME_ASSERT(!SCMI_SMT_IS_CHANNEL_FREE(local_packet->smt_header.status));

    //! Check if the request was successful & received some bytes
    if ((request->Output.Status == FPFW_ICC_TRANSPORT_STATUS_SUCCESS) && (request->Output.ReceivedBytes > 0))
    {
        //! Check if the message is a SCMI message
        if (recv_msg->header.msg_header.command == ICC_SCMI_HOST_MODULE_SEND)
        {
            //! Update the scmi status bit to free channel, pseudo ack to the tfa
            //! tfa will spin until the channel is free before it proceeds to loop until mesg received
            SET_SCMI_SMT_CHANNEL_FREE(local_packet->smt_header.status);
            __DSB();

            //! spew the messages if debug is enabled
            if ((scmi_debug & 2) != 0)
            {
                SCMI_LOG_INFO("SCMI ICC Message: %x\n", (int)scmi_recv_message->header.msg_header.command);
                SCMI_LOG_INFO("SCMI ICC Size: %x\n", (int)scmi_recv_message->header.msg_header.payload_size);

                if (scmi_recv_message->header.msg_header.payload_size != 0 && (scmi_debug & 8) != 0)
                {
                    for (uint8_t data_count = 0; data_count < scmi_recv_message->header.msg_header.payload_size; data_count++)
                    {
                        SCMI_LOG_INFO("  scmi_message data[%d]: %x\n", data_count, scmi_recv_message->data[data_count]);
                    }
                }
            }
            //! Parse the message as per SCMI protocol
            scmi_check_message((scmi_icc_packet_t*)&(recv_msg->data));
        }
        else
        {
            //! Unexpected! Only SCMI message command expected on scp tfa icc interface!
            FPFW_RUNTIME_ASSERT(0);
        }
    }
    else
    {
        //! Unexpected! Status failed or no bytes received
        FPFW_RUNTIME_ASSERT(0);
    }

    //! respawn a new async recv request, always keep an async recv request alive
    fpfw_status_t status = fpfw_icc_transport_recv_async_req(&(s_icc_mscp2tfa->base_interface),
                                                             &recv_request.Header,
                                                             (void*)scmi_recv_message,
                                                             sizeof(scmi_transport_message_t),
                                                             scmi_async_recv_completion,
                                                             NULL);
    FPFW_RUNTIME_ASSERT(status == FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
}

void scmi_drv_init(DFWK_INTERFACE_HEADER* p_scp_tfa_interface)
{
    //! Initialize the scp_tfa interface & scmi send/recv buffer
    FPFW_RUNTIME_ASSERT(p_scp_tfa_interface != NULL);
    s_icc_mscp2tfa = (mhu_icc_transport_intrf_t*)p_scp_tfa_interface;
    scmi_recv_message = (volatile scmi_transport_message_t*)s_icc_mscp2tfa->device->recv_channel.ch_shared_mem_addr;
    scmi_send_message = (volatile scmi_transport_message_t*)s_icc_mscp2tfa->device->send_channel.ch_shared_mem_addr;

    //! Initialize a request to be sent to the mbox icc transport interface
    DfwkAsyncRequestInitialize(&recv_request.Header, sizeof(FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST));

    //! Set the recv channel status bit to reflect channel is free (set to 1), required to recv the very 1st message
    volatile scmi_local_packet_t* local_packet = (volatile scmi_local_packet_t*)scmi_recv_message->data;
    SET_SCMI_SMT_CHANNEL_FREE(local_packet->smt_header.status);
    __DSB();

    //! Spawn the 1st async recv request to receive messages over scp_tfa interface
    fpfw_status_t status = fpfw_icc_transport_recv_async_req(&(s_icc_mscp2tfa->base_interface),
                                                             &recv_request.Header,
                                                             (void*)scmi_recv_message,
                                                             sizeof(scmi_transport_message_t),
                                                             scmi_async_recv_completion,
                                                             NULL);
    FPFW_RUNTIME_ASSERT(status == FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
}