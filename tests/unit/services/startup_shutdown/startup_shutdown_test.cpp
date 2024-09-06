//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_test.cpp
 * Startup shutdown service main tests
 */

/*------------- Includes -----------------*/
#include "sos_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

#include <CMockaWrapper.h>  // for expect_value, check_expected_ptr, Cmo...
#include <DfwkCommon.h>     // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUE...
#include <DfwkStatus.h>     // for DFWK_SUCCESS
#include <FpFwLinkedList.h> // for PFPFW_LIST_HANDLE, FPFW_LIST_HANDLE
#include <startup_shutdown.h>
#include <startup_shutdown_i.h>
#include <startup_shutdown_init.h>
#include <startup_shutdown_ssi.h>

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

DFWK_ASYNC_REQUEST_DISPATCH s_dispatch_routine = NULL;
DFWK_REQUEST_DISPATCH_SYNC s_dispatch_routine_sync = NULL;
FPFW_LIST_HANDLE* sp_list = NULL;
sos_service_context_t* sp_sos_service_context;

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
// wrapper function that checks expected values for the incoming parameters
void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{

    check_expected_ptr(Interface);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchQueue);
    check_expected_ptr(DispatchSync);
    // save the dispatch routine for later use
    s_dispatch_routine_sync = DispatchSync;
}

// wrapper function that checks expected values for the two incoming parameters
void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    check_expected_ptr(Device);
    check_expected_ptr(Schedule);
}

// wrapper function that checks expected values for the incoming parameters
void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    check_expected_ptr(Queue);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchRoutine);
    check_expected_ptr(DispatchContext);
    check_expected(QueueType);
    // save the dispatch routine for later use
    s_dispatch_routine = DispatchRoutine;
}

void __wrap_FpFwListInitialize(PFPFW_LIST_HANDLE list)
{
    check_expected_ptr(list);
    sp_list = list;
}

void __wrap_FpFwListInsertTail(PFPFW_LIST_HANDLE list, PFPFW_LIST_ENTRY newEntry)
{
    check_expected_ptr(list);
    check_expected_ptr(newEntry);
}

void __wrap_sos_thread_init(psos_service_context_t p_context)
{
    check_expected_ptr(p_context);
    sp_sos_service_context = p_context;
}

int32_t __wrap_DfwkInterfaceOpen(PDFWK_INTERFACE_HEADER Interface, PDFWK_DEVICE_HEADER ClientDevice)
{
    check_expected_ptr(Interface);
    check_expected_ptr(ClientDevice);
    return mock_type(int32_t);
}

void __wrap_sos_queue_start_phase(ssi_startup_type_t boot_type, ssi_startup_stage_t phase, PDFWK_ASYNC_REQUEST_HEADER p_request)
{
    check_expected(boot_type);
    check_expected(phase);
    check_expected_ptr(p_request);
}

void __wrap_sos_queue_shutdown(ssi_shutdown_type_t shutdown_type, PDFWK_ASYNC_REQUEST_HEADER p_request)
{
    check_expected(shutdown_type);
    check_expected_ptr(p_request);
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
}

void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    check_expected_ptr(Request);
    check_expected(RequestSize);
}

} // extern "C"

//
// Tests
//
SOS_TEST(init, NULL, NULL)
{
    sos_device_t test_device;

    DFWK_SCHEDULE test_schedule;

    expect_value(__wrap_DfwkDeviceInitialize, Device, &test_device.header);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &test_schedule);
    expect_value(__wrap_DfwkQueueInitialize, Queue, &test_device.default_queue);
    expect_value(__wrap_DfwkQueueInitialize, Device, &test_device.header);
    expect_any(__wrap_DfwkQueueInitialize, DispatchRoutine);
    expect_value(__wrap_DfwkQueueInitialize, DispatchContext, &test_device.header);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_ImmediateDispatch);

    expect_not_value(__wrap_FpFwListInitialize, list, NULL);
    expect_not_value(__wrap_sos_thread_init, p_context, NULL);

    sos_init(&test_device, &test_schedule);
    // verify that values passed to the list initialization function are the same as the values passed to the sos_service_context_t
    assert_int_equal((uintptr_t)&sp_sos_service_context->ssi_registrations, (uintptr_t)sp_list);
}

// interface init behaves differently based on whether shared flag is set or not
void interface_init_test(bool shared)
{
    sos_device_t test_device;
    sos_interface_t test_interface;

    expect_value(__wrap_DfwkInterfaceInitialize, Interface, &test_interface.header);
    expect_value(__wrap_DfwkInterfaceInitialize, Device, &test_device.header);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchQueue, &test_device.default_queue);
    expect_any(__wrap_DfwkInterfaceInitialize, DispatchSync);

    if (shared)
    {
        expect_value(__wrap_DfwkInterfaceOpen, Interface, &test_interface.header);
        expect_value(__wrap_DfwkInterfaceOpen, ClientDevice, &test_device.header);
        will_return(__wrap_DfwkInterfaceOpen, DFWK_SUCCESS);
    }

    sos_interface_init(&test_device, &test_interface, shared);
}

SOS_TEST(interface_init_shared, NULL, NULL)
{
    interface_init_test(true);
}

SOS_TEST(interface_init_non_shared, NULL, NULL)
{
    interface_init_test(false);
}

SOS_TEST(dispatch_async_STARTUP_REQUEST_START_PHASE_ASYNC, NULL, NULL)
{
#define TEST_BOOT_TYPE COLD_BOOT
#define TEST_PHASE     STARTUP_PHASE_AP_ASYNC
    startup_start_phase_request_t test_request;
    test_request.boot_type = TEST_BOOT_TYPE;
    test_request.stage = TEST_PHASE;
    test_request.header.async.RequestType = STARTUP_REQUEST_START_PHASE_ASYNC;

    expect_value(__wrap_sos_queue_start_phase, boot_type, TEST_BOOT_TYPE);
    expect_value(__wrap_sos_queue_start_phase, phase, TEST_PHASE);
    expect_value(__wrap_sos_queue_start_phase, p_request, &test_request);

    if (s_dispatch_routine)
    {
        s_dispatch_routine(&test_request.header.async, NULL);
    }
}

SOS_TEST(dispatch_async_SHUTDOWN_REQUEST_SHUTDOWN_ASYNC, NULL, NULL)
{
#define TEST_SHUTDOWN_TYPE MSCP_SUBSYS_RESET
    startup_shutdown_request_t test_request;
    test_request.shutdown_type = TEST_SHUTDOWN_TYPE;
    test_request.header.RequestType = STARTUP_REQUEST_SHUTDOWN_ASYNC;

    expect_value(__wrap_sos_queue_shutdown, shutdown_type, TEST_SHUTDOWN_TYPE);
    expect_value(__wrap_sos_queue_shutdown, p_request, &test_request);

    if (s_dispatch_routine)
    {
        s_dispatch_routine(&test_request.header, NULL);
    }
}

SOS_TEST(dispatch_async_SSI_STARTUP_STAGE_START_ASYNC, NULL, NULL)
{
    ssi_startup_notification_request_t test_request;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request);

    if (s_dispatch_routine)
    {
        s_dispatch_routine(&test_request.header, NULL);
    }
}

SOS_TEST(dispatch_async_SSI_STARTUP_STAGE_COMPLETE_ASYNC, NULL, NULL)
{
    ssi_startup_notification_request_t test_request;
    test_request.header.RequestType = SSI_STARTUP_STAGE_COMPLETE_ASYNC;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request);

    if (s_dispatch_routine)
    {
        s_dispatch_routine(&test_request.header, NULL);
    }
}

SOS_TEST(dispatch_async_SSI_SHUTDOWN_QUIESCE_ASYNC, NULL, NULL)
{
    ssi_shutdown_notification_request_t test_request;
    test_request.header.RequestType = SSI_SHUTDOWN_QUIESCE_ASYNC;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request);

    if (s_dispatch_routine)
    {
        s_dispatch_routine(&test_request.header, NULL);
    }
}

SOS_TEST(dispatch_sync_STARTUP_REGISTER_SSI_SYNC, NULL, NULL)
{
#define TEST_STARTING_REG_COUNT 1
    startup_register_ssi_request test_request = {};
    startup_ssi_registration_t test_registration = {};
    sos_interface_t test_interface = {};
    sos_device_t test_device = {};

    test_interface.header.OwningDevice = &test_device.header;
    test_interface.p_device = &test_device;
    test_registration.p_ssi_interface = &test_interface.header;

    test_request.header.RequestType = STARTUP_REGISTER_SSI_SYNC;
    test_request.header.OwningInterface = &test_interface.header;
    test_request.p_registration = &test_registration;

    assert_true(sp_sos_service_context != NULL);

    expect_value(__wrap_DfwkInterfaceOpen, Interface, &test_interface.header);
    expect_value(__wrap_DfwkInterfaceOpen, ClientDevice, &test_device.header);
    will_return(__wrap_DfwkInterfaceOpen, DFWK_SUCCESS);

    expect_value(__wrap_DfwkAsyncRequestInitialize, Request, &test_registration.ssi_request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(test_registration.ssi_request));

    expect_value(__wrap_FpFwListInsertTail, list, &sp_sos_service_context->ssi_registrations);
    expect_value(__wrap_FpFwListInsertTail, newEntry, &test_request.p_registration->list_entry);

    if (sp_sos_service_context)
    {
        sp_sos_service_context->registration_count = TEST_STARTING_REG_COUNT;
        if (s_dispatch_routine_sync)
        {
            s_dispatch_routine_sync(&test_request.header);
        }
        assert_int_equal(sp_sos_service_context->registration_count, TEST_STARTING_REG_COUNT + 1);
    }
    assert_int_equal(test_request.p_registration->interface_unique_flag, 1 << TEST_STARTING_REG_COUNT);
}

SOS_TEST(dispatch_sync_STARTUP_RESET_TIMEOUT_SYNC, NULL, NULL)
{
#define TEST_TIMEOUT 100

    startup_reset_timeout_request_t test_request = {};
    test_request.header.RequestType = STARTUP_RESET_TIMEOUT_SYNC;
    test_request.timeout_ms = TEST_TIMEOUT;

    sos_interface_t test_interface = {};
    test_request.header.OwningInterface = &test_interface.header;

    sos_device_t test_device = {};

    test_interface.header.OwningDevice = &test_device.header;
    test_interface.p_device = &test_device;

    // no expectations, currently

    if (s_dispatch_routine_sync)
    {
        s_dispatch_routine_sync(&test_request.header);
    }
}

SOS_TEST(dispatch_sync_STARTUP_REQUEST_START_PHASE_SYNC, NULL, NULL)
{
#define TEST_BOOT_TYPE COLD_BOOT
    startup_start_phase_request_t test_request = {};
    test_request.boot_type = TEST_BOOT_TYPE;
    test_request.stage = TEST_PHASE;
    test_request.header.sync.RequestType = STARTUP_REQUEST_START_PHASE_SYNC;

    sos_interface_t test_interface = {};
    test_request.header.sync.OwningInterface = &test_interface.header;

    sos_device_t test_device = {};

    test_interface.header.OwningDevice = &test_device.header;
    test_interface.p_device = &test_device;

    expect_value(__wrap_sos_queue_start_phase, boot_type, TEST_BOOT_TYPE);
    expect_value(__wrap_sos_queue_start_phase, phase, TEST_PHASE);
    expect_value(__wrap_sos_queue_start_phase, p_request, NULL);

    if (s_dispatch_routine_sync)
    {
        s_dispatch_routine_sync(&test_request.header.sync);
    }
}