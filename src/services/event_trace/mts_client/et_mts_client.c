//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file et_mts_client.c
 * This file contains the implementation for the Event Trace MTS client
 * which sends Event Trace data to MCP on getting pull request.
 *
 */

/*-------------------------------- Includes ---------------------------------*/
#include <bug_check.h>                // for BUG_CHECK
#include <et_mts_client.h>            // for event_trace_mts_client_notify_new_msg_cb
#include <et_mts_client_events.h>     // for ET_LOG_ET_SVC
#include <etc_etd_svc.h>              // for get_etc_buffer_size, get_etc_buffer_address
#include <inttypes.h>                 // for PRIx32
#include <message_transfer_service.h> // for mts_client_t ...
#include <mts_client.h>               // for MTS_CLIENT_ID_EVENT_TRACE, mts_client_register
#include <sdm_ext_cfg_regs.h>         // for SDM_EMCPU_DTCM_BASE_ADDRESS
#include <stdint.h>                   // for uint32_t,uint8_t,...
#include <stdio.h>                    // for snprintf
#include <transfer_relay_protocol.h>  // for trp_msg_t, trp_msg_t::(anon...
#include <tx_api.h>                   // for TX_SUCCESS, UINT, TX_TIMER_...

/*------------------- Symbolic Constant Macros (defines) --------------------*/
// Number of messages in the queue
#define ET_MAX_MTS_CLIENT_MESSAGES (1)

// size of trp_msg_t + header
#define ET_MTS_CLIENT_BLOCK_POOL_SIZE \
    ((sizeof(uint32_t) + MAX_TRP_MSG_BLOCK_SIZE) * ET_MAX_MTS_CLIENT_MESSAGES)

// Message size for the queue interms of uint32_t
#define ET_MTS_QUEUE_MSG_SIZE (1)

// Block ID for the Event Trace block - TODO: ADO2686498 What should this value be? Is it arbitrary?
#define ET_BLOCK_ID (51)

// Block version for the Event Trace block TODO: ADO2686498 What should this value be? Is it arbitrary?
#define ET_BLOCK_VER (0x0001)

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/
/* MTS Client Definition for Event Trace */
static mts_client_t s_event_trace_mts_client = {
    .notify_from_drv_frmwk = event_trace_mts_client_notify_new_msg_cb,
};

/* Message Queue Memory for the ET MTS Client */
p_trp_msg_t g_et_mts_client_queue_mem[ET_MAX_MTS_CLIENT_MESSAGES];
/* Memory for the ET MTS Client Block Pool (used by the thread) */
uint8_t g_et_mts_client_pool_mem[ET_MTS_CLIENT_BLOCK_POOL_SIZE];

/* Buffer to store messages for the Event Trace MTS client */
static char s_et_svc_message[LOG_DEFAULT_ASCII_STR_SIZE];

/*----------------------------- Static Functions ----------------------------*/

void event_trace_mts_client_notify_new_msg_cb(void)
{
    p_trp_msg_t p_trp_msg = NULL;

    UINT queue_status = 0;

    ET_LOG_ET_SVC(MTSClientInfo, "Rx ET MTS Msg");

    queue_status = tx_queue_receive(&s_event_trace_mts_client.rx_queue, &p_trp_msg, TX_NO_WAIT);
    if (queue_status != TX_SUCCESS)
    {
        // Soft failure. Report and move on
        ET_LOG_ET_SVC(MTSClientErr, "Rx Msg from Queue Fail %d", queue_status);
        return;
    }

    if (p_trp_msg == NULL)
    {
        // Soft failure. Report and move on
        ET_LOG_ET_SVC(MTSClientErr, "Rx NULL msg");
        return;
    }

    switch (p_trp_msg->hdr.trp_msg_id)
    {
    case TRP_MSG_ID_READ_INTERCORE_BLOCK: {
        p_trp_msg->hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE;
        p_trp_msg->hdr.payload_size = sizeof(trp_msg_read_block_rsp_t);

        p_trp_msg->payload.read_intercore_block_rsp.block_id = ET_BLOCK_ID;
        p_trp_msg->payload.read_intercore_block_rsp.block_version = ET_BLOCK_VER;
        p_trp_msg->payload.read_intercore_block_rsp.source_die_id = mts_get_this_die_id();
        p_trp_msg->payload.read_intercore_block_rsp.source_core_id = mts_get_this_core_id();
        p_trp_msg->payload.read_intercore_block_rsp.addr_offset =
            (uint32_t)((uintptr_t)get_etc_buffer_address() - SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS); // SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS
        p_trp_msg->payload.read_intercore_block_rsp.block_size = get_etc_buffer_size();
        p_trp_msg->payload.read_intercore_block_rsp.crc = fpfw_crc32(
            0,
            (void*)(SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS + p_trp_msg->payload.read_intercore_block_rsp.addr_offset),
            p_trp_msg->payload.read_intercore_block_rsp.block_size);

        ET_LOG_ET_SVC(MTSClientInfo, "Addr Offset: %" PRIx32, p_trp_msg->payload.read_intercore_block_rsp.addr_offset);
        ET_LOG_ET_SVC(MTSClientInfo, "Block Size: %" PRIu32, p_trp_msg->payload.read_intercore_block_rsp.block_size);
        ET_LOG_ET_SVC(MTSClientInfo, "CRC: 0x%" PRIx32, p_trp_msg->payload.read_intercore_block_rsp.crc);

        mts_client_send_trp_response(p_trp_msg);
        break;
    }

    case TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE: {
        // TODO ADO:2617976 Add error handling when TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE is sent without a
        // read_intercore_block the read_intercore_block_rsp fields are echoed back in
        // read_intercore_block_complete validate the fields
        if (p_trp_msg->payload.read_intercore_block.block_id != ET_BLOCK_ID)
        {
            p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_PARAM;
            mts_client_send_trp_response(p_trp_msg);
        }
        else
        {
            // Soft failure. Report and move on
            ET_LOG_ET_SVC(MTSClientErr, "Block ID Invalid: %d", p_trp_msg->payload.read_intercore_block.block_id);
        }
        break;
    }

    default:
        // Soft failure. Report and move on
        ET_LOG_ET_SVC(MTSClientErr, "Message ID Unknown: %d", p_trp_msg->hdr.trp_msg_id);
        break;
    }

    UINT block_status = tx_block_release(p_trp_msg);
    if (block_status != TX_SUCCESS)
    {
        // Soft failure. Report and move on
        ET_LOG_ET_SVC(MTSClientErr, "Block Release Fail: %d", block_status);
    }
}

/*----------------------------- Global Functions ----------------------------*/

void event_trace_mts_client_init(void)
{
    // Create a queue for receiving Event Trace MTS client messages
    UINT tx_status = tx_queue_create(&s_event_trace_mts_client.rx_queue, // queue_ptr
                                     "Event Trace MTS client queue",     // name_ptr
                                     ET_MTS_QUEUE_MSG_SIZE,              // number of uint32_t elements
                                     g_et_mts_client_queue_mem,          // queue_start
                                     sizeof(g_et_mts_client_queue_mem)); // queue_size

    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_event_trace_mts_client.rx_queue);

    tx_status = tx_block_pool_create(&s_event_trace_mts_client.rx_pool, // pool_ptr
                                     "Event Trace MTS client pool",     // name_ptr
                                     MAX_TRP_MSG_BLOCK_SIZE,            // block_size
                                     g_et_mts_client_pool_mem,          // pool_start
                                     sizeof(g_et_mts_client_pool_mem)); // pool_size
    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_event_trace_mts_client.rx_pool);

    mts_client_register(MTS_CLIENT_ID_EVENT_TRACE, &s_event_trace_mts_client);
}
