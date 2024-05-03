//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_collector.cpp
 * Event Trace Collector test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include "IFpFwEventTracingStatus.h" // for FPFW_ET_E_INVALIDARG, FPFW_...

#include <IFpFwEventTracingBuffers.h>
#include <IFpFwEventTracingController.h>
#include <error_handler.h>
#include <event_trace_collector.h>
#include <event_trace_collector_i.h>
#include <thread_x_mocks.h>
#include <tx_api.h>
#include <tx_initialize.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// clang-format off
static etc_service_config_t s_etc_config =
{
    .mode = ETC_SERVICE_MODE_STDIO,
    .trace_buffer_memory =
    {
        .p_pool = (void*)0x7000000,  // NOLINT - Test value
        .byte_count = 100,           // NOLINT - Test value
    },
    .thread_config =
    {
        .p_stack = (void*)0x7000000,      // NOLINT - Test value
        .stack_size = 0x1000,             // NOLINT - Test value
        .priority = 15,                   // NOLINT - Test value
        .time_slice_option = TX_NO_TIME_SLICE,
    },
    .event_manifest =
    {
        .provider_start = 0x7001000, // NOLINT - Test value
        .provider_end = 0x7002000,   // NOLINT - Test value
        .event_start = 0x7003000,    // NOLINT - Test value
        .event_end = 0x7004000       // NOLINT - Test value
    },
    .provider_filters =
    {
        .start = 0x7006000,         // NOLINT - Test value
        .end = 0x7007000            // NOLINT - Test value
    }
};
// clang-format on

/*------------- Functions ----------------*/

TEST_FUNCTION(init_nominal, NULL, NULL)
{
    etc_service_context_t service = {};

    expect_value(__wrap__txe_queue_create, queue_ptr, &service.work_queue);
    expect_string(__wrap__txe_queue_create, name_ptr, ETC_WORK_QUEUE_NAME);
    expect_value(__wrap__txe_queue_create, message_size, sizeof(etc_service_completion_request_t) / sizeof(uint32_t));
    expect_value(__wrap__txe_queue_create, queue_start, service.request_pool);
    expect_value(__wrap__txe_queue_create, queue_size, sizeof(service.request_pool));
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_value(__wrap__txe_semaphore_create, semaphore_ptr, &service.event_quota);
    expect_string(__wrap__txe_semaphore_create, name_ptr, ETC_EVENT_QUOTA_SEM_NAME);
    expect_value(__wrap__txe_semaphore_create, initial_count, ETC_SERVICE_EVENT_QUOTA);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);

    expect_value(__wrap__txe_thread_create, thread_ptr, &service.worker_thread);
    expect_string(__wrap__txe_thread_create, name_ptr, ETC_WORKER_THREAD_NAME);
    expect_any(__wrap__txe_thread_create, entry_function);
    expect_value(__wrap__txe_thread_create, entry_input, &service);
    expect_value(__wrap__txe_thread_create, stack_start, s_etc_config.thread_config.p_stack);
    expect_value(__wrap__txe_thread_create, stack_size, s_etc_config.thread_config.stack_size);
    expect_value(__wrap__txe_thread_create, priority, s_etc_config.thread_config.priority);
    expect_value(__wrap__txe_thread_create, preempt_threshold, s_etc_config.thread_config.priority);
    expect_value(__wrap__txe_thread_create, time_slice, s_etc_config.thread_config.time_slice_option);
    expect_value(__wrap__txe_thread_create, auto_start, TX_AUTO_START);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    expect_value(__wrap_FPFwETDecoderInit, pProviderMetadataStart, s_etc_config.event_manifest.provider_start);
    expect_value(__wrap_FPFwETDecoderInit, pProviderMetadataEnd, s_etc_config.event_manifest.provider_end);
    expect_value(__wrap_FPFwETDecoderInit, pEventMetadataStart, s_etc_config.event_manifest.event_start);
    expect_value(__wrap_FPFwETDecoderInit, pEventMetadataEnd, s_etc_config.event_manifest.event_end);
    will_return(__wrap_FPFwETDecoderInit, FPFW_ET_SUCCESS);

    expect_value(__wrap_FPFwETTraceInitialize, pController, &service.controller);
    expect_any(__wrap_FPFwETTraceInitialize, pConfig);
    will_return(__wrap_FPFwETTraceInitialize, FPFW_ET_SUCCESS);

    etc_initialize(&service, &s_etc_config);
}

TEST_FUNCTION(init_fail, NULL, NULL)
{
    etc_service_context_t service = {};

    // Bad args
    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        etc_initialize(nullptr, &s_etc_config);
    }

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        etc_initialize(&service, nullptr);
    }

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        s_etc_config.mode = ETC_SERVICE_MODE_MAX;
        etc_initialize(&service, &s_etc_config);
    }

    s_etc_config.mode = ETC_SERVICE_MODE_STDIO;

    // TX QUEUE fails
    expect_any_always(__wrap__txe_queue_create, queue_ptr);
    expect_any_always(__wrap__txe_queue_create, name_ptr);
    expect_any_always(__wrap__txe_queue_create, message_size);
    expect_any_always(__wrap__txe_queue_create, queue_start);
    expect_any_always(__wrap__txe_queue_create, queue_size);
    expect_any_always(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_NO_MEMORY);

    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NO_MEMORY);
    if (!set_error_handler_return())
    {
        etc_initialize(&service, &s_etc_config);
    }

    // TX SEMAPHORE fails
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_any_always(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any_always(__wrap__txe_semaphore_create, name_ptr);
    expect_any_always(__wrap__txe_semaphore_create, initial_count);
    expect_any_always(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_FEATURE_NOT_ENABLED);

    expect_value(FPFwErrorRaise, error, (uint32_t)TX_FEATURE_NOT_ENABLED);
    if (!set_error_handler_return())
    {
        etc_initialize(&service, &s_etc_config);
    }

    // TX THREAD fails
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);

    expect_any_always(__wrap__txe_thread_create, thread_ptr);
    expect_any_always(__wrap__txe_thread_create, name_ptr);
    expect_any_always(__wrap__txe_thread_create, entry_function);
    expect_any_always(__wrap__txe_thread_create, entry_input);
    expect_any_always(__wrap__txe_thread_create, stack_start);
    expect_any_always(__wrap__txe_thread_create, stack_size);
    expect_any_always(__wrap__txe_thread_create, priority);
    expect_any_always(__wrap__txe_thread_create, preempt_threshold);
    expect_any_always(__wrap__txe_thread_create, time_slice);
    expect_any_always(__wrap__txe_thread_create, auto_start);
    expect_any_always(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_NOT_DONE);

    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NOT_DONE);
    if (!set_error_handler_return())
    {
        etc_initialize(&service, &s_etc_config);
    }

    // Decoder fails
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    expect_any_always(__wrap_FPFwETDecoderInit, pProviderMetadataStart);
    expect_any_always(__wrap_FPFwETDecoderInit, pProviderMetadataEnd);
    expect_any_always(__wrap_FPFwETDecoderInit, pEventMetadataStart);
    expect_any_always(__wrap_FPFwETDecoderInit, pEventMetadataEnd);
    will_return(__wrap_FPFwETDecoderInit, FPFW_ET_E_INVALIDARG);

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        etc_initialize(&service, &s_etc_config);
    }

    // Controller fails
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);
    will_return(__wrap_FPFwETDecoderInit, TX_SUCCESS);

    expect_any_always(__wrap_FPFwETTraceInitialize, pController);
    expect_any_always(__wrap_FPFwETTraceInitialize, pConfig);
    will_return(__wrap_FPFwETTraceInitialize, FPFW_ET_E_INVALIDARG);

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        etc_initialize(&service, &s_etc_config);
    }
}

// TEST PRIVATE ETC FUNCTIONS

TEST_FUNCTION(process_event_request, NULL, NULL)
{
    etc_service_context_t service = {};
    etc_service_completion_request_t completion_request = {.type = ETC_SERVICE_REQUEST_TYPE_PROCESS_EVENT,
                                                           .event_request = {.p_event = (void*)0x7006000}}; // NOLINT

    expect_value(__wrap__txe_semaphore_put, semaphore_ptr, &service.event_quota);
    will_return(__wrap__txe_semaphore_put, TX_SUCCESS);

    expect_value(__wrap_FPFwETDecoderPrintEvent, pEvent, (void*)0x7006000); // NOLINT
    expect_any(__wrap_FPFwETDecoderPrintEvent, eventSize);
    will_return(__wrap_FPFwETDecoderPrintEvent, TX_SUCCESS);

    etc_process_request(&service, &completion_request);
}

TEST_FUNCTION(process_core_buffer_request, NULL, NULL)
{
    etc_service_context_t service = {};
    FPFW_ET_CORE_BUFFER_HEADER coreBuffer = {.BufferId = 10}; // NOLINT
    etc_service_completion_request_t completion_request = {
        .type = ETC_SERVICE_REQUEST_TYPE_PROCESS_CORE_BUFFER,
        .core_buffer_request = {(PFPFW_ET_CORE_BUFFER_HEADER)&coreBuffer, (PFPFW_ET_CONTROLLER)0x600}}; // NOLINT

    expect_value(__wrap_FPFwETControllerRecycleBuffer, pTraceController, (PFPFW_ET_CONTROLLER)0x600);
    expect_value(__wrap_FPFwETControllerRecycleBuffer, bufIndex, coreBuffer.BufferId);
    will_return(__wrap_FPFwETControllerRecycleBuffer, TX_SUCCESS);

    etc_process_request(&service, &completion_request);
}

TEST_FUNCTION(on_event_callback_nominal, NULL, NULL)
{
    threadx_mock_set_system_state(~TX_INITIALIZE_IN_PROGRESS);

    etc_service_completion_request_t completion_request = {.type = ETC_SERVICE_REQUEST_TYPE_PROCESS_EVENT,
                                                           .event_request = {.p_event = (void*)0x7006000}}; // NOLINT

    etc_service_context_t service = {};

    expect_value(__wrap__txe_semaphore_get, semaphore_ptr, &service.event_quota);
    expect_value(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_semaphore_get, TX_SUCCESS);

    expect_value(__wrap__txe_queue_send, queue_ptr, &service.work_queue);
    expect_memory(__wrap__txe_queue_send,
                  source_ptr,
                  &completion_request,
                  sizeof(completion_request.type) + sizeof(completion_request.event_request));
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    etc_on_event_cb((void*)0x7006000, &service); // NOLINT
}

TEST_FUNCTION(on_event_callback_no_space, NULL, NULL)
{
    threadx_mock_set_system_state(~TX_INITIALIZE_IN_PROGRESS);

    etc_service_context_t service = {};

    expect_value(__wrap__txe_semaphore_get, semaphore_ptr, &service.event_quota);
    expect_value(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_semaphore_get, TX_NO_MEMORY);

    etc_on_event_cb((void*)0x7006000, &service); // NOLINT
}

TEST_FUNCTION(boottime_on_event_callback, NULL, NULL)
{
    threadx_mock_set_system_state(TX_INITIALIZE_IN_PROGRESS);

    etc_service_context_t service = {};
    void* event = (void*)0x7006000; // NOLINT

    expect_value(__wrap_FPFwETDecoderPrintEvent, pEvent, event);
    expect_any(__wrap_FPFwETDecoderPrintEvent, eventSize);
    will_return(__wrap_FPFwETDecoderPrintEvent, TX_SUCCESS);

    etc_on_event_cb(event, &service);
}

TEST_FUNCTION(buffer_complete_cb, NULL, NULL)
{
    etc_service_completion_request_t completion_request = {
        .type = ETC_SERVICE_REQUEST_TYPE_PROCESS_CORE_BUFFER,
        .core_buffer_request = {(PFPFW_ET_CORE_BUFFER_HEADER)0x700, (PFPFW_ET_CONTROLLER)0x600}}; // NOLINT

    etc_service_context_t service = {};

    expect_value(__wrap__txe_queue_send, queue_ptr, &service.work_queue);
    expect_memory(__wrap__txe_queue_send, source_ptr, &completion_request, sizeof(completion_request));
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    etc_buffer_complete_cb(&service, (PFPFW_ET_CORE_BUFFER_HEADER)0x700, (PFPFW_ET_CONTROLLER)0x600); // NOLINT
}

TEST_FUNCTION(get_ticks, NULL, NULL)
{
    assert_true(etc_get_ticks_cb() > 0);
}
