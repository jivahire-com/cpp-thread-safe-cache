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
#include <cmsis_m7.h>                 // for SCB_CleanInvalidateDCache_by_Addr
#include <et_mts_client.h>            // for et_mts_rx_msg_handler
#include <et_mts_client_events.h>     // for ET_LOG_ET_SVC
#include <etc_etd_svc.h>              // for get_etc_buffer_size, get_etc_buffer_byte_pool_address
#include <idsw_kng.h>                 // for CPU_SDM, CPU_CDED_SDM
#include <inttypes.h>                 // for PRIx32, PRIu32
#include <message_transfer_service.h> // for mts_client_t ...
#include <mts_client.h>               // for MTS_CLIENT_ID_EVENT_TRACE, mts_client_register
#include <stdint.h>                   // for uint32_t,uint8_t,...
#include <stdio.h>                    // for snprintf
#include <transfer_relay_protocol.h>  // for trp_msg_t, trp_msg_t::(anon...
#include <tx_api.h>                   // for TX_SUCCESS, UINT, TX_TIMER_...

/*------------------- Symbolic Constant Macros (defines) --------------------*/
/* 1 (request already being processed) + 4 (2*2 requests for the 2 buffers, notification + response) */
#define ET_MTS_CLIENT_MAX_MESSAGES    (5U)
#define ET_MTS_CLIENT_BLOCK_POOL_SIZE (MAX_TRP_MSG_BLOCK_SIZE * ET_MTS_CLIENT_MAX_MESSAGES)

#define TIMEOUT_ET_BUFFER_FLUSH_MS (5000U) // 5 seconds

#define ET_MTS_EVENT_FLAG_NEW_MTS_MSG (0x1)
#define ET_MTS_EVENT_FLAG_ANY_VALID   (ET_MTS_EVENT_FLAG_NEW_MTS_MSG)

#define ET_MTS_STACK_SIZE ((TX_MINIMUM_STACK) + ((4) * (FPFW_KB)))

#define ET_MTS_THREAD_PRIORITY (10U)

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/
static TX_THREAD s_et_mts_thread;

/* Message Queue Memory for the ET MTS Client */
static p_trp_msg_t g_et_mts_client_queue_mem[ET_MTS_CLIENT_MAX_MESSAGES];

/* Memory for the ET MTS Client Block Pool (used by the thread) */
static uint8_t g_et_mts_client_pool_mem[ET_MTS_CLIENT_BLOCK_POOL_SIZE];

/* Buffer to store messages for the Event Trace MTS client */
static char s_et_svc_message[LOG_DEFAULT_ASCII_STR_SIZE];

static uint8_t s_et_mts_stack[ET_MTS_STACK_SIZE];

/* MTS Client Definition for Event Trace */
static mts_client_t s_et_mts_client = {0};

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

void et_mts_worker_thread_func(ULONG thread_input)
{
    FPFW_UNUSED(thread_input);

    /* Infinite loop that will decode and recycle buffers marked as completed */
    while (true)
    {
        p_trp_msg_t p_trp_msg = NULL;

        UINT queue_status = tx_queue_receive(&s_et_mts_client.rx_queue, &p_trp_msg, TX_WAIT_FOREVER);

        // Assert if the queue status is not TX_SUCCESS or TX_QUEUE_EMPTY
        BUG_ASSERT_PARAM((queue_status == TX_SUCCESS), queue_status, 0);

        switch (p_trp_msg->hdr.trp_msg_id)
        {
        case TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE: {
            /* Recycle the buffer. If the ETR returns a buffer that is not in the processing state,
             * FPFwETControllerRecycleBuffer will handle error management
             * An error in this case would indicate that the processing time from the MCP exceeds the time
             * taken to fill the buffer, consider increasing the buffer size in that case.*/
            ET_LOG_ET_SVC(MTSClientInfo, "Read Intercore Block Complete");
            if (p_trp_msg->payload.read_intercore_block_complete.block_id >= ETC_SERVICE_CORE_BUFFER_COUNT)
            {
                /* Invalid Block ID, report error and drop the message */
                ET_LOG_ET_SVC(MTSClientErr,
                              "Fail: Invalid ET Buffer: %1d",
                              p_trp_msg->payload.read_intercore_block_complete.block_id);
            }
            else
            {
                /* Recycle the buffer */
                uint32_t buffer_id = p_trp_msg->payload.read_intercore_block_complete.block_id;
                ET_LOG_ET_SVC(MTSClientInfo, "Recycling Buff %" PRIu32, buffer_id);

                FPFW_ET_STATUS status = FPFwETControllerRecycleBuffer(FPFwETGetController(), buffer_id);
                BUG_ASSERT_PARAM(status == FPFW_ET_SUCCESS, status, buffer_id);
            }
            break;
        }

        default:
            // Soft failure. Report and move on
            ET_LOG_ET_SVC(MTSClientErr, "Message ID Unknown: %d", p_trp_msg->hdr.trp_msg_id);
            break;
        }

        UINT block_status = tx_block_release(p_trp_msg);
        BUG_ASSERT_PARAM(block_status == TX_SUCCESS, block_status, (uintptr_t)p_trp_msg);

/* For unit tests - break out of the loop */
#ifdef _WIN32
        break;
#endif
    }
}

void event_trace_mts_client_init(void)
{
    ET_LOG_ET_SVC(MTSClientInfo, "Initializing ET MTS Client");

    UINT tx_status = tx_block_pool_create(&s_et_mts_client.rx_pool,          // pool_ptr
                                          "ET-MTS client pool",              // name_ptr
                                          MAX_TRP_MSG_BLOCK_SIZE,            // block_size (in bytes)
                                          g_et_mts_client_pool_mem,          // pool_start
                                          sizeof(g_et_mts_client_pool_mem)); // pool_size (in bytes)
    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_et_mts_client.rx_pool);

    // Create a queue for receiving Event Trace MTS client messages
    tx_status = tx_queue_create(&s_et_mts_client.rx_queue,              // queue_ptr
                                "ET-MTS client queue",                  // name_ptr
                                sizeof(p_trp_msg_t) / sizeof(uint32_t), // number of uint32_t elements
                                g_et_mts_client_queue_mem,              // queue_start
                                sizeof(g_et_mts_client_queue_mem));     // queue_size (in bytes)

    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_et_mts_client.rx_queue);

    /* Launch the MTS worker thread */
    tx_status = tx_thread_create(&s_et_mts_thread,
                                 "ET-MTS Worker Thread",
                                 et_mts_worker_thread_func,
                                 0,
                                 &s_et_mts_stack,
                                 sizeof(s_et_mts_stack),
                                 ET_MTS_THREAD_PRIORITY,
                                 ET_MTS_THREAD_PRIORITY,
                                 TX_NO_TIME_SLICE,
                                 TX_AUTO_START);
    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_et_mts_thread);

    mts_client_register(MTS_CLIENT_ID_EVENT_TRACE, &s_et_mts_client);

    ET_LOG_ET_SVC(MTSClientInfo, "ET MTS Client Init Done");
}

fpfw_status_t et_mts_notify_buffer_complete(void* p_buffer, etc_service_completion_request_t* p_request)
{
    FPFW_UNUSED(p_buffer);

    if (p_request == NULL || p_request->core_buffer_request.p_core_buffer == NULL)
    {
        ET_LOG_ET_SVC(MTSClientErr, "Fail: Invalid Buffer");
        return FPFW_STATUS_INVALID_ARGS;
    }

    uint8_t die_id = mts_get_this_die_id();
    uint8_t core_id = mts_get_this_core_id();

    uint32_t block_size = p_request->core_buffer_request.p_core_buffer->BufferSize;
    uint16_t buffer_id = p_request->core_buffer_request.p_core_buffer->BufferId;
    BUG_ASSERT_PARAM(buffer_id < ETC_SERVICE_CORE_BUFFER_COUNT, buffer_id, p_request);

    uint32_t et_buffer_addr = (uint32_t)get_etc_buffer_byte_pool_address() + (get_etc_buffer_size() * buffer_id);
    uint32_t crc = fpfw_crc32(0, (void*)et_buffer_addr, block_size);

    trp_msg_t send_msg = {.hdr =
                              {
                                  .src_node =
                                      {
                                          .die_id = die_id,
                                          .core_id = core_id,
                                      },
                                  .dest_node =
                                      {
                                          .die_id = die_id,
                                          .core_id = CPU_MCP,
                                      },
                                  .trp_msg_status = TRP_STATUS_SUCCESS,
                                  .broadcast_type = TRP_BROADCAST_NONE,
                                  .mts_client_id = MTS_CLIENT_ID_EVENT_TRACE,
                                  .trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION,
                                  .payload_size = sizeof(trp_msg_read_block_rsp_t),
                              },
                          .payload = {.intercore_block_notification = {
                                          .block_id = buffer_id, // Use the block_id to share the buffer_id being uploaded
                                          .block_version = 0, // Always set to 0 for now. Can share number of blocks sent to MCP here
                                          .source_die_id = die_id,
                                          .source_core_id = core_id,
                                          .addr_offset = et_buffer_addr,
                                          .block_size = block_size,
                                          .crc = crc,
                                      }}};

    /* Flush Cache on SCP since data is read on the other side of the (cached) EXP RAM */
    if (core_id == CPU_SCP)
    {
        SCB_CleanDCache_by_Addr((uint32_t*)et_buffer_addr, block_size);
    }

    mts_client_send_new_trp_msg(&send_msg);

    return FPFW_STATUS_SUCCESS;
}
