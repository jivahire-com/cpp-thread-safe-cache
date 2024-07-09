//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_main_test.cpp
 * APcore service main tests
 */

/*------------- Includes -----------------*/
#include "ap_core_test.h"

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

#include <CMockaWrapper.h> // for expect_value, check_expected_ptr, Cmo...
#include <DfwkCommon.h>    // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUE...
#include <ap_core.h>
#include <ap_core_i.h>
#include <ap_core_init.h>
#include <corebits.h>
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#include <startup_shutdown_ssi.h>

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
bool should_mock_ap_core_ppu_init = true;

static DFWK_ASYNC_REQUEST_DISPATCH s_dispatch_routine = NULL;
static DFWK_REQUEST_DISPATCH_SYNC s_dispatch_routine_sync = NULL;
static pap_core_service_context_t s_ap_core_ctx = NULL;
static icc_base_recv_complete_notify fw_load_cb = NULL;
static uint32_t icc_hspmbx_ctx;

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

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

void __wrap_ap_core_util_get_fuse_enabled_cores(corebits_t* p_enabled_cores)
{
    assert_non_null(p_enabled_cores);
    check_expected_ptr(p_enabled_cores);
}

void __real_ap_core_ppu_init(ap_core_service_context_t* p_context);
void __wrap_ap_core_ppu_init(ap_core_service_context_t* p_context)
{
    if (!should_mock_ap_core_ppu_init)
    {
        __real_ap_core_ppu_init(p_context);
        return;
    }
    assert_non_null(p_context);
    check_expected_ptr(p_context);
    s_ap_core_ctx = p_context;
}

void __wrap_ap_core_ppu_clusters_on(ap_core_service_context_t* p_context)
{
    assert_non_null(p_context);
    check_expected_ptr(p_context);
}

void __wrap_ap_core_ppu_core_set_power_state(ap_core_service_context_t* p_context, unsigned core_idx, bool power_state_on)
{
    assert_non_null(p_context);
    check_expected_ptr(p_context);
    check_expected(core_idx);
    check_expected(power_state_on);
}

unsigned int __wrap_ap_core_util_boot_core(ap_core_service_context_t* p_context)
{
    assert_non_null(p_context);
    check_expected_ptr(p_context);
    return mock_type(unsigned int);
}

void __wrap_ap_core_util_set_rvbaraddr(ap_core_service_context_t* p_context, unsigned core_idx, uint64_t rvbaraddr)
{
    assert_non_null(p_context);
    check_expected_ptr(p_context);
    check_expected(core_idx);
    check_expected(rvbaraddr);
}

void __wrap_ap_core_util_set_all_rvbaraddr(ap_core_service_context_t* p_context, uint64_t rvbaraddr)
{
    assert_non_null(p_context);
    check_expected_ptr(p_context);
    check_expected(rvbaraddr);
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    assert_non_null(icc_ctx);
    check_expected_ptr(params->payload_buffer);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    assert_non_null(icc_ctx);
    check_expected(params->recv_cmd_code);
    fw_load_cb = params->cb;
    ((kng_hsp_mailbox_msg*)(params->payload_buffer))->header.cmd = mock_type(int);

    return mock_type(fpfw_status_t);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

} // extern "C"

//
// Tests
//
AP_CORE_TEST(init, NULL, NULL)
{
    ap_core_service_t test_device;
    ap_core_service_config_t test_config;

    DFWK_SCHEDULE test_schedule;

    expect_value(__wrap_DfwkDeviceInitialize, Device, &test_device.header);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &test_schedule);
    expect_value(__wrap_DfwkQueueInitialize, Queue, &test_device.default_queue);
    expect_value(__wrap_DfwkQueueInitialize, Device, &test_device.header);
    expect_any(__wrap_DfwkQueueInitialize, DispatchRoutine);
    expect_value(__wrap_DfwkQueueInitialize, DispatchContext, &test_device.header);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_SerializedDispatch);

    // add the expected/check values for power internal functions
    // don't care about the actual values, just that they are passed (wrap function will confirm not null)
    expect_any(__wrap_ap_core_util_get_fuse_enabled_cores, p_enabled_cores);
    expect_any(__wrap_ap_core_ppu_init, p_context);

    expect_not_value(__wrap_FpFwAssert, expression, false);
    expect_not_value(__wrap_FpFwAssert, expression, false);
    expect_not_value(__wrap_FpFwAssert, expression, false);

    ap_core_init(&test_device, &test_schedule, (fpfw_icc_base_ctx_t*)&icc_hspmbx_ctx, &test_config);
}

AP_CORE_TEST(interface_init, NULL, NULL)
{
    ap_core_service_t test_device;
    ap_core_interface_t test_interface;

    expect_value(__wrap_DfwkInterfaceInitialize, Interface, &test_interface.header);
    expect_value(__wrap_DfwkInterfaceInitialize, Device, &test_device.header);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchQueue, &test_device.default_queue);
    expect_any(__wrap_DfwkInterfaceInitialize, DispatchSync);

    expect_not_value(__wrap_FpFwAssert, expression, false);
    expect_not_value(__wrap_FpFwAssert, expression, false);

    ap_core_interface_init(&test_device, &test_interface);
}

AP_CORE_TEST(dispatch_sync_default, NULL, NULL)
{
    DFWK_SYNC_REQUEST_HEADER test_request;
    test_request.RequestType = -1; // invalid request type

    expect_value(__wrap_FpFwAssert, expression, false);

    assert_non_null(s_dispatch_routine_sync);
    s_dispatch_routine_sync(&test_request);
}

AP_CORE_TEST(dispatch_default, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER test_request;
    ap_core_service_t test_device;
    test_request.RequestType = -1; // invalid request type

    expect_value(__wrap_FpFwAssert, expression, false);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request, &test_device.header);
}

AP_CORE_TEST(dispatch_rvbar, NULL, NULL)
{
#define TEST_RVBARADDR 0x12345678

    ap_core_asynchronous_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = APCORE_SET_RVBARADDR_ASYNC;
    test_request.data.rvbaraddr = TEST_RVBARADDR;

    // expect a call to set_all_rvbaraddr
    expect_value(__wrap_ap_core_util_set_all_rvbaraddr, p_context, s_ap_core_ctx);
    expect_value(__wrap_ap_core_util_set_all_rvbaraddr, rvbaraddr, TEST_RVBARADDR);

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_power_on_async, NULL, NULL)
{
    ap_core_asynchronous_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = APCORE_CORE_POWER_ON_ASYNC;
    test_request.data.core_id = 0;

    // expect a call to set_power_state
    expect_value(__wrap_ap_core_ppu_core_set_power_state, p_context, s_ap_core_ctx);
    expect_value(__wrap_ap_core_ppu_core_set_power_state, core_idx, test_request.data.core_id);
    expect_value(__wrap_ap_core_ppu_core_set_power_state, power_state_on, true);

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_power_off_async, NULL, NULL)
{
    ap_core_asynchronous_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = APCORE_CORE_POWER_OFF_ASYNC;
    test_request.data.core_id = 0;

    // expect a call to set_power_state
    expect_value(__wrap_ap_core_ppu_core_set_power_state, p_context, s_ap_core_ctx);
    expect_value(__wrap_ap_core_ppu_core_set_power_state, core_idx, test_request.data.core_id);
    expect_value(__wrap_ap_core_ppu_core_set_power_state, power_state_on, false);

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_ap_core_boot, NULL, NULL)
{
#define TEST_BOOT_CORE 5
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_PRIMARY_AP_CORE_BOOT;

    // expect a call to get boot_core
    expect_value(__wrap_ap_core_util_boot_core, p_context, s_ap_core_ctx);
    will_return(__wrap_ap_core_util_boot_core, TEST_BOOT_CORE);

    // expect a call to set_rvbaraddr
    expect_value(__wrap_ap_core_util_set_rvbaraddr, p_context, s_ap_core_ctx);
    expect_value(__wrap_ap_core_util_set_rvbaraddr, core_idx, TEST_BOOT_CORE);
    assert_non_null(s_ap_core_ctx); // ensure the context is set by previous test
    expect_value(__wrap_ap_core_util_set_rvbaraddr, rvbaraddr, s_ap_core_ctx->p_config->boot_core_rvbaraddr);

    // expect call to turn core on
    expect_value(__wrap_ap_core_ppu_core_set_power_state, p_context, s_ap_core_ctx);
    expect_value(__wrap_ap_core_ppu_core_set_power_state, core_idx, TEST_BOOT_CORE);
    expect_value(__wrap_ap_core_ppu_core_set_power_state, power_state_on, true);

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_cluster_core_init, NULL, NULL)
{
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_CLUSTER_CORE_INIT;

    // expect a call to turn on cluster PPUs
    expect_value(__wrap_ap_core_ppu_clusters_on, p_context, s_ap_core_ctx);

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_start_default, NULL, NULL)
{
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_PHASE_MSCP_ASYNC; // unhandled stage

    // only expect the request to be completed
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_complete_default, NULL, NULL)
{
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_COMPLETE_ASYNC;
    test_request.stage = STARTUP_PHASE_MSCP_ASYNC; // unhandled stage

    // only expect the request to be completed
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_shutdown, NULL, NULL)
{
#define TEST_BOOT_CORE 5
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_SHUTDOWN_QUIESCE_ASYNC;

    // nothing done today, just check that the request is completed
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_bl31_load, NULL, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_BL31_LOAD; // unhandled stage

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HspFirmwareIdPeManagementMode,
        .address = 0x00000000,
        .size = 0x00000000,
    };
    expect_memory(__wrap_fpfw_icc_base_send, params->payload_buffer, &send_request, sizeof(send_request));
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Call API under test
    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);

    // Call the callback to simulate the response
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);
    fw_load_cb(NULL, 0, FPFW_STATUS_SUCCESS);
}