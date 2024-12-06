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
#include <idsw_kng.h>
#include <kng_icc_shared.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static p_trp_icc_config_t s_trp_icc_config;

/*------------- Functions ----------------*/
void tlm_relay_init(p_trp_icc_config_t icc_config)
{
    s_trp_icc_config = icc_config;
    bool found_dcp_endpoint = false;

    for (uint16_t index = 0; index < s_trp_icc_config->number_of_endpoints; index++)
    {
        p_trp_icc_endpoint_t icc_endpoint = &s_trp_icc_config->endpoint_table[index];
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

        fpfw_status_t status = fpfw_icc_base_recv(icc_endpoint->icc_base_ctx, &icc_endpoint->async_recv_req);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, (uintptr_t)icc_endpoint, 0, 0);
        // printf("Endpoint %s ICC Receive status: 0x%04lx\n", icc_endpoint->name, (uint32_t)status);
    }
}

// called from the driver framework thread
void tlm_relay_icc_recv_complete_notify_from_drv_frmwk(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    // context is valid whether or not the notification is successful
    p_trp_icc_endpoint_t icc_endpoint = (p_trp_icc_endpoint_t)context;
    fpfw_icc_base_recv_req_t* recv_context = &icc_endpoint->async_recv_req;

    if (FPFW_STATUS_SUCCEEDED(status))
    {
        if (recv_context->recv_cmd_code == ICC_COMMAND_DCP_MSG)
        {
            // only one DCP endpoint(asserted at init), which is on die 0, AP core
            p_dcp_msg_t dcp_msg = (p_dcp_msg_t)recv_context->payload_buffer;

            trp_msg_t trp_msg = {
                .hdr =
                    {
                        .source_die_id = DIE_0,
                        .source_cpu_id = CPU_AP,
                        .dest_die_id = s_trp_icc_config->this_die_id,
                        .dest_cpu_id = s_trp_icc_config->this_cpu_id,
                        .dcp_client_id = dcp_msg->hdr.client_id,
                        .trp_msg_id = TRP_MSG_ID_DCP_FORWARD,
                        .trp_msg_status = TRP_STATUS_SUCCESS,
                        .source_seq_num = dcp_msg->hdr.seq_num,
                    },
                .payload.dcp_msg = *dcp_msg,
            };

            dcs_forward_trp_msg_to_client_from_drv_frmwk(&trp_msg);
        }
        else if (recv_context->recv_cmd_code == ICC_COMMAND_TRP_MSG)
        {
            // all inter-core messages are TRP messages
            p_trp_msg_t trp_msg = (p_trp_msg_t)recv_context->payload_buffer;
            dcs_forward_trp_msg_to_client_from_drv_frmwk(trp_msg);
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

    for (uint16_t index = 0; index < s_trp_icc_config->number_of_endpoints; index++)
    {
        // validate that the endpoint passed into this completion is a valid endpoint
        // if so, re-register the endpoint for the next receive
        p_trp_icc_endpoint_t registered_icc_endpoint = &s_trp_icc_config->endpoint_table[index];
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