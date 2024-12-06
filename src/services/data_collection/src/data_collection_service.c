//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_collection_service.c
 * This file contains the implementation for the Data Collection Service
 */

/*------------- Includes -----------------*/
#include "data_collection_service.h"

#include "data_collection_protocol.h"
#include "data_collection_service_i.h"
#include "tlm_relay_i.h"

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <fpfw_status.h>
#include <stdio.h>
#include <tx_api.h>
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static dcs_context_t s_dcs_context;
/*------------- Functions ----------------*/
static void dcs_thread(ULONG thread_input);

void dcs_init(p_dcs_config_t config)
{
    UINT tx_status = tx_thread_create(&s_dcs_context.thread,
                                      "dcs_thread",
                                      dcs_thread,
                                      (ULONG)NULL,
                                      config->thread_config.p_stack,
                                      config->thread_config.stack_size,
                                      config->thread_config.priority,
                                      config->thread_config.priority,
                                      config->thread_config.time_slice_option,
                                      TX_AUTO_START);
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, 0, 0, 0);

    tx_status = tx_queue_create(&s_dcs_context.dcp_error_queue.queue,
                                "dcp error queue",
                                1, // number of uint32_t elements
                                s_dcs_context.dcp_error_queue.mem,
                                sizeof(s_dcs_context.dcp_error_queue.mem));
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_dcs_context.dcp_error_queue.queue, 0, 0);

    tx_status = tx_queue_create(&s_dcs_context.trp_forward_queue.queue,
                                "trp forward queue",
                                1, // number of uint32_t elements
                                s_dcs_context.trp_forward_queue.mem,
                                sizeof(s_dcs_context.trp_forward_queue.mem));
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_dcs_context.trp_forward_queue.queue, 0, 0);

    tx_status = tx_block_pool_create(&s_dcs_context.msg_pool.pool,        // pool_ptr
                                     "dcs msg pool",                      // name_ptr
                                     DCS_MSG_BLOCK_SIZE,                  // block_size
                                     s_dcs_context.msg_pool.mem,          // pool_start
                                     sizeof(s_dcs_context.msg_pool.mem)); // pool_size
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_dcs_context.msg_pool.pool, 0, 0);

    tx_status = tx_event_flags_create(&s_dcs_context.thread_ctrl, "DCS Thread Control");
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, 0, 0, 0);

    tlm_relay_init(&config->trp_icc_config);
}

static void dcs_thread(ULONG thread_input)
{
    FPFW_UNUSED(thread_input);
    while (1)
    {
        tx_thread_sleep(1000);
    }
}

void dcs_forward_trp_msg_to_client_from_drv_frmwk(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);

    // printf("source_die_id: %d, source_cpu_id: %d, dest_die_id: %d, dest_cpu_id:%d, dcp_client_id: %d, "
    //        "trp_msg_id: %d\n",
    //        trp_msg->hdr.source_cpu_id,
    //        trp_msg->hdr.source_die_id,
    //        trp_msg->hdr.dest_cpu_id,
    //        trp_msg->hdr.dest_die_id,
    //        trp_msg->hdr.dcp_client_id,
    //        trp_msg->hdr.trp_msg_id);
}