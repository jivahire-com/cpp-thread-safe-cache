//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_scmi_drv.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:scmi_drv

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <ap_core.h>
#include <icc_mhu.h> // for icc mhu
#include <icc_mhu_cfg.h>
#include <kng_scmi_shared.h> // for SCMI Data structures and functions
#include <mhu_icc_transport.h>
#include <scmi_init.h>
#include <scmi_prim.h>
#include <scmi_prim_i.h>
#include <startup_shutdown.h>
#include <startup_shutdown_init.h>
#include <stddef.h> // for NULL
#include <stdint.h> // for uint32_t
#include <system_info.h>
}

/*-- Symbolic Constant Macros (defines) --*/

#define PROTOCOL_VERSION 0x2000

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern "C" {
int __real_scmi_send_resp(uint8_t protocol_id, uint8_t command, uint8_t* payload, size_t size);
int __real_scmi_check_message(scmi_icc_packet_t* packet);
int __real_scmi_power_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size);
int __real_scmi_sys_pwr_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size);
int __real_scmi_ap_core_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size);
void __real_scmi_cold_boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context);
void __real_scmi_shutdown_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context);
void __real_scmi_warm_boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context);
void __real_ap_core_power_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context);
void __real_ap_core_reset_addr_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context);
}

/*-- Declarations (Statics and globals) --*/

static uint8_t s_test_mscp2_tfa_recv_data[256];
static uint8_t s_test_mscp2_tfa_send_data[256];

static mhu_icc_transport_device_t s_test_mscp2_tfa_dev = {
    .recv_channel =
        {
            .ch_shared_mem_addr = (uintptr_t)s_test_mscp2_tfa_recv_data,
            .ch_shared_mem_size = sizeof(s_test_mscp2_tfa_recv_data),
        },
    .send_channel =
        {
            .ch_shared_mem_addr = (uintptr_t)s_test_mscp2_tfa_send_data,
            .ch_shared_mem_size = sizeof(s_test_mscp2_tfa_send_data),
        },
};
static mhu_icc_transport_intrf_t s_test_mscp2_tfa_if = {
    .device = &s_test_mscp2_tfa_dev,
};

typedef struct
{
    smt_header_t smt_header;
    scmi_message_header_t header;
    uint32_t payload[12];
} test_scmi_local_packet_t;

typedef struct
{
    icc_mhu_header_t header;
    uint8_t data[64];
} test_scmi_transport_message_t;

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    // Just invoke making sure nothing breaking
    scmi_set_debug_mode(15);
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST));
    will_return(__wrap_system_info_is_warm_start, false);
    will_return(__wrap_fpfw_icc_transport_recv_async_req, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    scmi_drv_init((DFWK_INTERFACE_HEADER*)&s_test_mscp2_tfa_if);
    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_scmi_drv_init, nullptr, test_teardown)
{
    test_scmi_transport_message_t* recv_message = (test_scmi_transport_message_t*)s_test_mscp2_tfa_recv_data;
    test_scmi_local_packet_t* local_packet = (test_scmi_local_packet_t*)recv_message->data;

    local_packet->smt_header.status = 0;
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST));
    will_return(__wrap_system_info_is_warm_start, false);
    will_return(__wrap_fpfw_icc_transport_recv_async_req, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    scmi_drv_init((DFWK_INTERFACE_HEADER*)&s_test_mscp2_tfa_if);
    assert_int_equal(local_packet->smt_header.status, 1U);

    local_packet->smt_header.status = 0xA5A5A5A5U;
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST));
    will_return(__wrap_system_info_is_warm_start, true);
    will_return(__wrap_fpfw_icc_transport_recv_async_req, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    scmi_drv_init((DFWK_INTERFACE_HEADER*)&s_test_mscp2_tfa_if);
    assert_int_equal(local_packet->smt_header.status, 0xA5A5A5A5U);
}

TEST_FUNCTION(test_scmi_set_apcore_interface, test_setup, test_teardown)
{
#define APCORE_INTERFACE 0x12345678
    expect_value(__wrap_DfwkClientInterfaceOpen, Interface, APCORE_INTERFACE);
    will_return(__wrap_DfwkClientInterfaceOpen, 0);
    scmi_set_apcore_interface((DFWK_INTERFACE_HEADER*)APCORE_INTERFACE);
}

TEST_FUNCTION(test_scmi_send_resp, test_setup, test_teardown)
{
    scmi_protocol_version_resp_t protocol_ver_resp;
    protocol_ver_resp.status = 0;
    protocol_ver_resp.status = PROTOCOL_VERSION;
    will_return(__wrap_fpfw_icc_transport_try_send_sync_req, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    int status =
        __real_scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTO_VERSION_MSG, (uint8_t*)&protocol_ver_resp, sizeof(protocol_ver_resp));
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_scmi_check_message, test_setup, test_teardown)
{
    scmi_icc_packet_t packet;
    packet.smt_header.payload_size = sizeof(packet.header);
    packet.header.cmd_code = SCMI_PROTOCOL_VERSION;

    will_return(__wrap_scmi_power_protocol_cmds, SCMI_PROTOCOL_CMD_SUCCESS);
    packet.header.protocol_id = SCMI_PWR_DMN_PROTO_ID;
    int status = __real_scmi_check_message(&packet);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    will_return(__wrap_scmi_sys_pwr_protocol_cmds, SCMI_PROTOCOL_CMD_SUCCESS);
    packet.header.protocol_id = SCMI_SYS_PWR_PROTO_ID;
    status = __real_scmi_check_message(&packet);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    will_return(__wrap_scmi_ap_core_protocol_cmds, SCMI_PROTOCOL_CMD_SUCCESS);
    packet.header.protocol_id = SCMI_AP_CORE_PROTO_ID;
    status = __real_scmi_check_message(&packet);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    packet.header.protocol_id = 0xFF;
    status = __real_scmi_check_message(&packet);
    assert_int_equal(status, SCMI_PROTOCOL_NOT_SUPPORTED);
}

TEST_FUNCTION(test_ap_core_power_completion, test_setup, test_teardown)
{
    _DFWK_ASYNC_REQUEST_HEADER req = {0};

    // APCORE_CORE_POWER_ON_ASYNC
    req.RequestType = 1;
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    __real_ap_core_power_completion(&req, NULL);

    // APCORE_CORE_POWER_OFF_ASYNC
    req.RequestType = 2;
    __real_ap_core_power_completion(&req, NULL);
}

TEST_FUNCTION(test_ap_core_reset_addr_completion, test_setup, test_teardown)
{
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    __real_ap_core_reset_addr_completion(NULL, NULL);
}

TEST_FUNCTION(test_scmi_ap_core_protocol_cmds, test_setup, test_teardown)
{
#define RVBARLO 0x12345678
#define RVBARHI 0x87654321

    // AP Core Command Tests
    uint8_t data[] = {1, 2, 3, 4};
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    int status = __real_scmi_ap_core_protocol_cmds(SCMI_PROTOCOL_VERSION, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // test set rvbaraddr handling
    scmi_apcore_reset_address_set_a2p_t reset_addr_set = {RVBARLO, RVBARHI, 0};
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(ap_core_asynchronous_request_t));
    expect_value(__wrap_ap_core_set_rvbaraddr, rvbaraddr, ((uint64_t)RVBARHI << 32 | RVBARLO));
    status = __real_scmi_ap_core_protocol_cmds(SCMI_AP_CORE_RESET_ADDR_SET_MSG, (uint8_t*)&reset_addr_set, sizeof(reset_addr_set));
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    status = __real_scmi_ap_core_protocol_cmds(SCMI_AP_CORE_RESET_ADDR_GET_MSG, data, 2);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    status = __real_scmi_ap_core_protocol_cmds(0xFF, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_UKNOWN);
}

TEST_FUNCTION(test_scmi_sys_pwr_protocol_cmds, test_setup, test_teardown)
{
    // Sys Power Control Commands
    uint8_t data[] = {1, 2, 3, 4};
    scmi_sys_pwr_set_state_a2p_t sys_power_set = {0};

    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    int status = __real_scmi_sys_pwr_protocol_cmds(SCMI_PROTOCOL_VERSION, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // Sys Power State Set Invalid flags
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    sys_power_set.flags = 0xFF;
    sys_power_set.system_state = SCMI_SYS_PWR_SYS_STATE_SHUTDOWN;
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_SET_MSG, (uint8_t*)&sys_power_set, sizeof(sys_power_set));
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // Sys Power State Set Shutdown
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_shutdown_request_t));
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    sys_power_set.flags = 0;
    sys_power_set.system_state = SCMI_SYS_PWR_SYS_STATE_SHUTDOWN;
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_SET_MSG, (uint8_t*)&sys_power_set, sizeof(sys_power_set));
    // callback function
    __real_scmi_shutdown_completion(NULL, NULL);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // Sys Power State Set Cold Reset
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_shutdown_request_t));
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    sys_power_set.flags = 0;
    sys_power_set.system_state = SCMI_SYS_PWR_SYS_STATE_COLD_RESET;
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_SET_MSG, (uint8_t*)&sys_power_set, sizeof(sys_power_set));
    // callback function
    __real_scmi_cold_boot_completion(NULL, NULL);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // Sys Power State Set Warm Reset
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_shutdown_request_t));
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    sys_power_set.flags = 0;
    sys_power_set.system_state = SCMI_SYS_PWR_SYS_STATE_WARM_RESET;
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_SET_MSG, (uint8_t*)&sys_power_set, sizeof(sys_power_set));
    // callback function
    __real_scmi_warm_boot_completion(NULL, NULL);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // Sys Power State Set Force Power Up
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_shutdown_request_t));
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    sys_power_set.flags = 0;
    sys_power_set.system_state = SCMI_SYS_PWR_SYS_STATE_POWER_UP;
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_SET_MSG, (uint8_t*)&sys_power_set, sizeof(sys_power_set));
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // Sys Power State Set Ignore Power Up
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_shutdown_request_t));
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    sys_power_set.flags = 1;
    sys_power_set.system_state = SCMI_SYS_PWR_SYS_STATE_POWER_UP;
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_SET_MSG, (uint8_t*)&sys_power_set, sizeof(sys_power_set));
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // Sys Power State Set Suspend
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_shutdown_request_t));
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    sys_power_set.flags = 0;
    sys_power_set.system_state = SCMI_SYS_PWR_SYS_STATE_SUSPEND;
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_SET_MSG, (uint8_t*)&sys_power_set, sizeof(sys_power_set));
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // Sys Power State Set Invalid system_state
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_shutdown_request_t));
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    sys_power_set.flags = 0;
    sys_power_set.system_state = 0xFF;
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_SET_MSG, (uint8_t*)&sys_power_set, sizeof(sys_power_set));
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_GET_MSG, data, 2);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    status = __real_scmi_sys_pwr_protocol_cmds(0xFF, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_UKNOWN);
}

TEST_FUNCTION(test_scmi_power_protocol_cmds, test_setup, test_teardown)
{
#define TEST_CORE_NUM 5

    // Power Domain Commands
    uint8_t data[] = {1, 2, 3, 4};
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    int status = __real_scmi_power_protocol_cmds(SCMI_PROTOCOL_VERSION, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // test core power on
    scmi_pd_power_state_set_a2p_t power_set = {0, TEST_CORE_NUM, SCMI_PD_CORE_STATE_ON};
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(ap_core_asynchronous_request_t));
    expect_value(__wrap_ap_core_core_power_on, core_id, TEST_CORE_NUM);
    status = __real_scmi_power_protocol_cmds(SCMI_PWR_STATE_SET_MSG, (uint8_t*)&power_set, sizeof(power_set));
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    // test core power off
    power_set.power_state = SCMI_PD_CORE_STATE_OFF;
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(ap_core_asynchronous_request_t));
    expect_value(__wrap_ap_core_core_power_off, core_id, TEST_CORE_NUM);
    status = __real_scmi_power_protocol_cmds(SCMI_PWR_STATE_SET_MSG, (uint8_t*)&power_set, sizeof(power_set));
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    status = __real_scmi_power_protocol_cmds(SCMI_PWR_STATE_GET_MSG, data, 2);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    status = __real_scmi_power_protocol_cmds(0xFF, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_UKNOWN);
}