//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_collector.cpp
 * Event Trace Collector test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep

extern "C" {
#include "IFpFwEventTracingStatus.h" // for FPFW_ET_SUCCESS

#include <scp_event_trace_collector.h> // for scp_etc_initialize
#include <tx_api.h>                    // for TX_SUCCESS
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

TEST_FUNCTION(init, NULL, NULL)
{
    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_any(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_create, name_ptr);
    expect_any(__wrap__txe_semaphore_create, initial_count);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);

    expect_any(__wrap__txe_thread_create, thread_ptr);
    expect_any(__wrap__txe_thread_create, name_ptr);
    expect_any(__wrap__txe_thread_create, entry_function);
    expect_any(__wrap__txe_thread_create, entry_input);
    expect_any(__wrap__txe_thread_create, stack_start);
    expect_any(__wrap__txe_thread_create, stack_size);
    expect_any(__wrap__txe_thread_create, priority);
    expect_any(__wrap__txe_thread_create, preempt_threshold);
    expect_any(__wrap__txe_thread_create, time_slice);
    expect_any(__wrap__txe_thread_create, auto_start);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    expect_any(__wrap_FPFwETDecoderInit, pProviderMetadataStart);
    expect_any(__wrap_FPFwETDecoderInit, pProviderMetadataEnd);
    expect_any(__wrap_FPFwETDecoderInit, pEventMetadataStart);
    expect_any(__wrap_FPFwETDecoderInit, pEventMetadataEnd);
    will_return(__wrap_FPFwETDecoderInit, FPFW_ET_SUCCESS);

    expect_any(__wrap_FPFwETTraceInitialize, pController);
    expect_any(__wrap_FPFwETTraceInitialize, pConfig);
    will_return(__wrap_FPFwETTraceInitialize, FPFW_ET_SUCCESS);

    scp_etc_initialize();
}
