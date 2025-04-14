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
#include <accelip_id.h>
#include <ap_core.h>
#include <ap_core_i.h>
#include <ap_core_init.h>
#include <ap_fw_info.h>
#include <corebits.h>
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#define __NO_CSR_TYPEDEFS__
#include <mcp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <mcp_exp_top_regs.h>
#include <mscp_exp_rmss_memory_map.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <sds_api.h>
#include <sds_configuration.h>
#include <sds_init.h>
#include <shared_sds_def.h>
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
static void* cb_ctx = NULL;

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

void __wrap_ap_core_ppu_clusters_off(ap_core_service_context_t* p_context, uint32_t timeout_ms)
{
    FPFW_UNUSED(p_context);
    FPFW_UNUSED(timeout_ms);
    function_called();
}

void __wrap_ap_core_ppu_cores_off(ap_core_service_context_t* p_context, uint32_t timeout_ms)
{
    FPFW_UNUSED(p_context);
    FPFW_UNUSED(timeout_ms);
    function_called();
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
    cb_ctx = params->cb_ctx;
    ((kng_hsp_mailbox_msg*)(params->payload_buffer))->header.cmd = mock_type(int);

    return mock_type(fpfw_status_t);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

int __wrap_write_fuse_info_to_ap()
{
    return mock_type(int);
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected(addr);
    return (mock_type(uint32_t));
}

int32_t __wrap_sds_block_write(uint32_t sds_module_id, void* buffer, size_t buffer_size)
{
    check_expected(sds_module_id);
    check_expected_ptr(buffer);
    check_expected(buffer_size);

    return 0;
}

int32_t __wrap_sds_block_creation(uint32_t sds_module_id, uint32_t request_size, uint32_t region_id)
{
    FPFW_UNUSED(sds_module_id);
    FPFW_UNUSED(request_size);
    FPFW_UNUSED(region_id);

    return 0;
}

void __wrap_accel_get_itcm_addr(ACCEL_ID accel_type, uint32_t* low_addr, uint32_t* high_addr)
{
    FPFW_UNUSED(low_addr);
    FPFW_UNUSED(high_addr);

    check_expected(accel_type);
    function_called();
}
void __wrap_accel_get_dtcm_addr(ACCEL_ID accel_type, uint32_t* low_addr, uint32_t* high_addr)
{
    FPFW_UNUSED(low_addr);
    FPFW_UNUSED(high_addr);

    check_expected(accel_type);
    function_called();
}
void __wrap_accel_disable_cpu_wait(ACCEL_ID accel_type)
{
    check_expected(accel_type);
    function_called();
}

uint32_t __wrap_atu_svc_accel_atu_addr(ACCEL_ID accel_id)
{
    FPFW_UNUSED(accel_id);

    return mock_type(uint32_t);
}

uint32_t __wrap_idsw_get_platform_sdv()
{
    return mock_type(uint32_t);
}
uint8_t __wrap_idsw_get_die_id()
{
    return mock_type(uint8_t);
}
uint8_t __wrap_idhw_get_soc_id()
{
    return mock_type(uint8_t);
}

int32_t __wrap__txe_event_flags_set(TX_EVENT_FLAGS_GROUP* event_flags_group_ptr, ULONG flags, UINT options)
{
    FPFW_UNUSED(event_flags_group_ptr);
    FPFW_UNUSED(flags);
    FPFW_UNUSED(options);
    return mock_type(int32_t);
}

} // extern "C"

static int setup(void** state)
{
    FPFW_UNUSED(state);

    const corebits_t default_cores = (const corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFF, 0xFFFFFFFF, 0xF);

    ap_core_service_t test_device;
    static ap_core_service_config_t test_config = {
        .platform_cores_in_die = &default_cores,
        .primary_boot_die = true,
    };

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
    return 0;
}

//
// Tests
//
AP_CORE_TEST(init, setup, NULL)
{
    // nothing to do, setup stage will validate init function works
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

AP_CORE_TEST(dispatch_rvbar, setup, NULL)
{
#define TEST_RVBARADDR 0x12345678

    ap_core_asynchronous_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = APCORE_SET_RVBARADDR_ASYNC;
    test_request.data.rvbaraddr = TEST_RVBARADDR;

    // expect a call to set_all_rvbaraddr
    expect_any(__wrap_ap_core_util_set_all_rvbaraddr, p_context);
    expect_value(__wrap_ap_core_util_set_all_rvbaraddr, rvbaraddr, TEST_RVBARADDR);

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_power_on_async, setup, NULL)
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

AP_CORE_TEST(dispatch_power_off_async, setup, NULL)
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

AP_CORE_TEST(dispatch_ap_core_boot, setup, NULL)
{
#define TEST_BOOT_CORE 5
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_PRIMARY_AP_CORE_BOOT;
    test_request.boot_type = COLD_BOOT;

    // expect a call to get boot_core
    expect_value(__wrap_ap_core_util_boot_core, p_context, s_ap_core_ctx);
    will_return(__wrap_ap_core_util_boot_core, TEST_BOOT_CORE);
    will_return(__wrap_system_info_is_hsp_present, true);

    // expect that die info is stored in SDS
    shared_scp_exp_csr_die_config test_die_config = {.as_uint32 = 42};
    expect_value(__wrap_mmio_read32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_DIE_CONFIG_ADDRESS);
    will_return(__wrap_mmio_read32, test_die_config.as_uint32);
    expect_value(__wrap_sds_block_write, sds_module_id, SDS_DIE_CONFIG_STRUCT_ID);
    expect_memory(__wrap_sds_block_write, buffer, &test_die_config, sizeof(test_die_config));
    expect_value(__wrap_sds_block_write, buffer_size, sizeof(test_die_config));
    will_return(__wrap_write_fuse_info_to_ap, 0);

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

AP_CORE_TEST(dispatch_cluster_core_init, setup, NULL)
{
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_CLUSTER_CORE_INIT;
    test_request.boot_type = COLD_BOOT;

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

AP_CORE_TEST(dispatch_shutdown, setup, NULL)
{
    ssi_shutdown_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_SHUTDOWN_QUIESCE_ASYNC;

    assert_non_null(s_dispatch_routine);

    for (int idx = SHUTDOWN; idx <= AP_WARM_RESET; idx++)
    {
        if (idx != MSCP_SUBSYS_RESET)
        {
            expect_function_call(__wrap_ap_core_ppu_cores_off);
            expect_function_call(__wrap_ap_core_ppu_clusters_off);
        }
        expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

        test_request.shutdown_type = (ssi_shutdown_type_t)idx;
        s_dispatch_routine(&test_request.header, &test_device.header);
    }
}

AP_CORE_TEST(dispatch_bl31_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_BL31_LOAD; // unhandled stage
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_PE_SECURITY_MONITOR,
        .address = TFA_FW_LOAD_ADDRESS,
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

AP_CORE_TEST(dispatch_pcie_phyfw_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_PCIE_PHY_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_PCIE_PHY,
        .address = SCP_EXP_PCIE_PHY_FW_BASE,
        .size = 0x00000000,
    };
    expect_memory(__wrap_fpfw_icc_base_send, params->payload_buffer, &send_request, sizeof(send_request));
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Call API under test
    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);

    // Call the callback to simulate the response
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);
    fw_load_cb(NULL, 0, FPFW_STATUS_SUCCESS);
}

AP_CORE_TEST(dispatch_pcie_phyfw_load_hsp_not_present, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_PCIE_PHY_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    // In ap_core_dispatch()
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any(__wrap_DfwkAsyncRequestComplete, Request);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_mcp_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_MCP_LOAD; // unhandled stage

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_MCP,
        .address = MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS,
        .size = 0x00000000,
    };
    expect_memory(__wrap_fpfw_icc_base_send, params->payload_buffer, &send_request, sizeof(send_request));
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Call API under test
    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);

    struct kng_hsp_mailbox_cmd_start_core_req mcp_start_req = {
        .header.cmd = HSP_MAILBOX_CMD_START_CORE_REQ,
        .id = HSP_FIRMWARE_ID_MCP,
        .address = MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS + HSP_BOOT_RAM_META_DATA_SIZE};

    // MCP reset release cb also sends a mailbox message
    expect_memory(__wrap_fpfw_icc_base_send, params->payload_buffer, &mcp_start_req, sizeof(mcp_start_req));
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Call the callback to simulate the response
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);
    fw_load_cb(&icc_hspmbx_ctx, 0, FPFW_STATUS_SUCCESS);
}

AP_CORE_TEST(dispatch_sdm_itcm_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_SDM_ITCM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    // In ap_core_dispatch()
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_die_id, 0x0);

    // In ap_core_request_load_ap_fw()
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_accel_get_itcm_addr, accel_type, ACCEL_ID_SDM);
    expect_function_call(__wrap_accel_get_itcm_addr);
    expect_any(__wrap_fpfw_icc_base_send, params->payload_buffer);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Call API under test
    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);

    // Call the callback to simulate the response
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);
    fw_load_cb(cb_ctx, 0, FPFW_STATUS_SUCCESS);
}

AP_CORE_TEST(dispatch_sdm_itcm_load_hsp_not_present, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_SDM_ITCM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    // In ap_core_dispatch()
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any(__wrap_DfwkAsyncRequestComplete, Request);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_sdm_dtcm_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_SDM_DTCM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    // In ap_core_dispatch()
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return(__wrap_idsw_get_die_id, 0x0);

    // In ap_core_request_load_ap_fw()
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_accel_get_dtcm_addr, accel_type, ACCEL_ID_SDM);
    expect_function_call(__wrap_accel_get_dtcm_addr);
    expect_any(__wrap_fpfw_icc_base_send, params->payload_buffer);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Call API under test
    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);

    // Call the callback to simulate the response
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0);
    will_return_always(__wrap_idsw_get_platform_sdv, 0x41);
    will_return_always(__wrap_idsw_get_die_id, 0x0);
    will_return_always(__wrap_idhw_get_soc_id, 0x0);

    expect_value(__wrap_accel_disable_cpu_wait, accel_type, ACCEL_ID_SDM);
    expect_function_call(__wrap_accel_disable_cpu_wait);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);
    fw_load_cb(cb_ctx, 0, FPFW_STATUS_SUCCESS);
}

AP_CORE_TEST(dispatch_sdm_dtcm_load_hsp_not_present, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_SDM_DTCM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    // In ap_core_dispatch()
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any(__wrap_DfwkAsyncRequestComplete, Request);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_cded_itcm_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_CDED_ITCM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    // In ap_core_dispatch()
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_die_id, 0x0);

    // In ap_core_request_load_ap_fw()
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_accel_get_itcm_addr, accel_type, ACCEL_ID_CDED);
    expect_function_call(__wrap_accel_get_itcm_addr);
    expect_any(__wrap_fpfw_icc_base_send, params->payload_buffer);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Call API under test
    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);

    // Call the callback to simulate the response
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);
    fw_load_cb(cb_ctx, 0, FPFW_STATUS_SUCCESS);
}

AP_CORE_TEST(dispatch_cded_itcm_load_hsp_not_present, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_CDED_ITCM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    // In ap_core_dispatch()
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any(__wrap_DfwkAsyncRequestComplete, Request);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_cded_dtcm_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_CDED_DTCM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    // In ap_core_dispatch()
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return(__wrap_idsw_get_die_id, 0x0);

    // In ap_core_request_load_ap_fw()
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_accel_get_dtcm_addr, accel_type, ACCEL_ID_CDED);
    expect_function_call(__wrap_accel_get_dtcm_addr);
    expect_any(__wrap_fpfw_icc_base_send, params->payload_buffer);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Call API under test
    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);

    // Call the callback to simulate the response
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0);
    will_return_always(__wrap_idsw_get_platform_sdv, 0x41);
    will_return_always(__wrap_idsw_get_die_id, 0x0);
    will_return_always(__wrap_idhw_get_soc_id, 0x0);

    expect_value(__wrap_accel_disable_cpu_wait, accel_type, ACCEL_ID_CDED);
    expect_function_call(__wrap_accel_disable_cpu_wait);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);
    fw_load_cb(cb_ctx, 0, FPFW_STATUS_SUCCESS);
}

AP_CORE_TEST(dispatch_cded_dtcm_load_hsp_not_present, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_CDED_DTCM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    // In ap_core_dispatch()
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any(__wrap_DfwkAsyncRequestComplete, Request);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_kmp_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_KMP_LOAD; // unhandled stage
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_die_id, 0x0);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

#define KMP_LOAD_ADDRESS 0XFFFFFF0480000
    kng_hsp_mailbox_cmd_load_fw_64bit_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_KMP,
        .load_addr_low = KMP_LOAD_ADDRESS & (uint32_t)0xFFFFFFFF,
        .load_addr_high = KMP_LOAD_ADDRESS >> 32,
    };
    expect_memory(__wrap_fpfw_icc_base_send, params->payload_buffer, &send_request, sizeof(send_request));
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_request.header);

    // Call API under test
    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);

#define KMP_START_ADDRESS 0x00080080
    struct kng_hsp_mailbox_cmd_start_core_req kmp_start_req = {.header.cmd = HSP_MAILBOX_CMD_START_CORE_REQ,
                                                               .id = HSP_FIRMWARE_ID_KMP,
                                                               .address = KMP_START_ADDRESS};

    // KMP reset release cb also sends a mailbox message
    expect_memory(__wrap_fpfw_icc_base_send, params->payload_buffer, &kmp_start_req, sizeof(kmp_start_req));
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Call the callback to simulate the response
    fw_load_cb(&icc_hspmbx_ctx, 0, FPFW_STATUS_SUCCESS);
}

AP_CORE_TEST(dispatch_kmp_hsp_not_present, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_KMP_LOAD; // unhandled stage
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any(__wrap_DfwkAsyncRequestComplete, Request);

    // Call API under test
    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request.header, &test_device.header);
}

AP_CORE_TEST(dispatch_stmm_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_STMM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_PE_MANAGEMENT_MODE,
        .address = STMM_FW_LOAD_ADDRESS,
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

AP_CORE_TEST(dispatch_bl33_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_BL33_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_PE_UEFI,
        .address = BL33_FW_LOAD_ADDRESS,
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

AP_CORE_TEST(dispatch_hafnium_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_HAFNIUM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_PE_SPM,
        .address = HAFNIUM_FW_LOAD_ADDRESS,
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

AP_CORE_TEST(dispatch_rmm_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_RMM_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_64bit_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_RMM,
        .load_addr_low = REALM_FW_LOAD_ADDRESS & (uint32_t)0xFFFFFFFF,
        .load_addr_high = REALM_FW_LOAD_ADDRESS >> 32,
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

AP_CORE_TEST(dispatch_rp_exe_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_RP_EXE_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_RP_EXE,
        .address = RP_EXE_LOAD_ADDRESS,
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

AP_CORE_TEST(dispatch_rp_data_load, setup, NULL)
{
    // Set up pre-conditions
    ssi_startup_notification_request_t test_request;
    ap_core_service_t test_device;
    test_request.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    test_request.stage = STARTUP_RP_DATA_LOAD;
    test_request.boot_type = COLD_BOOT;

    // Set up expectations
    will_return(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_RP_DATA,
        .address = RP_DATA_LOAD_ADDRESS,
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
