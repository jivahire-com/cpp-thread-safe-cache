//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_relay.c
 * This file handles routing of TRP messages between dies and cores.
 */

/*------------- Includes -----------------*/
#ifndef BEGIN_EXTERN_C
    #define BEGIN_EXTERN_C
#endif
#ifndef END_EXTERN_C
    #define END_EXTERN_C
#endif

#include "data_collection_protocol.h"
#include "data_collection_service.h"
#include "data_collection_service_i.h"
#include "dcs_events_i.h"
#include "telemetry_relay_protocol.h"
#include "tlm_relay_i.h"

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <fpfw_status.h>
#include <icc_mhu.h>
#include <idsw_kng.h>
#include <kng_icc_shared.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
p_trp_icc_config_t trp_icc_config;

/*------------- Functions ----------------*/
void tlm_relay_init(p_trp_icc_config_t icc_config)
{
    trp_icc_config = icc_config;
    bool found_dcp_endpoint = false;

    for (uint16_t index = 0; index < trp_icc_config->number_of_endpoints; index++)
    {
        p_trp_icc_endpoint_t icc_endpoint = &trp_icc_config->endpoint_table[index];
        if (icc_endpoint->icc_payload_protocol == ICC_COMMAND_DCP_MSG)
        {
            FPFW_RUNTIME_ASSERT_EXT(!found_dcp_endpoint, (uintptr_t)icc_endpoint, 0, 0, 0);
            found_dcp_endpoint = true;
        }

        memset(&icc_endpoint->async_recv_req, 0, sizeof(fpfw_icc_base_recv_req_t));
        icc_endpoint->async_recv_req.payload_buffer = icc_endpoint->async_recv_buffer;
        icc_endpoint->async_recv_req.buffer_size = icc_endpoint->async_recv_buffer_size;
        icc_endpoint->async_recv_req.recv_cmd_code = icc_endpoint->icc_payload_protocol;
        icc_endpoint->async_recv_req.cb = tlm_relay_icc_recv_complete_notify_from_drv_frmwk;
        icc_endpoint->async_recv_req.cb_ctx = icc_endpoint;

        icc_endpoint->seq_number.as_uint16 = 0;
        icc_endpoint->seq_number.die_id = (icc_config->this_die_id > 0) ? 1 : 0;
        icc_endpoint->seq_number.cpu_id = icc_config->this_cpu_id;
        icc_endpoint->seq_number.number = 1 << icc_config->this_cpu_id;

        fpfw_status_t status = fpfw_icc_base_recv(icc_endpoint->icc_base_ctx, &icc_endpoint->async_recv_req);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, (uintptr_t)icc_endpoint, 0, 0);
        // printf("Endpoint %s ICC Receive status: 0x%04lx\n", icc_endpoint->name, (uint32_t)status);
    }
}

bool tlm_relay_is_primary_instance(void)
{
    return (trp_icc_config->this_die_id == DIE_0) && (trp_icc_config->this_cpu_id == CPU_MCP);
}

bool tlm_relay_is_this_device(uint8_t die_id, uint8_t cpu_id)
{
    return (trp_icc_config->this_die_id == die_id) && (trp_icc_config->this_cpu_id == cpu_id);
}

uint8_t tlm_relay_get_this_die_id(void)
{
    return trp_icc_config->this_die_id;
}

uint8_t tlm_relay_get_this_cpu_id(void)
{
    return trp_icc_config->this_cpu_id;
}

// called from the driver framework thread
void tlm_relay_icc_recv_complete_notify_from_drv_frmwk(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    // context is valid whether or not the notification is successful
    p_trp_icc_endpoint_t icc_endpoint = (p_trp_icc_endpoint_t)context;
    fpfw_icc_base_recv_req_t* recv_context = &icc_endpoint->async_recv_req;

    uint16_t size_of_transport_hdr = sizeof(icc_mhu_header_t);

    if (FPFW_STATUS_SUCCEEDED(status))
    {
        if (recv_context->recv_cmd_code == ICC_COMMAND_DCP_MSG)
        {
            // only one DCP endpoint(asserted at init), which is on die 0, AP core
            p_dcp_msg_t dcp_msg = (p_dcp_msg_t)(recv_context->payload_buffer + size_of_transport_hdr);

            trp_msg_t trp_msg = {
                .hdr =
                    {
                        .source_die_id = DIE_0,
                        .source_cpu_id = CPU_AP,
                        .dest_die_id = trp_icc_config->this_die_id,
                        .dest_cpu_id = trp_icc_config->this_cpu_id,
                        .dcp_client_id = dcp_msg->hdr.client_id,
                        .trp_msg_id = TRP_MSG_ID_DCP_FORWARD,
                        .trp_msg_status = TRP_STATUS_SUCCESS,
                        .source_seq_num.as_uint16 = dcp_msg->hdr.seq_num,
                        .incoming_endpt = icc_endpoint,
                        .broadcast_type = TRP_BROADCAST_NONE,
                        .payload_size = dcp_msg->hdr.payload_size + sizeof(dcp_msg_hdr_t),
                    },
                .payload.dcp_msg = *dcp_msg,
            };

            dcs_forward_trp_msg_to_client_from_drv_frmwk(&trp_msg);
        }
        else if (recv_context->recv_cmd_code == ICC_COMMAND_TRP_MSG)
        {
            // all inter-core messages are TRP messages
            p_trp_msg_t trp_msg = (p_trp_msg_t)(recv_context->payload_buffer + size_of_transport_hdr);
            trp_msg->hdr.incoming_endpt = icc_endpoint; // prevent broadcasting back to the same endpoint

            if ((trp_msg->hdr.dest_die_id == trp_icc_config->this_die_id) &&
                (trp_msg->hdr.dest_cpu_id == trp_icc_config->this_cpu_id))
            {
                dcs_forward_trp_msg_to_client_from_drv_frmwk(trp_msg);
            }
            else
            {
                dcs_queue_for_outbound_from_drv_frmwk(trp_msg, false);
            }
        }
        else
        {
            FPFW_ET_LOG(TrpIccUnexpectedCmdCode, recv_context->recv_cmd_code);
            // invalid command code, do not re-register the endpoint
            return;
        }
    }
    else
    {
        FPFW_ET_LOG(TrpIccReceiveFail, status);
    }

    for (uint16_t index = 0; index < trp_icc_config->number_of_endpoints; index++)
    {
        // validate that the endpoint passed into this completion is a valid endpoint
        // if so, re-register the endpoint for the next receive
        p_trp_icc_endpoint_t registered_icc_endpoint = &trp_icc_config->endpoint_table[index];
        if (icc_endpoint == registered_icc_endpoint)
        {
            fpfw_status_t status = fpfw_icc_base_recv(icc_endpoint->icc_base_ctx, &icc_endpoint->async_recv_req);

            if (FPFW_STATUS_FAILED(status))
            {
                FPFW_ET_LOG(TrpIccReceiveRegisterFail, status, (uint32_t)icc_endpoint);
            }

            // printf("Endpoint %s ICC Re registered %ld\n", icc_endpoint->name, (uint32_t)status);
            return;
        }
    }

    FPFW_ET_LOG(TrpIccInvalidCallbackEndpoint, (uint32_t)icc_endpoint);
}

void tlm_relay_send_outgoing_msg(p_trp_msg_t trp_msg)
{
    bool log_route_error = false;

    // will be overriding the destination for broadcast messages
    // save and restore after sending to all routes
    uint8_t orig_dest_die_id = trp_msg->hdr.dest_die_id;
    uint8_t orig_dest_cpu_id = trp_msg->hdr.dest_cpu_id;

    for (uint16_t route = 0; route < trp_icc_config->number_of_routes; route++)
    {
        p_trp_route_t trp_route = &trp_icc_config->routing_table[route];

        if (trp_msg->hdr.broadcast_type == TRP_BROADCAST_NONE)
        {
            log_route_error = true;
            if ((trp_route->dest.die_id == trp_msg->hdr.dest_die_id) &&
                (trp_route->dest.cpu_id == trp_msg->hdr.dest_cpu_id))
            {
                tlm_relay_send_trp_via_icc(trp_msg, trp_route->icc_endpoint);
                return;
            }
        }
        else if (tlm_relay_should_broadcast_to_this_route(trp_msg, trp_route))
        {
            trp_msg->hdr.dest_die_id = trp_route->dest.die_id;
            trp_msg->hdr.dest_cpu_id = trp_route->dest.cpu_id;

            tlm_relay_send_trp_via_icc(trp_msg, trp_route->icc_endpoint);
        }
        // there may be no endpoints to broadcast to, so will not log error if route not found
    }

    trp_msg->hdr.dest_die_id = orig_dest_die_id;
    trp_msg->hdr.dest_cpu_id = orig_dest_cpu_id;

    if (log_route_error)
    {
        // if running single die for development, there isn't any error, the route isn't found and the message is dropped.
        // however suppress the error logging if single die and the destination is not the local die
        if ((trp_icc_config->is_dual_die) || (trp_msg->hdr.dest_die_id == trp_icc_config->this_die_id))
        {
            FPFW_ET_LOG(TrpIccNoRoute, trp_msg->hdr.dest_die_id, trp_msg->hdr.dest_cpu_id);
        }
    }
}

bool tlm_relay_should_broadcast_to_this_route(p_trp_msg_t trp_msg, p_trp_route_t trp_route)
{
    if (trp_msg->hdr.incoming_endpt == trp_route->icc_endpoint)
    {
        // never broadcast back to the sender
        return false;
    }

    bool broadcast = false;

    switch (trp_msg->hdr.broadcast_type)
    {
    case TRP_BROADCAST_PRIM_MCP_TO_SEC_MCP_ONLY:
        if (dcs_is_primary_instance() && (trp_route->dest.cpu_id == CPU_MCP))
        {
            broadcast = true;
        }
        break;

    case TRP_BROADCAST_TO_SEC_MCP_THEN_TO_SCP:
        if (dcs_is_primary_instance())
        {
            if (trp_route->dest.cpu_id == CPU_MCP)
            {
                // primary MCP broadcast to secondary MCP
                broadcast = true;
            }

            if (trp_route->dest.cpu_id == CPU_SCP)
            {
                // primary MCP broadcast to local SCP
                broadcast = true;
            }
        }
        else
        {
            if (trp_icc_config->this_cpu_id == CPU_MCP)
            {
                // this is a secondary MCP, broadcast to SCP if on this die
                if ((trp_route->dest.cpu_id == CPU_SCP) && (trp_route->dest.die_id == trp_icc_config->this_die_id))
                {
                    broadcast = true;
                }
            }
        }
        break;

    default:
        FPFW_ET_LOG(TrpInvalidBroadcastType, trp_msg->hdr.broadcast_type);
        break;
    }

    return broadcast;
}

void tlm_relay_send_trp_via_icc(p_trp_msg_t trp_msg, p_trp_icc_endpoint_t icc_endpoint)
{
    icc_msg_t icc_msg;
    if ((trp_msg->hdr.trp_msg_id == TRP_MSG_ID_DCP_FORWARD) && (trp_msg->hdr.dest_cpu_id == CPU_AP) &&
        (trp_msg->hdr.dest_die_id == trp_icc_config->this_die_id))
    {
        // sending DCP message to AP cpu on this die needs dcp messaging

        // if incoming is NULL, then new message requires a new sequence number
        // if not NULL, then reply or forward with the incoming sequence number
        if (trp_msg->hdr.incoming_endpt == NULL)
        {
            trp_msg->payload.dcp_msg.hdr.seq_num = tlm_relay_get_next_seq_num(icc_endpoint);
        }
        else
        {
            if ((trp_msg->hdr.incoming_endpt == icc_endpoint) && (trp_msg->hdr.source_seq_num.init_cmd == 0))
            {
                // nominal sequence, init_cmd is set by the sender when originating the message
                // the receiver processes the cmd and clears the init_cmd here to send the response
                // however, if the condition above is met, then this means the sender is sending
                // the same message to the receiver again. This is not expected behavior and could be possible
                // if there was a mal-formed message or a bug in the sender. The result would be an infinite loop
                // of the same message being sent back and forth. To prevent this, the message is dropped.
                FPFW_ET_LOG(DcsDroppedTrpMsg,
                            trp_msg->hdr.source_die_id,
                            trp_msg->hdr.source_cpu_id,
                            trp_msg->hdr.dest_die_id,
                            trp_msg->hdr.dest_cpu_id,
                            trp_msg->hdr.dcp_client_id,
                            trp_msg->hdr.trp_msg_id,
                            trp_msg->hdr.trp_msg_status,
                            GET_INT_CMD_BIT(trp_msg->hdr.source_seq_num.as_uint16),
                            GET_BASE_SEQ_NUM(trp_msg->hdr.source_seq_num.as_uint16)); // remove init_cmd bit

                return;
            }
            trp_msg->hdr.source_seq_num.init_cmd = 0;
        }

        icc_msg.header.msg_header.command = ICC_COMMAND_DCP_MSG;
        icc_msg.header.msg_header.payload_size = trp_msg->payload.dcp_msg.hdr.payload_size + sizeof(dcp_msg_hdr_t);
        icc_msg.dcp_msg = trp_msg->payload.dcp_msg;

        // debug level trace
        FPFW_ET_LOG(DcsDebugOutgoingDcpMsg,
                    trp_msg->payload.dcp_msg.hdr.client_id,
                    trp_msg->payload.dcp_msg.hdr.msg_id,
                    trp_msg->payload.dcp_msg.hdr.seq_num);
    }
    else
    {
        // if incoming is NULL, then new message requires a new sequence number
        // if not NULL, then reply or forward with the incoming sequence number
        if (trp_msg->hdr.incoming_endpt == NULL)
        {
            // diagnostic messages will retain special case sequence numbers
            if ((trp_msg->hdr.source_seq_num.as_uint16 & DIAG_SEQ_NUM_MASK) != DIAG_SEQ_NUM_CMD)
            {
                trp_msg->hdr.source_seq_num.as_uint16 = tlm_relay_get_next_seq_num(icc_endpoint);
            }
        }
        else
        {
            if ((trp_msg->hdr.incoming_endpt == icc_endpoint) && (trp_msg->hdr.source_seq_num.init_cmd == 0))
            {
                // nominal sequence, init_cmd is set by the sender when originating the message
                // the receiver processes the cmd and clears the init_cmd here to send the response
                // however, if the condition above is met, then this means the sender is sending
                // the same message to the receiver again. This is not expected behavior and could be possible
                // if there was a mal-formed message or a bug in the sender. The result would be an infinite loop
                // of the same message being sent back and forth. To prevent this, the message is dropped.
                FPFW_ET_LOG(DcsDroppedTrpMsg,
                            trp_msg->hdr.source_die_id,
                            trp_msg->hdr.source_cpu_id,
                            trp_msg->hdr.dest_die_id,
                            trp_msg->hdr.dest_cpu_id,
                            trp_msg->hdr.dcp_client_id,
                            trp_msg->hdr.trp_msg_id,
                            trp_msg->hdr.trp_msg_status,
                            GET_INT_CMD_BIT(trp_msg->hdr.source_seq_num.as_uint16),
                            GET_BASE_SEQ_NUM(trp_msg->hdr.source_seq_num.as_uint16)); // remove init_cmd bit

                return;
            }
            trp_msg->hdr.source_seq_num.init_cmd = 0;
        }

        icc_msg.header.msg_header.command = ICC_COMMAND_TRP_MSG;
        icc_msg.header.msg_header.payload_size = trp_msg->hdr.payload_size + sizeof(trp_msg_hdr_t);
        icc_msg.trp_msg = *trp_msg;

        // debug level trace
        FPFW_ET_LOG(DcsDebugOutgoingTrpMsg,
                    trp_msg->hdr.source_die_id,
                    trp_msg->hdr.source_cpu_id,
                    trp_msg->hdr.dest_die_id,
                    trp_msg->hdr.dest_cpu_id,
                    trp_msg->hdr.dcp_client_id,
                    trp_msg->hdr.trp_msg_id,
                    trp_msg->hdr.trp_msg_status,
                    GET_INT_CMD_BIT(trp_msg->hdr.source_seq_num.as_uint16),
                    GET_BASE_SEQ_NUM(trp_msg->hdr.source_seq_num.as_uint16)); // remove init_cmd bit
    }

    fpfw_status_t status = fpfw_icc_base_send_sync(icc_endpoint->icc_base_ctx,
                                                   &icc_msg,
                                                   icc_msg.header.msg_header.payload_size + sizeof(icc_mhu_header_t));
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(TrpIccSendFail, status, (uint32_t)icc_endpoint);
    }
}

uint16_t tlm_relay_get_next_seq_num(p_trp_icc_endpoint_t icc_endpoint)
{
    icc_endpoint->seq_number.number++;
    trp_seq_number_t next_seq_num;
    next_seq_num.as_uint16 = icc_endpoint->seq_number.as_uint16;
    next_seq_num.init_cmd = 1;

    return next_seq_num.as_uint16;
}
