//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_thread_test.cpp
 * Startup shutdown service thread tests
 */

/*------------- Includes -----------------*/
#include "sos_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

#include <CMockaWrapper.h> // for check_expected, check_expected_ptr
#include <DfwkCommon.h>    // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <FpFwLinkedList.h>
#include <FpFwUtils.h>            // for FPFW_ARRAY_SIZE
#include <startup_shutdown.h>     // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_i.h>   // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_ssi.h> // for _ssi_startup_stage, _startup_type
#include <string.h>
#include <tx_api.h> // for ULONG, UINT, VOID, TX_EVENT_FLAGS_...

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
sos_queue_entry_t s_queue_entry;

startup_ssi_registration_t s_test_registrations[5];
DFWK_INTERFACE_HEADER s_test_interfaces[5];
sos_service_context_t s_test_service_ctx = {};

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
// mocks for ThreadX (tx) APIs
int32_t __wrap__txe_thread_create(TX_THREAD* thread_ptr,
                                  CHAR* name_ptr,
                                  VOID (*entry_function)(ULONG entry_input),
                                  ULONG entry_input,
                                  VOID* stack_start,
                                  ULONG stack_size,
                                  UINT priority,
                                  UINT preempt_threshold,
                                  ULONG time_slice,
                                  UINT auto_start)
{
    check_expected_ptr(thread_ptr);
    check_expected_ptr(name_ptr);
    check_expected_ptr(entry_function);
    check_expected(entry_input);
    check_expected_ptr(stack_start);
    check_expected(stack_size);
    check_expected(priority);
    check_expected(preempt_threshold);
    check_expected(time_slice);
    check_expected(auto_start);
    return mock_type(int32_t);
}

int32_t __wrap__txe_event_flags_create(TX_EVENT_FLAGS_GROUP* event_flags_group_ptr, const char* name_ptr)
{
    check_expected_ptr(event_flags_group_ptr);
    check_expected_ptr(name_ptr);
    return mock_type(int32_t);
}

int32_t __wrap__txe_event_flags_set(TX_EVENT_FLAGS_GROUP* event_flags_group_ptr, ULONG flags, UINT options)
{
    check_expected_ptr(event_flags_group_ptr);
    check_expected(flags);
    check_expected(options);
    return mock_type(int32_t);
}

UINT __wrap__txe_event_flags_get(TX_EVENT_FLAGS_GROUP* group_ptr, ULONG requested_flags, UINT get_option, ULONG* actual_flags_ptr, ULONG wait_option)
{
    check_expected_ptr(group_ptr);
    check_expected(requested_flags);
    check_expected(get_option);
    check_expected_ptr(actual_flags_ptr);
    check_expected(wait_option);
    return mock_type(UINT);
}

int32_t __wrap__txe_queue_create(TX_QUEUE* queue_ptr, const char* name_ptr, UINT message_size, VOID* queue_start, ULONG queue_size)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(name_ptr);
    check_expected(message_size);
    check_expected_ptr(queue_start);
    check_expected(queue_size);
    return mock_type(int32_t);
}

int32_t __wrap__txe_queue_send(TX_QUEUE* queue_ptr, VOID* source_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(source_ptr);
    check_expected(wait_option);
    memcpy(&s_queue_entry, source_ptr, sizeof(sos_queue_entry_t));
    return mock_type(int32_t);
}
void __wrap_ssi_startup_stage_start(PDFWK_INTERFACE_HEADER p_interface,
                                    pssi_request_t p_request,
                                    ssi_startup_stage_t stage,
                                    ssi_startup_type_t boot_type,
                                    DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                    void* p_completion_context)
{

    check_expected_ptr(p_interface);
    check_expected_ptr(p_request);
    check_expected(stage);
    check_expected(boot_type);
    check_expected_ptr(completion_routine);
    check_expected_ptr(p_completion_context);
}

void __wrap_ssi_startup_stage_complete(PDFWK_INTERFACE_HEADER p_interface,
                                       pssi_request_t p_request,
                                       ssi_startup_stage_t stage,
                                       ssi_startup_type_t boot_type,
                                       DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                       void* p_completion_context)
{
    check_expected_ptr(p_interface);
    check_expected_ptr(p_request);
    check_expected(stage);
    check_expected(boot_type);
    check_expected_ptr(completion_routine);
    check_expected_ptr(p_completion_context);
}

void __wrap_ssi_shutdown_quiesce(PDFWK_INTERFACE_HEADER p_interface,
                                 pssi_request_t p_request,
                                 ssi_shutdown_type_t shutdown_type,
                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                 void* p_completion_context)
{
    check_expected_ptr(p_interface);
    check_expected_ptr(p_request);
    check_expected(shutdown_type);
    check_expected_ptr(completion_routine);
    check_expected_ptr(p_completion_context);
}

unsigned __wrap_sos_core_boot_stage_count()
{
    return mock_type(unsigned);
}
const startup_shutdown_boot_stage_t* __wrap_sos_core_boot_stages()
{
    return mock_type(const startup_shutdown_boot_stage_t*);
}

extern void __real_sos_thread_init(psos_service_context_t p_context);
extern void __real_sos_queue_start_phase(ssi_startup_type_t boot_type, ssi_startup_stage_t phase, PDFWK_ASYNC_REQUEST_HEADER p_request);
extern void __real_sos_queue_shutdown(ssi_shutdown_type_t shutdown_type, PDFWK_ASYNC_REQUEST_HEADER p_request);
extern void __real_FpFwListInitialize(PFPFW_LIST_HANDLE list);
extern void __real_FpFwListInsertTail(PFPFW_LIST_HANDLE list, PFPFW_LIST_ENTRY newEntry);
} // extern "C"

//
// Tests
//

#define TEST_STAGE_COUNT (3)
startup_shutdown_boot_stage_t test_stages[TEST_STAGE_COUNT] = {
    {(ssi_startup_stage_t)1, (ssi_startup_stage_t)1, 0, false, false},
    {(ssi_startup_stage_t)2, (ssi_startup_stage_t)2, 0, false, false},
    {(ssi_startup_stage_t)3, (ssi_startup_stage_t)3, 0, false, false}};

// test for sos_queue_find_phase
SOS_TEST(sos_queue_find_phase, NULL, NULL)
{
#define TEST_PHASE ((ssi_startup_stage_t)2)
    will_return_always(__wrap_sos_core_boot_stage_count, TEST_STAGE_COUNT);
    will_return_always(__wrap_sos_core_boot_stages, &test_stages);

    // call the function
    assert_int_equal(sos_queue_find_phase(TEST_PHASE), TEST_PHASE - 1);
}

SOS_TEST(sos_queue_find_phase__not_found, NULL, NULL)
{
#define PHASE_INDEX_NOT_FOUND (-1)
#define TEST_PHASE_NOT_FOUND  ((ssi_startup_stage_t)0)
    will_return_always(__wrap_sos_core_boot_stage_count, TEST_STAGE_COUNT);
    will_return_always(__wrap_sos_core_boot_stages, &test_stages);

    // call the function
    assert_int_equal(sos_queue_find_phase(TEST_PHASE_NOT_FOUND), PHASE_INDEX_NOT_FOUND);
}

SOS_TEST(sos_completion, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER test_request = {};
    void* test_completion_context = (void*)0x5678;

    // expectations for ThreadX APIs
    expect_value(__wrap__txe_event_flags_set, flags, (uint32_t)test_completion_context);
    expect_value(__wrap__txe_event_flags_set, options, TX_OR);
    expect_not_value(__wrap__txe_event_flags_set, event_flags_group_ptr, NULL);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    // call the function
    sos_completion(&test_request, test_completion_context);
}

void setup_registrations()
{
    __real_FpFwListInitialize(&s_test_service_ctx.ssi_registrations);
    for (unsigned int i = 0; i < FPFW_ARRAY_SIZE(s_test_registrations); i++)
    {
        __real_FpFwListInsertTail(&s_test_service_ctx.ssi_registrations, &s_test_registrations[i].list_entry);
        s_test_registrations[i].p_ssi_interface = &s_test_interfaces[i];
        s_test_registrations[i].interface_unique_flag = i;
    }
}

SOS_TEST(sos_notify_ssi_boot_stage, NULL, NULL)
{
    ssi_startup_stage_t test_stage = STARTUP_MCP_LOAD;
    ssi_startup_type_t test_boot_type = COLD_BOOT;

    setup_registrations();

    unsigned count = FPFW_ARRAY_SIZE(s_test_registrations);

    for (unsigned i = 0; i < count; i++)
    {
        expect_value(__wrap_ssi_startup_stage_start, p_interface, (uintptr_t)(s_test_registrations[i].p_ssi_interface));
        expect_value(__wrap_ssi_startup_stage_start, p_request, (uintptr_t)&s_test_registrations[i].ssi_request);
        expect_value(__wrap_ssi_startup_stage_start, stage, test_stage);
        expect_value(__wrap_ssi_startup_stage_start, boot_type, test_boot_type);
        expect_any(__wrap_ssi_startup_stage_start, completion_routine);
        expect_value(__wrap_ssi_startup_stage_start, p_completion_context, s_test_registrations[i].interface_unique_flag);
    }

    // call the function
    sos_notify_ssi_boot_stage(&s_test_service_ctx, test_stage, test_boot_type, true);
}

void setup_expectations_for_boot_stage_complete(ssi_startup_stage_t test_stage, ssi_startup_type_t test_boot_type)
{
    setup_registrations();

    unsigned count = FPFW_ARRAY_SIZE(s_test_registrations);

    for (unsigned i = 0; i < count; i++)
    {
        expect_value(__wrap_ssi_startup_stage_complete, p_interface, (uintptr_t)(s_test_registrations[i].p_ssi_interface));
        expect_value(__wrap_ssi_startup_stage_complete, p_request, (uintptr_t)&s_test_registrations[i].ssi_request);
        expect_value(__wrap_ssi_startup_stage_complete, stage, test_stage);
        expect_value(__wrap_ssi_startup_stage_complete, boot_type, test_boot_type);
        expect_any(__wrap_ssi_startup_stage_complete, completion_routine);
        expect_value(__wrap_ssi_startup_stage_complete, p_completion_context, s_test_registrations[i].interface_unique_flag);
    }
}

SOS_TEST(sos_notify_ssi_boot_stage_complete, NULL, NULL)
{
    ssi_startup_stage_t test_stage = STARTUP_MCP_LOAD;
    ssi_startup_type_t test_boot_type = COLD_BOOT;

    setup_expectations_for_boot_stage_complete(test_stage, test_boot_type);

    // call the function
    sos_notify_ssi_boot_stage(&s_test_service_ctx, test_stage, test_boot_type, false);
}

SOS_TEST(sos_notify_ssi_shutdown, NULL, NULL)
{
    ssi_shutdown_type_t test_shutdown = SHUTDOWN;

    setup_registrations();

    unsigned count = FPFW_ARRAY_SIZE(s_test_registrations);

    for (unsigned i = 0; i < count; i++)
    {
        expect_value(__wrap_ssi_shutdown_quiesce, p_interface, (uintptr_t)(s_test_registrations[i].p_ssi_interface));
        expect_value(__wrap_ssi_shutdown_quiesce, p_request, (uintptr_t)&s_test_registrations[i].ssi_request);
        expect_value(__wrap_ssi_shutdown_quiesce, shutdown_type, test_shutdown);
        expect_any(__wrap_ssi_shutdown_quiesce, completion_routine);
        expect_value(__wrap_ssi_shutdown_quiesce, p_completion_context, s_test_registrations[i].interface_unique_flag);
    }

    // call the function
    sos_notify_ssi_shutdown(&s_test_service_ctx, test_shutdown);
}

void setup_wait_ssi_complete_expectations()
{
    // expectations for ThreadX APIs
    expect_not_value(__wrap__txe_event_flags_get, group_ptr, NULL);
    expect_any(__wrap__txe_event_flags_get, requested_flags);
    expect_value(__wrap__txe_event_flags_get, get_option, TX_AND_CLEAR);
    expect_not_value(__wrap__txe_event_flags_get, actual_flags_ptr, NULL);
    expect_value(__wrap__txe_event_flags_get, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);
}

// test for wait_ssi_complete
SOS_TEST(sos_notify_ssi_boot_stage_and_wait, NULL, NULL)
{
    ssi_startup_stage_t test_stage = STARTUP_MCP_LOAD;
    ssi_startup_type_t test_boot_type = COLD_BOOT;

    // setup expectations for complete notification
    setup_expectations_for_boot_stage_complete(test_stage, test_boot_type);

    // setup expectations for wait
    setup_wait_ssi_complete_expectations();

    // call the function
    sos_notify_ssi_boot_stage_and_wait(&s_test_service_ctx, test_stage, test_boot_type, false);
}

// test for sos_thread_init

SOS_TEST(sos_thread_init, NULL, NULL)
{
#define SOS_THREAD_PRIORITY          (10)
#define SOS_THREAD_PREEMPT_THRESHOLD (10)

    sos_service_context_t test_service_ctx;

    // expectations for queue create API
    expect_not_value(__wrap__txe_queue_create, queue_ptr, NULL);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_value(__wrap__txe_queue_create, message_size, sizeof(sos_queue_entry_t) / sizeof(uint32_t));
    expect_not_value(__wrap__txe_queue_create, queue_start, NULL);
    expect_not_value(__wrap__txe_queue_create, queue_size, 0);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    // expectations for event flags create API
    expect_not_value(__wrap__txe_event_flags_create, event_flags_group_ptr, NULL);
    expect_any(__wrap__txe_event_flags_create, name_ptr);
    will_return(__wrap__txe_event_flags_create, TX_SUCCESS);

    // expectations for ThreadX APIs
    expect_not_value(__wrap__txe_thread_create, thread_ptr, NULL);
    expect_any(__wrap__txe_thread_create, name_ptr);
    expect_not_value(__wrap__txe_thread_create, entry_function, NULL);
    expect_value(__wrap__txe_thread_create, entry_input, &test_service_ctx);
    expect_not_value(__wrap__txe_thread_create, stack_start, NULL);
    expect_not_value(__wrap__txe_thread_create, stack_size, 0);
    expect_value(__wrap__txe_thread_create, priority, SOS_THREAD_PRIORITY);
    expect_value(__wrap__txe_thread_create, preempt_threshold, SOS_THREAD_PREEMPT_THRESHOLD);
    expect_value(__wrap__txe_thread_create, time_slice, TX_NO_TIME_SLICE);
    expect_value(__wrap__txe_thread_create, auto_start, TX_AUTO_START);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    // call the function
    __real_sos_thread_init(&test_service_ctx);
}

// test for sos_queue_start_phase
SOS_TEST(sos_queue_start_phase, NULL, NULL)
{
    ssi_startup_type_t test_boot_type = COLD_BOOT;
    ssi_startup_stage_t test_stage = STARTUP_MCP_LOAD;

    // expectations for ThreadX APIs
    expect_not_value(__wrap__txe_queue_send, queue_ptr, NULL);
    expect_not_value(__wrap__txe_queue_send, source_ptr, NULL);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // call the function
    __real_sos_queue_start_phase(test_boot_type, test_stage, NULL);

    // verify observable output
    assert_int_equal(s_queue_entry.data.boot_phase.boot_type, test_boot_type);
    assert_int_equal(s_queue_entry.data.boot_phase.phase, test_stage);
}

// test for sos_queue_shutdown
SOS_TEST(sos_queue_shutdown, NULL, NULL)
{
    ssi_shutdown_type_t test_shutdown_type = SHUTDOWN;

    // expectations for ThreadX APIs
    expect_not_value(__wrap__txe_queue_send, queue_ptr, NULL);
    expect_not_value(__wrap__txe_queue_send, source_ptr, NULL);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // call the function
    __real_sos_queue_shutdown(test_shutdown_type, NULL);

    // verify observable output
    assert_int_equal(s_queue_entry.data.shutdown_type, test_shutdown_type);
}