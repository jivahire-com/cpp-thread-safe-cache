//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_scmi_drv.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <icc_mhu.h>   // for icc mhu
#include <icc_mhu_cfg.h>
#include <icc_mhu_trans_prim.h>
#include <kng_scmi_shared.h> // for SCMI Data structures and functions
#include <scmi_prim.h>
#include <scmi_prim_i.h>
#include <stddef.h> // for NULL
#include <stdint.h> // for uint32_t
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
}

/*-- Declarations (Statics and globals) --*/

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    // Just invoke making sure nothing breaking
    scmi_set_debug_mode(15);
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
TEST_FUNCTION(test_scmi_send_resp, test_setup, test_teardown)
{
    scmi_protocol_version_resp_t protocol_ver_resp;
    protocol_ver_resp.status = 0;
    protocol_ver_resp.status = PROTOCOL_VERSION;
    will_return(__wrap_icc_mhu_trans_send_message_idx, ICC_MHU_STATUS_SUCCESS);
    int status =
        __real_scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTO_VERSION_MSG, (uint8_t*)&protocol_ver_resp, sizeof(protocol_ver_resp));
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);
}

TEST_FUNCTION(test_scmi_poll_message, test_setup, test_teardown)
{

    will_return(__wrap_icc_mhu_trans_get_msg_from_index, ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE);
    will_return(__wrap_scmi_check_message, SCMI_PROTOCOL_CMD_SUCCESS);
    int status = scmi_poll_message();
    assert_int_equal(status, ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE);
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

TEST_FUNCTION(test_scmi_ap_core_protocol_cmds, test_setup, test_teardown)
{
    // AP Core Command Tests
    uint8_t data[] = {1, 2, 3, 4};
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    int status = __real_scmi_ap_core_protocol_cmds(SCMI_PROTOCOL_VERSION, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    status = __real_scmi_ap_core_protocol_cmds(SCMI_AP_CORE_RESET_ADDR_SET_MSG, data, 2);
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
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    int status = __real_scmi_sys_pwr_protocol_cmds(SCMI_PROTOCOL_VERSION, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_SET_MSG, data, 2);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    status = __real_scmi_sys_pwr_protocol_cmds(SCMI_SYS_PWR_STATE_GET_MSG, data, 2);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    status = __real_scmi_sys_pwr_protocol_cmds(0xFF, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_UKNOWN);
}

TEST_FUNCTION(test_scmi_power_protocol_cmds, test_setup, test_teardown)
{
    // Power Domain Commands
    uint8_t data[] = {1, 2, 3, 4};
    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    int status = __real_scmi_power_protocol_cmds(SCMI_PROTOCOL_VERSION, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    status = __real_scmi_power_protocol_cmds(SCMI_PWR_STATE_SET_MSG, data, 2);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    will_return(__wrap_scmi_send_resp, ICC_MHU_STATUS_SUCCESS);
    status = __real_scmi_power_protocol_cmds(SCMI_PWR_STATE_GET_MSG, data, 2);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_SUCCESS);

    status = __real_scmi_power_protocol_cmds(0xFF, data, 0);
    assert_int_equal(status, SCMI_PROTOCOL_CMD_UKNOWN);
}