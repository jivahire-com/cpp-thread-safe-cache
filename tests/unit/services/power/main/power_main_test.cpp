//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_main_test.cpp
 * Power service main tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

#include <CMockaWrapper.h>     // for expect_value, check_expected_ptr, Cmo...
#include <DfwkCommon.h>        // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUE...
#include <power_dfwk.h>        // for power_service_t, power_service_interf...
#include <power_hw_int_i.h>    // for power_telcfg_t
#include <power_init.h>        // for power_init, power_interface_init
#include <power_runconfig.h>   // for power_service_config_t
#include <power_runconfig_i.h> // for power_runconfig_t

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

DFWK_ASYNC_REQUEST_DISPATCH s_dispatch_routine = NULL;
DFWK_REQUEST_DISPATCH_SYNC s_dispatch_routine_sync = NULL;
const power_telcfg_t* sp_telemetry_config;

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

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

// wrap for power_runconfig_init
void __wrap_power_runconfig_init(const power_service_config_t* p_config)
{
    check_expected_ptr(p_config);
}

// wrap for power_telemetry_init_config
void __wrap_power_telemetry_init_config(const power_telcfg_t* p_telemetry_config)
{
    assert_non_null(p_telemetry_config);
    sp_telemetry_config = p_telemetry_config;
}

// wrap for power_init_soc
void __wrap_power_init_soc(const power_runconfig_t* p_runconfig)
{
    check_expected_ptr(p_runconfig);
}

// wrap for power_init_core
void __wrap_power_init_core(const power_runconfig_t* p_runconfig, const power_telcfg_t* p_telemetry_config)
{
    check_expected_ptr(p_runconfig);
    assert_int_equal((uintptr_t)p_telemetry_config, (uintptr_t)sp_telemetry_config);
}

// wrap for power_runconfig_get
power_runconfig_t* __wrap_power_runconfig_get()
{
    return mock_type(power_runconfig_t*);
}

} // extern "C"

//
// Tests
//
POWER_TEST(init, NULL, NULL)
{
    power_service_t test_device;
    power_service_config_t test_config;
    power_runconfig_t test_runconfig;

    DFWK_SCHEDULE test_schedule;

    expect_value(__wrap_DfwkDeviceInitialize, Device, &test_device.header);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &test_schedule);
    expect_value(__wrap_DfwkQueueInitialize, Queue, &test_device.default_queue);
    expect_value(__wrap_DfwkQueueInitialize, Device, &test_device.header);
    expect_any(__wrap_DfwkQueueInitialize, DispatchRoutine);
    expect_value(__wrap_DfwkQueueInitialize, DispatchContext, &test_device.header);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_SerializedDispatch);

    // add the expected/check values for power internal functions
    expect_value(__wrap_power_runconfig_init, p_config, &test_config);
    will_return(__wrap_power_runconfig_get, &test_runconfig);
    expect_value(__wrap_power_init_soc, p_runconfig, &test_runconfig);
    expect_value(__wrap_power_init_core, p_runconfig, &test_runconfig);

    power_init(&test_device, &test_schedule, &test_config);
}

POWER_TEST(interface_init, NULL, NULL)
{
    power_service_t test_device;
    power_service_interface_t test_interface;

    expect_value(__wrap_DfwkInterfaceInitialize, Interface, &test_interface.header);
    expect_value(__wrap_DfwkInterfaceInitialize, Device, &test_device.header);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchQueue, &test_device.default_queue);
    expect_any(__wrap_DfwkInterfaceInitialize, DispatchSync);

    power_interface_init(&test_device, &test_interface);
}

POWER_TEST(dispatch_sync_default, NULL, NULL)
{
    DFWK_SYNC_REQUEST_HEADER test_request;
    test_request.RequestType = -1; // invalid request type

    expect_value(__wrap_FpFwAssert, expression, false);

    if (s_dispatch_routine_sync)
    {
        s_dispatch_routine_sync(&test_request);
    }
}

POWER_TEST(dispatch_default, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER test_request;
    power_service_t test_device;
    test_request.RequestType = -1; // invalid request type

    expect_value(__wrap_FpFwAssert, expression, false);

    if (s_dispatch_routine)
    {
        s_dispatch_routine(&test_request, &test_device.header);
    }
}
