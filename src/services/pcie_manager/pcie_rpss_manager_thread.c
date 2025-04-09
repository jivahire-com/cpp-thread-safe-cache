//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe subsystem service threads.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkClient.h>   // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FpFwAssert.h>   // for FPFW_RUNTIME_ASSERT
#include <idsw_kng.h>
#include <pcie_config_variable.h>
#include <pcie_dfwk.h>             // for pcie_async_request_t, pcie_dfwk_interf...
#include <pcie_manager_i.h>        // for rpss_req_completion_cb, rpss_service_t...
#include <pcie_phy_load_events.h>  // PhyFW load event
#include <pcie_rp_event_handler.h> // Process Events/misc RP helper functions
#include <pcie_ss_common.h>        // pciess_entity
#include <pciess.h>
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <silibs_kng_soc.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <tx_api.h>  // for TX_WAIT_FOREVER, ULONG, tx_queue_receive

/*-- Symbolic Constant Macros (defines) --*/
/*
 * Service initialization sleep durations to yield worker threads while
 * they are waiting for phy sram load and reset events to complete
 * within an rpss.
 */
#define PRE_SI_WORKER_YIELD_TICKS  (2)
#define POST_SI_WORKER_YIELD_TICKS (200)

#define RP_LINK_TRAINING_TIMEOUT_TICKS (10)
#define RP_TIMER_ONE_SHOT              (0)
#define SET_RPSS_AND_RP_INDEX_FOR_TIMER_CALLBACK(rp_index, rpss_index) \
    (((rp_index & 0xFF) << 8) | (rpss_index & 0xFF))
#define GET_RPSS_INDEX_FROM_TIMER_CALLBACK(cb_val) (cb_val & 0xFF)
#define GET_RP_INDEX_FROM_TIMER_CALLBACK(cb_val)   ((cb_val >> 8) & 0xFF)
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void pcie_sched_async_op(PDFWK_INTERFACE_HEADER iface,
                                PDFWK_ASYNC_REQUEST_HEADER req,
                                DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE callback,
                                void* context);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void pcie_sched_async_op(PDFWK_INTERFACE_HEADER iface,
                                PDFWK_ASYNC_REQUEST_HEADER req,
                                DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE callback,
                                void* context)
{
    FPFW_RUNTIME_ASSERT(req->AllocatedSize >= sizeof(pcie_async_request_t));
    DfwkAsyncRequestSetCompletionRoutine(req, callback, context);
    DfwkInterfaceSendAsync(iface, req);
}

static void send_async_wait_for_event_for_pciess(pcie_manager_context_t* ctx)
{
    for (uint8_t rp_idx = 0; rp_idx < PCIESS_NUM_PORTS; rp_idx++)
    {
        send_async_wait_for_event(ctx, rp_idx, MAX_PENDING_WAIT_FOR_EVENT_PER_RP);
    }
}

static void pcie_rp_timer_expiry_callback(unsigned long cb_val)
{
    /*
     * The callback context is a combination of rpss index (first byte)
     * and root port index (second byte)
     */
    uint8_t rpss_idx = GET_RPSS_INDEX_FROM_TIMER_CALLBACK(cb_val);
    uint8_t rp_idx = GET_RP_INDEX_FROM_TIMER_CALLBACK(cb_val);

    FPFW_DBGPRINT_WARNING("RPSS[%d] RP[%d]: Link training timer expired!\n", rpss_idx, rp_idx);
    pcie_rp_check_link(rpss_idx, rp_idx);
}

static void start_link_training_timer(pcie_manager_context_t* ctx)
{
    unsigned int status;
    pcie_ss_entity_t* rpss = send_sync_get_rpss_entity(ctx->rpss_idx);

    for (uint8_t rp_idx = 0; rp_idx < PCIESS_NUM_PORTS; rp_idx++)
    {
        // Start the LT Timer only for enabled RP.
        if (rpss->rps[rp_idx].enabled)
        {
            unsigned long timer_ctx = SET_RPSS_AND_RP_INDEX_FOR_TIMER_CALLBACK(rp_idx, ctx->rpss_idx);
            tx_timer_delete(&(ctx->expiry_timer[rp_idx]));
            status = tx_timer_create(&(ctx->expiry_timer[rp_idx]),
                                     "pcie_link_training_timer",
                                     pcie_rp_timer_expiry_callback,
                                     timer_ctx,
                                     RP_LINK_TRAINING_TIMEOUT_TICKS,
                                     RP_TIMER_ONE_SHOT,
                                     TX_AUTO_ACTIVATE);
            if (status != TX_SUCCESS)
            {
                FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Failed to create link training timer! TX_STATUS: %d\n",
                                    ctx->rpss_idx,
                                    rp_idx,
                                    status);
                FPFwErrorRaise(status, 0, 0, 0, 0);
            }
        }
    }
}

static void send_full_pciess_init(pcie_manager_context_t* ctx)
{
    /* Send synchronous requests for pre-link training RPSS setup */
    pcie_sync_request_t sync_req = {
        .header = {.RequestType = INITIAL_CONFIG_REQUEST},
        .rpss_index = ctx->rpss_idx,
        .rp_index = 0x0,
        .req_type = INITIAL_CONFIG_REQUEST,
    };
    unsigned long worker_yield_ticks =
        (idsw_get_platform_sdv() >= PLATFORM_RVP_EVT_SILICON) ? POST_SI_WORKER_YIELD_TICKS : PRE_SI_WORKER_YIELD_TICKS;

    int32_t status = DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(ctx->iface), &sync_req.header);
    FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);

    /* Set bit in event flags to signal that these configs are populated */
    tx_event_flags_set(ctx->event_ptr, (1 << ctx->rpss_idx), TX_OR);

    tx_thread_sleep(worker_yield_ticks);

    sync_req.header.RequestType = PRE_RP_INIT_REQUEST;
    sync_req.req_type = PRE_RP_INIT_REQUEST;
    sync_req.rp_index = 0x0;
    status = DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(ctx->iface), &sync_req.header);
    FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
    tx_thread_sleep(worker_yield_ticks);

    sync_req.header.RequestType = POST_RP_INIT_REQUEST;
    sync_req.req_type = POST_RP_INIT_REQUEST;
    sync_req.rp_index = 0x0;
    status = DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(ctx->iface), &sync_req.header);
    FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
}

void send_sync_start_link_training_requests(pcie_manager_context_t* ctx)
{
    pcie_ss_entity_t* rpss = send_sync_get_rpss_entity(ctx->rpss_idx);
    int32_t status;
    /* Send synchronous requests for pre-link training RPSS setup */
    pcie_sync_request_t sync_req = {
        .header = {.RequestType = INITIATE_LINK_TRAINING},
        .rpss_index = ctx->rpss_idx,
        .req_type = INITIATE_LINK_TRAINING,
    };

    /* The number of root ports to be enabled will change based on PCIe configuration */
    for (uint8_t rp_idx = 0; rp_idx < PCIESS_NUM_PORTS; rp_idx++)
    {
        if (rpss->rps[rp_idx].enabled)
        {
            sync_req.rp_index = rp_idx;
            status = DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(ctx->iface), &sync_req.header);
            FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("RPSS[%d] RP[%d]: Skip link training!\n", ctx->rpss_idx, rp_idx);
        }
    }
}

void send_async_wait_for_event(pcie_manager_context_t* ctx, uint8_t rp_idx, uint8_t num_event)
{
    pcie_ss_entity_t* rpss = send_sync_get_rpss_entity(ctx->rpss_idx);

    FPFW_RUNTIME_ASSERT(num_event <= MAX_PENDING_WAIT_FOR_EVENT_PER_RP);
    if (rpss->rps[rp_idx].enabled)
    {
        for (uint8_t req_per_rp = 0; req_per_rp < num_event; req_per_rp++)
        {
            pcie_async_request_t* async_req =
                &(ctx->async_req[rp_idx * MAX_PENDING_WAIT_FOR_EVENT_PER_RP + req_per_rp]);
            DfwkAsyncRequestInitialize(&(async_req->header), sizeof(pcie_async_request_t));
            async_req->rpss_index = ctx->rpss_idx;
            async_req->rp_index = rp_idx;
            async_req->rp_op = WAIT_FOR_EVENT;
            pcie_sched_async_op(&(ctx->iface->header), &(async_req->header), rpss_req_completion_cb, ctx);
        }
    }
}

pcie_ss_entity_t* send_sync_get_rpss_entity(uint8_t rpss_idx)
{
    pcie_manager_context_t* ctx = scp_pcie_get_manager_context(rpss_idx);
    pcie_ss_entity_t* rpss_ent = NULL;
    pcie_sync_request_t sync_req = {0};

    sync_req.p_requested_data = (void*)(&rpss_ent);
    sync_req.rpss_index = rpss_idx;
    sync_req.req_type = GET_RPSS_ENTITY_REQUEST;
    sync_req.header.RequestType = GET_RPSS_ENTITY_REQUEST;
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(ctx->iface), &sync_req.header);

    if (sync_req.status != SILIBS_SUCCESS || rpss_ent == NULL)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Error getting RPSS entity!\n", rpss_idx);
    }

    return rpss_ent;
}

void rpss_service_thread_fn(ULONG thread_input)
{
    /* Process the thread context block and obtain the device and interface handles */
    pcie_manager_context_t* ctx = (pcie_manager_context_t*)(thread_input);
    pciess_device_t* d = ctx->dev;
    pciess_device_interface_t* iface = ctx->iface;

    /* Initialize and open the interface for this root port subsystem */
    pcie_dfwk_interface_init(d, iface);
    DfwkClientInterfaceOpen(&(iface->header));

    /* Wait for the PCIe phy firmware load event - set within the AP core firmware load module */
    FPFW_DBGPRINT_INFO("RPSS[%d]: Waiting for phy firmware load\n", ctx->rpss_idx);
    pcie_phyfw_wait_load_event(ctx->phyfw_load_event_ptr);

    /* Start Initial PCIESS/RP Init */
    send_full_pciess_init(ctx);

    /* Send MAX_PENDING_WAIT_FOR_EVENT_PER_RP ASYNC Requests */
    send_async_wait_for_event_for_pciess(ctx);

    /*
     * Start LT Timer. On SVP, the link_up interrupt fires very quickly, and
     * the timer starts only after the interrupt is processed.
     * So, we start the timer just *before* the driver call to set LTSSM_EN
     */
    start_link_training_timer(ctx);

    /* Send Start LT */
    send_sync_start_link_training_requests(ctx);

    while (true)
    {
        /* Check if there is a completion request waiting for us */
        pciess_completion_request_t cmpl_req = {0};
        (void)tx_queue_receive(&(ctx->work_queue), (void*)&cmpl_req, TX_WAIT_FOREVER);

        /* Handle completion based on request type */
        FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d]: WAIT_FOR_EVENT completed | Op: %d | Status: %d |\n",
                           ctx->rpss_idx,
                           cmpl_req.rp_index,
                           cmpl_req.op,
                           (int)cmpl_req.async_data.data);

        process_wait_for_event_data(ctx->rpss_idx, &cmpl_req);
    }
}
