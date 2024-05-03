//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_controller.c
 *  This modules initializes and configures event tracing for a core. Worker thread core specific.
 */

/*------------- Includes -----------------*/

#include "IFpFwEventTracingStatus.h" // for FPFW_ET_SUCCESS, FPFW_ET_E_...
#include "event_trace_collector_i.h" // for etc_buffer_complete_cb, etc...

#include <ErrorHandler.h>                // for FPFwErrorRaise
#include <IFpFwEventTracing.h>           // for FPFwETTraceInitialize
#include <IFpFwEventTracingBuffers.h>    // for PFPFW_ET_CORE_BUFFER_HEADER
#include <IFpFwEventTracingController.h> // for FPFwETControllerRecycleBuffer
#include <IFpFwEventTracingDecoder.h>    // for FPFwETDecoderPrintEvent
#include <IFpFwEventTracingDefines.h>    // for FPFW_ET_EVENT_METADATA_HEADER
#include <IFpFwEventTracingEvents.h>     // for FPFW_ET_PROVIDER_EVENT_FILTER
#include <event_trace_collector.h>       // for (anonymous struct)::(anonym...
#include <stdbool.h>                     // for true
#include <stddef.h>                      // for NULL
#include <stdint.h>                      // for uint8_t, uint32_t, uint64_t
#include <tx_api.h>                      // for TX_SUCCESS, TX_NO_WAIT, ULONG
#include <tx_initialize.h>               // for TX_INITIALIZE_IN_PROGRESS
#include <tx_thread.h>                   // IWYU pragma: keep

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

void etc_worker_thread_func(ULONG thread_input);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void etc_initialize(etc_service_context_t* p_service, const etc_service_config_t* p_config)
{
    if (NULL == p_service || NULL == p_config || p_config->mode >= ETC_SERVICE_MODE_MAX)
    {
        FPFwErrorRaise(FPFW_ET_E_INVALIDARG);
    }

    // Initialize completion queue
    int status = tx_queue_create(&p_service->work_queue,
                                 ETC_WORK_QUEUE_NAME,
                                 sizeof(p_service->request_pool[0]) / sizeof(uint32_t),
                                 p_service->request_pool,
                                 sizeof(p_service->request_pool));

    if (status != TX_SUCCESS)
    {
        FPFwErrorRaise(status);
    }

    status = tx_semaphore_create(&p_service->event_quota, ETC_EVENT_QUOTA_SEM_NAME, ETC_SERVICE_EVENT_QUOTA);

    if (status != TX_SUCCESS)
    {
        FPFwErrorRaise(status);
    }

    status = tx_thread_create(&p_service->worker_thread,
                              ETC_WORKER_THREAD_NAME,
                              etc_worker_thread_func,
                              (ULONG)p_service,
                              p_config->thread_config.p_stack,
                              p_config->thread_config.stack_size,
                              p_config->thread_config.priority,
                              p_config->thread_config.priority,
                              p_config->thread_config.time_slice_option,
                              TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        FPFwErrorRaise(status);
    }

    status = FPFwETDecoderInit((FPFW_ET_PROVIDER_METADATA_HEADER*)p_config->event_manifest.provider_start,
                               (FPFW_ET_PROVIDER_METADATA_HEADER*)p_config->event_manifest.provider_end,
                               (FPFW_ET_EVENT_METADATA_HEADER*)p_config->event_manifest.event_start,
                               (FPFW_ET_EVENT_METADATA_HEADER*)p_config->event_manifest.event_end);

    if (status != FPFW_ET_SUCCESS)
    {
        FPFwErrorRaise(status);
    }

    FPFW_ET_CONTROLLER_CONFIG controller_config = {
        .CoreId = p_config->core_id,
        .ManifestId = p_config->manifest_id,
        .BufferSize = p_config->trace_buffer_memory.byte_count / ETC_SERVICE_CORE_BUFFER_COUNT,
        .Buffers = {((uint8_t*)p_config->trace_buffer_memory.p_pool),
                    ((uint8_t*)p_config->trace_buffer_memory.p_pool) +
                        p_config->trace_buffer_memory.byte_count / ETC_SERVICE_CORE_BUFFER_COUNT},
        .ProviderFilter = (FPFW_ET_PROVIDER_EVENT_FILTER*)p_config->provider_filters.start,
        .ProviderCount = (p_config->provider_filters.end - p_config->provider_filters.start) / sizeof(FPFW_ET_PROVIDER_EVENT_FILTER),
        .Callbacks = {.BufferCompletionCallback = etc_buffer_complete_cb,
                      .GetTicks = etc_get_ticks_cb,
                      .OnEvent = etc_on_event_cb,
                      .Context = p_service}};

    status = FPFwETTraceInitialize(&p_service->controller, &controller_config);

    if (status != FPFW_ET_SUCCESS)
    {
        FPFwErrorRaise(status);
    }
}

void etc_buffer_complete_cb(void* p_context, PFPFW_ET_CORE_BUFFER_HEADER p_core_buffer_header, PFPFW_ET_CONTROLLER p_controller)
{
    etc_service_context_t* p_service = (etc_service_context_t*)p_context;

    etc_service_completion_request_t completion_request = {.type = ETC_SERVICE_REQUEST_TYPE_PROCESS_CORE_BUFFER,
                                                           .core_buffer_request = {p_core_buffer_header, p_controller}};

    // This will always succeed. As the queue is created before this is called and the queue
    // capacity can contain the maximum number of requests.
    (void)tx_queue_send(&p_service->work_queue, (void*)&completion_request, TX_NO_WAIT);
}

void etc_on_event_cb(void* p_event, void* p_context)
{
    etc_service_context_t* p_service = (etc_service_context_t*)p_context;

    // Events during boot will be written directly
    if (TX_INITIALIZE_IN_PROGRESS == TX_THREAD_GET_SYSTEM_STATE())
    {
        uint32_t size = 0;
        (void)FPFwETDecoderPrintEvent(p_event, &size);
        return;
    }

    // If the quota is met, drop the event. Blocking the caller could result in deadlocks
    // as tracing is used in transport libraries such as ICC
    if (TX_SUCCESS == tx_semaphore_get(&p_service->event_quota, TX_NO_WAIT))
    {
        etc_service_completion_request_t completion_request = {.type = ETC_SERVICE_REQUEST_TYPE_PROCESS_EVENT,
                                                               .event_request.p_event = p_event};
        tx_queue_send(&p_service->work_queue, (void*)&completion_request, TX_NO_WAIT);
    }
}

uint64_t etc_get_ticks_cb()
{
    return 1;
}

void etc_worker_thread_func(ULONG thread_input)
{
    etc_service_context_t* p_service = (etc_service_context_t*)thread_input;

    // Infinite loop that will decode and recycle buffers marked as completed
    while (true)
    {
        etc_service_completion_request_t completion_request = {0};
        // This will always succeed. As the queue is created before this is called and the queue
        // capacity can contain the maximum number of requests.
        (void)tx_queue_receive(&p_service->work_queue, (void*)&completion_request, TX_WAIT_FOREVER);

        // Call the processing function if available, otherwise call internal processing.
        if (p_service->process_request.func != NULL)
        {
            p_service->process_request.func(p_service->process_request.p_context, &completion_request);
        }
        else
        {
            etc_process_request(p_service, &completion_request);
        }
    }
}

void etc_process_request(etc_service_context_t* p_service, etc_service_completion_request_t* p_request)
{
    switch (p_request->type)
    {
    case ETC_SERVICE_REQUEST_TYPE_PROCESS_CORE_BUFFER: {
        (void)FPFwETControllerRecycleBuffer(p_request->core_buffer_request.p_origin_controller,
                                            p_request->core_buffer_request.p_core_buffer->BufferId);
    }
    break;

    case ETC_SERVICE_REQUEST_TYPE_PROCESS_EVENT: {
        uint32_t size;
        (void)FPFwETDecoderPrintEvent(p_request->event_request.p_event, &size);
        (void)tx_semaphore_put(&p_service->event_quota);
    }
    break;

    default:
        break;
    }
}
