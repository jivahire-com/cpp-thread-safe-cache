//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mts_platform.c
 * Implementations for the Message Transfer Service platform specific functions
 */

/*------------- Includes -----------------*/

#ifndef BEGIN_EXTERN_C
    #define BEGIN_EXTERN_C
#endif
#ifndef END_EXTERN_C
    #define END_EXTERN_C
#endif

#include "data_collection_protocol.h"
#include "message_transfer_service.h"
#include "message_transfer_service_i.h"
#include "mts_events_i.h"
#include "mts_platform_definitions.h"
#include "mts_platform_implementation_i.h"
#include "mts_platform_specialization.h"
#include "transfer_relay_i.h"
#include "transfer_relay_protocol.h"

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <assert.h>
#include <fpfw_status.h>
#include <icc_mhu.h>
#include <icc_platform_defines.h>
#include <idsw_kng.h>
#include <kng_icc_shared.h>
#include <mscp_uefi_shared_ddrss.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/**
 * Maximum number of retries for ICC send operations.
 * Retries help recover from transient timeout errors (e.g., FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR)
 * when the receiving endpoint is temporarily busy or unresponsive.
 */
#ifndef MTS_ICC_SEND_MAX_RETRIES
    #define MTS_ICC_SEND_MAX_RETRIES (3)
#endif

/**
 * Delay in milliseconds between ICC send retries.
 * This non-blocking delay allows other threads to run and gives the
 * receiving endpoint time to recover before the next retry attempt.
 */
#ifndef MTS_ICC_SEND_RETRY_DELAY_MS
    #define MTS_ICC_SEND_RETRY_DELAY_MS (10)
#endif

/* Convert milliseconds to ThreadX ticks (assuming TX_TIMER_TICKS_PER_SECOND is defined) */
#define MTS_MS_TO_TX_TICKS(ms) (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)

/* Ensure the retry delay converts to at least 1 ThreadX tick */
static_assert(MTS_MS_TO_TX_TICKS(MTS_ICC_SEND_RETRY_DELAY_MS) > 0, "Retry delay must convert to at least 1 tick");

static_assert(CPU_AP == MTS_PLATFORM_CORE_AP, "CPU core mismatch");
static_assert(CPU_SCP == MTS_PLATFORM_CORE_SCP, "CPU core mismatch");
static_assert(CPU_MCP == MTS_PLATFORM_CORE_MCP, "CPU core mismatch");
static_assert(CPU_HSP == MTS_PLATFORM_CORE_HSP, "CPU core mismatch");
static_assert(CPU_TBP == MTS_PLATFORM_CORE_TBP, "CPU core mismatch");
static_assert(CPU_SDM == MTS_PLATFORM_CORE_SDM, "CPU core mismatch");
static_assert(CPU_CDED_SDM == MTS_PLATFORM_CORE_CDED_SDM, "CPU core mismatch");
static_assert(CPU_CDED_KMP == MTS_PLATFORM_CORE_CDED_KMP, "CPU core mismatch");

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void mts_platform_get_transport_info(uint32_t transport_type, p_mts_platform_transport_info_t info)
{

    switch (transport_type)
    {
    case TRP_TRANSPORT_TYPE_ICC_ARM_MHU:
        info->header_size = sizeof(icc_mhu_header_t);
        info->icc_dcp_command = ICC_COMMAND_DCP_MSG;
        info->icc_trp_command = ICC_COMMAND_TRP_MSG;
        break;

    case TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX:
        info->header_size = sizeof(large_fifo_mailbox_msg_header);
        info->icc_dcp_command = LARGE_FIFO_MAILBOX_MSG_DCP;
        info->icc_trp_command = LARGE_FIFO_MAILBOX_MSG_TRP;
        break;

    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

void mts_platform_send_msg_via_transport(p_trp_msg_t trp_msg, p_trp_endpoint_t trp_endpoint)
{
    icc_trp_msg_t outgoing_full_msg;
    p_trp_icc_endpoint_t trp_icc_endpoint = (p_trp_icc_endpoint_t)trp_endpoint;

    // fill in header for both transports, although only one will be used
    large_fifo_mailbox_msg_header large_mbox_hdr = {0};
    icc_mhu_header_t icc_mhu_hdr = {0};

    if ((trp_msg->hdr.trp_msg_id == TRP_MSG_ID_DCP_FORWARD) && (trp_msg->hdr.dest_node.core_id == MTS_PLATFORM_CORE_AP) &&
        (trp_msg->hdr.dest_node.die_id == transfer_rly_get_this_die_id()))
    {
        // sending DCP message to AP cpu on this die needs dcp messaging
        if (!transfer_rly_should_send_dcp_msg(trp_msg, trp_icc_endpoint))
        {
            return;
        }

        icc_mhu_hdr.msg_header.command = ICC_COMMAND_DCP_MSG;
        icc_mhu_hdr.msg_header.payload_size = trp_msg->payload.dcp_msg.hdr.payload_size + sizeof(dcp_msg_hdr_t);
        large_mbox_hdr.seq = (uint8_t)trp_msg->hdr.source_seq_num.number;
        large_mbox_hdr.cmd = LARGE_FIFO_MAILBOX_MSG_DCP;
    }
    else
    {
        if (!transfer_rly_should_send_trp_msg(trp_msg, trp_icc_endpoint))
        {
            return;
        }

        icc_mhu_hdr.msg_header.command = ICC_COMMAND_TRP_MSG;
        icc_mhu_hdr.msg_header.payload_size = trp_msg->hdr.payload_size + sizeof(trp_msg_hdr_t);
        large_mbox_hdr.seq = (uint8_t)trp_msg->hdr.source_seq_num.number;
        large_mbox_hdr.cmd = LARGE_FIFO_MAILBOX_MSG_TRP;
    }

    size_t transfer_size = 0;
    switch (trp_endpoint->transport_type)
    {
    case TRP_TRANSPORT_TYPE_ICC_ARM_MHU: {
        outgoing_full_msg.arm_mhu.header = icc_mhu_hdr;
        if (icc_mhu_hdr.msg_header.command == ICC_COMMAND_DCP_MSG)
        {
            outgoing_full_msg.arm_mhu.dcp_msg = trp_msg->payload.dcp_msg;
        }
        else
        {
            outgoing_full_msg.arm_mhu.trp_msg = *trp_msg;
        }
        transfer_size = icc_mhu_hdr.msg_header.payload_size + sizeof(icc_mhu_header_t);
        if (transfer_size > ICC_MHU_DDR_PAYLOAD_SIZE)
        {
            FPFW_ET_LOG(MtsTransportSizeExceedsMax, transfer_size, ICC_MHU_DDR_PAYLOAD_SIZE);
            return;
        }
        break;
    }
    case TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX: {
        outgoing_full_msg.large_mbox.header = large_mbox_hdr;

        if (large_mbox_hdr.cmd == LARGE_FIFO_MAILBOX_MSG_DCP)
        {
            outgoing_full_msg.large_mbox.dcp_msg = trp_msg->payload.dcp_msg;
        }
        else
        {
            outgoing_full_msg.large_mbox.trp_msg = *trp_msg;
        }
        // the mbox header does not contain the payload size, however the mhu header payload size is set correctly
        // so only the mbox header size needs to be added to the payload size
        // to get the total size of the message to be sent
        transfer_size = icc_mhu_hdr.msg_header.payload_size + sizeof(large_fifo_mailbox_msg_header);
        if (transfer_size > sizeof(large_fifo_mailbox_msg))
        {
            FPFW_ET_LOG(MtsTransportSizeExceedsMax, transfer_size, sizeof(large_fifo_mailbox_msg));
            return;
        }
        // Setting transfer size to sizeof(large_fifo_mailbox_msg) as any other value will return FPFW_ICC_BASE_STATUS_INVALID_SIZE_ARG_ERR
        transfer_size = sizeof(large_fifo_mailbox_msg);
        break;
    }
    default: {
        FPFW_ET_LOG(MtsInvalidTransportType, trp_endpoint->transport_type);
        return;
        break;
    }
    }

    /*
     * ICC send with retry logic for transient timeout errors (when AP is busy).
     * Initialize status to timeout to enter the loop on first attempt.
     * Only timeout errors (FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR) trigger retries;
     * other errors exit immediately as they are not recoverable via retry.
     */
    fpfw_status_t status = FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR;
    uint32_t attempts_remaining = MTS_ICC_SEND_MAX_RETRIES;

    while ((status == FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR) && (attempts_remaining-- > 0))
    {
        /* TODO(ADO3329299): Move to fpfw_icc_base_send */
        status = fpfw_icc_base_send_sync(trp_icc_endpoint->icc_base_ctx, &outgoing_full_msg, transfer_size);

        if (FPFW_STATUS_FAILED(status))
        {
            /* Log every failed attempt for diagnostics */
            FPFW_ET_LOG(TrpIccSendFail, status, (uint32_t)trp_icc_endpoint);

            /* Delay before retry to allow the receiver time to recover */
            if ((status == FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR) && (attempts_remaining > 0))
            {
                tx_thread_sleep((ULONG)MTS_MS_TO_TX_TICKS(MTS_ICC_SEND_RETRY_DELAY_MS));
            }
        }
    }
}
