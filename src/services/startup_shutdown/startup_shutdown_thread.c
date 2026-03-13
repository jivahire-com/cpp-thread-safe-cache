//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_thread.c
 * Implements the startup or shutdown thread for SSI notifications
 */

/*------------- Includes -----------------*/
#include "startup_shutdown.h"
#include "startup_shutdown_i.h"
#include "startup_shutdown_init.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <debug.h>
#include <et_mts_client.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <startup_shutdown_events_i.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SOS_THREAD_PRIORITY          (10)
#define SOS_THREAD_PREEMPT_THRESHOLD (10)

#define SOS_STACK_SIZE    ((TX_MINIMUM_STACK) + ((1) * (FPFW_KB)))
#define SOS_QUEUE_ENTRIES (10)

#define SOS_QUEUE_NAME  "SoS Work Queue"
#define SOS_THREAD_NAME "SoS Worker"
#define SOS_EVENT_NAME  "SSI flags"

#define PHASE_INDEX_NOT_FOUND (-1)
#define MS_TO_TX_TICKS(ms)    (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)

// 200 ms delay for EVT flush to proceed
#define EVT_TX_DELAY 200

/*------------- Typedefs -----------------*/
typedef struct
{
    uint8_t sos_stack[SOS_STACK_SIZE];
    sos_queue_entry_t sos_queue[SOS_QUEUE_ENTRIES];
    TX_THREAD work_thread;
    TX_QUEUE work_queue;
    TX_TIMER timeout_timer;
    TX_EVENT_FLAGS_GROUP ssi_flags;
    uint32_t expected_complete_flags;
} sos_thread_context_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static sos_thread_context_t s_sos_thread_ctx = {0};
/*------------- Functions ----------------*/

// find the core-specific boot stage index for the given phase
int sos_queue_find_phase(ssi_startup_stage_t phase)
{
    for (unsigned int i = 0; i < sos_core_boot_stage_count(); i++)
    {
        if (sos_core_boot_stages()[i].phase == phase)
        {
            return i;
        }
    }
    return PHASE_INDEX_NOT_FOUND;
}

static inline void sos_signal_flags_and_assert(uint32_t flag_bits)
{
    int status = tx_event_flags_set(&s_sos_thread_ctx.ssi_flags, flag_bits, TX_OR);
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);
}

void sos_accel_quiesce_complete(uint32_t accel_id, uint32_t event_flag)
{
    SOS_LOG_INFO("Accel:%d Quiesce Complete, event flag: %x\n", (int)accel_id, (int)event_flag);
    sos_signal_flags_and_assert(event_flag);
}

void sos_quiesce_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    pstartup_ssi_registration_t ssi_registration = (pstartup_ssi_registration_t)p_completion_context;
    SOS_LOG_INFO("Quiesce Request to an SSI ID : %08lx completed (%x)",
                 (unsigned long)ssi_registration->registration_id,
                 (uintptr_t)request);
    sos_signal_flags_and_assert(ssi_registration->interface_unique_flag);
}

void sos_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    pstartup_ssi_registration_t ssi_registration = (pstartup_ssi_registration_t)p_completion_context;
    SOS_LOG_TRACE("Request to an SSI ID : %08lx completed (%x)\n",
                  (unsigned long)ssi_registration->registration_id,
                  (uintptr_t)request);
    sos_signal_flags_and_assert(ssi_registration->interface_unique_flag);
}

void sos_notify_ssi_boot_stage(psos_service_context_t p_context, ssi_startup_stage_t stage, ssi_startup_type_t startup_type, bool start)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    // iterate over ssi registrations
    PFPFW_LIST_ENTRY iterator;
    PFPFW_LIST_ENTRY iterator_next;

    FpFwListForEach(p_context->ssi_registrations, iterator, iterator_next)
    {
        pstartup_ssi_registration_t p_registration = CONTAINING_RECORD(iterator, startup_ssi_registration_t, list_entry);

        // start flag indicates this is entry into the phase
        // call the appropriate SSI function, passing in the request object registered at boot time
        if (start)
        {
            ssi_startup_stage_start(p_registration->p_ssi_interface,
                                    &p_registration->ssi_request,
                                    stage,
                                    startup_type,
                                    sos_completion,
                                    (void*)p_registration);
        }
        else
        {
            ssi_startup_stage_complete(p_registration->p_ssi_interface,
                                       &p_registration->ssi_request,
                                       stage,
                                       startup_type,
                                       sos_completion,
                                       (void*)p_registration);
        }
    }
}

void sos_notify_ssi_shutdown(psos_service_context_t p_context, ssi_shutdown_type_t shutdown_type)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    // iterate over ssi registrations
    PFPFW_LIST_ENTRY iterator;
    PFPFW_LIST_ENTRY iterator_next;

    FpFwListForEach(p_context->ssi_registrations, iterator, iterator_next)
    {
        pstartup_ssi_registration_t p_registration = CONTAINING_RECORD(iterator, startup_ssi_registration_t, list_entry);

        // call the appropriate SSI function, passing in the request object registered at boot time
        ssi_shutdown(p_registration->p_ssi_interface, &p_registration->ssi_request, shutdown_type, sos_completion, (void*)p_registration);
    }
}

void sos_notify_ssi_quiesce(psos_service_context_t p_context, ssi_shutdown_type_t shutdown_type)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    // iterate over ssi registrations
    PFPFW_LIST_ENTRY iterator;
    PFPFW_LIST_ENTRY iterator_next;

    FpFwListForEach(p_context->ssi_registrations, iterator, iterator_next)
    {
        pstartup_ssi_registration_t p_registration = CONTAINING_RECORD(iterator, startup_ssi_registration_t, list_entry);

        // call the appropriate SSI function, passing in the request object registered at boot time
        ssi_quiesce(p_registration->p_ssi_interface,
                    &p_registration->ssi_request,
                    shutdown_type,
                    sos_quiesce_completion,
                    (void*)p_registration);
    }
}

void wait_ssi_complete(sos_stage_timeout_t* current_stage)
{
    ULONG flags = 0;

    // check crashdump in progress
    bool crash_dump_in_progress = crash_dump_get_is_dump_transferring(NULL);

    if (current_stage->stage_category == BOOT_STAGE)
    {
        current_stage->timeout_ms = sos_boot_timeout(current_stage);
    }
    else if (current_stage->stage_category == SHUTDOWN_STAGE)
    {
        current_stage->timeout_ms =
            crash_dump_in_progress ? FULL_CD_TRANSFER_TIMEOUT_MS : sos_shutdown_timeout(current_stage);
        SOS_LOG_INFO("Shutdown ssi stage timeout: %lu ms\n", current_stage->timeout_ms);
    }
    else
    {
        current_stage->timeout_ms = DEFAULT_SOS_TIMEOUT_MS;
    }

    SOS_LOG_TRACE("SSI completion - Expected flags: %lx\n", (unsigned long)s_sos_thread_ctx.expected_complete_flags);
    int status = tx_event_flags_get(&s_sos_thread_ctx.ssi_flags,
                                    s_sos_thread_ctx.expected_complete_flags,
                                    TX_AND_CLEAR,
                                    &flags,
                                    MS_TO_TX_TICKS(current_stage->timeout_ms));

    if (status == TX_SUCCESS)
    {
        SOS_LOG_TRACE("SSI completion - All flags received: 0x%08lx", (unsigned long)flags);
        return;
    }

    if (status == TX_NO_EVENTS)
    {
        ULONG current_set_flags = 0;
        tx_event_flags_info_get(&s_sos_thread_ctx.ssi_flags, TX_NULL, &current_set_flags, TX_NULL, TX_NULL, TX_NULL);

        uint32_t missing_flags = s_sos_thread_ctx.expected_complete_flags & ~current_set_flags;

        const uint32_t crashdump_event_flag = SOS_SSI_ID_TO_EVENT_FLAG(SOS_SSI_ID_CRASHDUMP);
        if (crash_dump_in_progress && (missing_flags == crashdump_event_flag))
        {
            SOS_LOG_CRIT("SSI completion timeout occurred while CD transfer");
        }
        else
        {
            SOS_LOG_CRIT("SSI completion - Timeout occurred waiting for flags: %lx, missing flags: %lx\n",
                         (unsigned long)s_sos_thread_ctx.expected_complete_flags,
                         (unsigned long)missing_flags);

            BUG_ASSERT_PARAM(
                false,
                missing_flags,
                (current_stage->stage_category == BOOT_STAGE)
                    ? ((current_stage->stage_category << 16) | (current_stage->operation_stage.boot & 0xFFFF))
                    : ((current_stage->stage_category << 16) | (current_stage->operation_stage.shutdown & 0xFFFF)));
        }
    }
    else
    {
        SOS_LOG_CRIT("SSI completion - Error occurred waiting for flags: %d\n", status);
        BUG_ASSERT_PARAM(
            false,
            status,
            (current_stage->stage_category == BOOT_STAGE)
                ? ((current_stage->stage_category << 16) | (current_stage->operation_stage.boot & 0xFFFF))
                : ((current_stage->stage_category << 16) | (current_stage->operation_stage.shutdown & 0xFFFF)));
    }
}

void sos_notify_ssi_boot_stage_and_wait(psos_service_context_t p_context,
                                        ssi_startup_stage_t stage,
                                        ssi_startup_type_t startup_type,
                                        bool start)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    sos_notify_ssi_boot_stage(p_context, stage, startup_type, start);

    sos_stage_timeout_t current_stage = {.stage_category = BOOT_STAGE, .operation_stage.boot = stage};
    wait_ssi_complete(&current_stage);
}

void sos_worker_thread_function(ULONG service_ctx)
{
    sos_service_context_t* p_sos_ctx = (sos_service_context_t*)service_ctx;
    sos_queue_entry_t message;

    sos_stage_timeout_t current_stage;

    SOS_LOG_INFO("Worker thread begin\n");

    while (1)
    {
        UINT status = tx_queue_receive(&s_sos_thread_ctx.work_queue, &message, TX_WAIT_FOREVER);
        FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);

        // store the flags expected to be set when all SSI notifications are complete
        s_sos_thread_ctx.expected_complete_flags = p_sos_ctx->registered_ssi_mask;

        switch (message.type)
        {
        case SOS_QUEUE_ENTRY_TYPE_BOOT_PHASE:
            SOS_LOG_INFO("Worker handling boot phase %d\n", message.data.boot_phase.phase);
            int phase = sos_queue_find_phase(message.data.boot_phase.phase);
            if (phase != PHASE_INDEX_NOT_FOUND)
            {
                SOS_LOG_TRACE("Phase found at index %d\n", phase);
                // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1821526/
                //       start timer for stage timeout - expect timeout could merely trigger crash dump as it's a boot error
                //       sos_notify below should save some state that can be used in timeout handler to signal what failed
                // for each stage, call handlers

                // notify of phase entry
                sos_notify_ssi_boot_stage_and_wait(p_sos_ctx,
                                                   message.data.boot_phase.phase,
                                                   message.data.boot_phase.boot_type,
                                                   true);
                for (unsigned int stage_idx = phase;
                     stage_idx < sos_core_boot_stage_count() &&
                     sos_core_boot_stages()[stage_idx].phase == message.data.boot_phase.phase;
                     stage_idx++)
                {
                    // remote core sync if needed
                    if (!wait_for_remote_die_boot_stage(sos_core_boot_stages()[stage_idx]))
                    {
                        SOS_LOG_WARN("Timeout occurred on remote stage sync (%d)\n",
                                     sos_core_boot_stages()[stage_idx].stage);
                    }

                    // Local core sync if needed
                    if (!wait_for_local_core_boot_stage(sos_core_boot_stages()[stage_idx]))
                    {
                        SOS_LOG_WARN("Local Sync Failed for stage (%d)\n", sos_core_boot_stages()[stage_idx].stage);
                    }

                    // notify stage entry
                    sos_notify_ssi_boot_stage_and_wait(p_sos_ctx,
                                                       sos_core_boot_stages()[stage_idx].stage,
                                                       message.data.boot_phase.boot_type,
                                                       true);
                    // notify stage complete
                    sos_notify_ssi_boot_stage_and_wait(p_sos_ctx,
                                                       sos_core_boot_stages()[stage_idx].stage,
                                                       message.data.boot_phase.boot_type,
                                                       false);
                }
                // notify of phase complete
                sos_notify_ssi_boot_stage_and_wait(p_sos_ctx,
                                                   message.data.boot_phase.phase,
                                                   message.data.boot_phase.boot_type,
                                                   false);
            }
            else
            {
                SOS_LOG_WARN("SOS unknown phase %d\n", message.data.boot_phase.phase);
            }

            SOS_ET_BOOT_PROFILE_INFO(BOOT_STAGE, message.data.boot_phase.boot_type, SOS_ET_TYPE_BOOT_OPERATION_COMPLETE);
            break;

        case SOS_QUEUE_ENTRY_TYPE_SHUTDOWN:
            SOS_LOG_INFO("SOS message: shutdown, (%d)\n", message.data.shutdown_type);
            pstartup_shutdown_request_t shutdown_req = (pstartup_shutdown_request_t)message.p_request;
            shutdown_req->result = KNG_SUCCESS;

            // Send D2D shutdown request to initiate shutdown sequence on remote die
            if (message.data.shutdown_type != REMOTE_SCP_SHUTDOWN && message.data.shutdown_type != MSCP_SUBSYS_RESET)
            {
                SOS_LOG_INFO("Sending D2D shutdown request for type %d\n", message.data.shutdown_type);
                sos_send_d2d_shutdown_request();
            }

            // notify all registered interfaces
            sos_notify_ssi_shutdown(p_sos_ctx, message.data.shutdown_type);

            // wait for responses
            current_stage.stage_category = SHUTDOWN_STAGE;
            current_stage.operation_stage.shutdown = message.data.shutdown_type;

            wait_ssi_complete(&current_stage);

            if (message.data.shutdown_type != AP_WARM_RESET)
            {
                pstartup_shutdown_request_t req = (pstartup_shutdown_request_t)message.p_request;
                req->result = sos_core_shutdown_handler(message.data.shutdown_type);
                SOS_ET_BOOT_PROFILE_INFO(SHUTDOWN_STAGE, message.data.shutdown_type, SOS_ET_TYPE_BOOT_OPERATION_COMPLETE);
            }
            else
            {
                // trigger warm reset operation
                sos_start_phase(fpfw_init_get_handle("sos_int"), NULL, WARM_BOOT_POST_AP, STARTUP_PHASE_AP_ASYNC, NULL, NULL);
            }

            break;

        case SOS_QUEUE_ENTRY_TYPE_QUIESCE:
            SOS_LOG_INFO("SOS message: quiesce, (%d)\n", message.data.shutdown_type);

            // Invoke async Accel quiesce flow first as it's quiesce time may be long
            if (idsw_get_cpu_type() == CPU_SCP)
            {
                SOS_LOG_INFO("Invoking Accel Quiesce Flow\n");
                uint32_t accel_event_mask = execute_accel_quiesce_flow();
                // Update the flags to include accel events
                s_sos_thread_ctx.expected_complete_flags |= accel_event_mask;
            }

            // notify all locally registered interfaces
            sos_notify_ssi_quiesce(p_sos_ctx, message.data.shutdown_type);

            current_stage.stage_category = QUIESCE_STAGE;
            current_stage.timeout_ms = DEFAULT_SOS_TIMEOUT_MS;
            wait_ssi_complete(&current_stage);

            SOS_ET_BOOT_PROFILE_INFO(QUIESCE_STAGE, message.data.shutdown_type, SOS_ET_TYPE_BOOT_OPERATION_COMPLETE);

            break;
        }

        if (message.p_request)
        {
            DfwkAsyncRequestComplete(message.p_request);
        }
    }
}

void sos_queue_start_phase(ssi_startup_type_t boot_type, ssi_startup_stage_t phase, PDFWK_ASYNC_REQUEST_HEADER p_request)
{
    sos_queue_entry_t message;
    message.type = SOS_QUEUE_ENTRY_TYPE_BOOT_PHASE;
    message.data.boot_phase.boot_type = boot_type;
    message.data.boot_phase.phase = phase;
    message.p_request = p_request;

    SOS_LOG_TRACE("Queue start phase %d boot type %d (request %x)\n", phase, boot_type, (uintptr_t)p_request);

    // int status =
    tx_queue_send(&s_sos_thread_ctx.work_queue, &message, TX_NO_WAIT);
    // FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);
}

void sos_queue_shutdown(ssi_shutdown_type_t shutdown_type, PDFWK_ASYNC_REQUEST_HEADER p_request)
{
    sos_queue_entry_t message;
    message.type = SOS_QUEUE_ENTRY_TYPE_SHUTDOWN;
    message.data.shutdown_type = shutdown_type;
    message.p_request = p_request;

    SOS_LOG_INFO("Queue shutdown type %d\n", shutdown_type);

    int status = tx_queue_send(&s_sos_thread_ctx.work_queue, &message, TX_NO_WAIT);
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);
}

void sos_queue_quiesce(ssi_shutdown_type_t shutdown_type, PDFWK_ASYNC_REQUEST_HEADER p_request)
{
    sos_queue_entry_t message;
    message.type = SOS_QUEUE_ENTRY_TYPE_QUIESCE;
    message.data.shutdown_type = shutdown_type;
    message.p_request = p_request;

    SOS_LOG_INFO("Queue quiesce type %d\n", shutdown_type);

    int status = tx_queue_send(&s_sos_thread_ctx.work_queue, &message, TX_NO_WAIT);
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);
}

void sos_thread_init(psos_service_context_t p_context)
{

    FPFW_RUNTIME_ASSERT(p_context != NULL);
    SOS_LOG_INFO("Worker thread init\n");

    // create thread, queue, and event flags for handling requests
    int status = tx_queue_create(&s_sos_thread_ctx.work_queue,                 /* TX_QUEUE *queue_ptr */
                                 SOS_QUEUE_NAME,                               /* CHAR *name_ptr */
                                 sizeof(sos_queue_entry_t) / sizeof(uint32_t), /* UINT message_size in 32b words */
                                 (void*)s_sos_thread_ctx.sos_queue,            /* VOID *queue_start */
                                 sizeof(s_sos_thread_ctx.sos_queue));          /* ULONG queue_size */
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);

    status = tx_event_flags_create(&s_sos_thread_ctx.ssi_flags, SOS_EVENT_NAME);
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);

    status = tx_thread_create(&s_sos_thread_ctx.work_thread,      /* TX_THREAD *thread_ptr */
                              SOS_THREAD_NAME,                    /* CHAR *name_ptr */
                              sos_worker_thread_function,         /* VOID (*entry_function)(ULONG) */
                              (ULONG)p_context,                   /* ULONG entry_input */
                              (void*)s_sos_thread_ctx.sos_stack,  /* VOID *stack_start */
                              sizeof(s_sos_thread_ctx.sos_stack), /* ULONG stack_size */
                              SOS_THREAD_PRIORITY,                /* UINT priority */
                              SOS_THREAD_PREEMPT_THRESHOLD,       /* UINT preempt_threshold */
                              TX_NO_TIME_SLICE,                   /* ULONG time_slice */
                              TX_AUTO_START);                     /* UINT auto_start */
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);
}
