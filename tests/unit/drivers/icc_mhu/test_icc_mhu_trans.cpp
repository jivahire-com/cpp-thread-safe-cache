//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_icc_mhu_trans.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include "icc_mhu_trans_ut.h" // local defines

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <icc_mhu_trans_prim.h> // for icc mhu
#include <icc_mhu_trans_prim_i.h>
#include <kng_scmi_shared.h>
#include <stddef.h> // for NULL
#include <stdint.h> // for uint32_t
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern "C" {
int __real_icc_mhu_trans_get_message(uint16_t mhu_interface_id, picc_msg_receive_t message);
}

/*-- Declarations (Statics and globals) --*/

icc_mhu_properties_t properties;
uint8_t mailbox_0[SIZE_OF_MAILBOX_TEST];
uint8_t mailbox_1[SIZE_OF_MAILBOX_TEST];

typedef struct
{
    uint32_t command;
    uint16_t size;
    uint8_t data[SIZE_OF_MSG_BUFFER];
} transport_message_t;

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    icc_mhu_trans_get_config_entries(&properties);
    icc_mhu_configuration_t* configuration = (icc_mhu_configuration_t*)properties.pIcc_configuration_table;
    configuration[0].mailbox_address = (uintptr_t)mailbox_0;
    configuration[1].mailbox_address = (uintptr_t)mailbox_1;
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
TEST_FUNCTION(test_icc_mhu_trans_init, test_setup, test_teardown)
{
    // Ensure that the MHU is initialized
    int status = icc_mhu_trans_init();
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_check_scmi_status_bit, test_setup, test_teardown)
{
    icc_mhu_trans_check_scmi_status_bit(ICC_MSG_INDEX_SEND);
    icc_mhu_trans_check_scmi_status_bit(ICC_MSG_INDEX_RECEIVE);

    // Check if bits are set, note that mailbox_1 is used for received
    picc_mhu_packet_t packet = (picc_mhu_packet_t)mailbox_1;
    scmi_icc_packet_t* scmi_packet = (scmi_icc_packet_t*)packet->payload;
    assert_int_equal(scmi_packet->smt_header.status, 1);
}

TEST_FUNCTION(test_icc_mhu_trans_enable_interrupts, test_setup, test_teardown)
{
    // Simple interrupt test, may need to expand further
    int status = icc_mhu_trans_enable_interrupts();
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_set_configuration_table, test_setup, test_teardown)
{
    // Ensure that the MHU is initialized
    int status = icc_mhu_trans_set_configuration_table(NULL, properties.number_of_channel_cfg);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);
    status = icc_mhu_trans_set_configuration_table((icc_mhu_configuration_t*)properties.pIcc_configuration_table, 0);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);

    status = icc_mhu_trans_set_configuration_table((icc_mhu_configuration_t*)properties.pIcc_configuration_table,
                                                   properties.number_of_channel_cfg);
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_send_message_idx, test_setup, test_teardown)
{
    // data initialized, dummy data
    uint8_t data[] = {0, 1, 2};
    will_return(__wrap_icc_mhu_send_message, ICC_MHU_STATUS_SUCCESS);
    int status = icc_mhu_trans_send_message_idx(ICC_MSG_INDEX_SEND, WRAPPER_ICC_COMMAND, data, sizeof(data));
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);

    will_return(__wrap_icc_mhu_send_message, ICC_MHU_INVALID_PARAMETER);
    status = icc_mhu_trans_send_message_idx(ICC_MSG_INDEX_SEND, WRAPPER_ICC_COMMAND, data, sizeof(data));
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);

    will_return(__wrap_icc_mhu_send_message, ICC_MHU_INTERFACE_BUSY);
    status = icc_mhu_trans_send_message_idx(ICC_MSG_INDEX_SEND, WRAPPER_ICC_COMMAND, data, sizeof(data));
    assert_int_equal(status, ICC_MHU_INTERFACE_BUSY);
}

TEST_FUNCTION(test_icc_mhu_trans_get_message, test_setup, test_teardown)
{

    icc_msg_receive_t message;
    will_return(__wrap_icc_mhu_get_message, ICC_MHU_STATUS_SUCCESS);
    int status = __real_icc_mhu_trans_get_message(ICC_MSG_SCMI_RECEIVE_ID, &message);
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);

    will_return(__wrap_icc_mhu_get_message, ICC_MHU_INVALID_PARAMETER);
    status = __real_icc_mhu_trans_get_message(ICC_MSG_SCMI_RECEIVE_ID, &message);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);
}

TEST_FUNCTION(test_icc_mhu_trans_get_msg_from_index, test_setup, test_teardown)
{
    // create an example command and buffer
    transport_message_t message;
    message.command = WRAPPER_ICC_COMMAND;
    message.size = sizeof(message.data);

    will_return(__wrap_icc_mhu_trans_get_message, ICC_MHU_STATUS_SUCCESS);
    int status = icc_mhu_trans_get_msg_from_index(ICC_MSG_INDEX_RECEIVE, (icc_mhu_transport_message_t*)&message);
    assert_int_equal(status, ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE);

    message.command = WRAPPER_ICC_COMMAND_FAIL;
    will_return(__wrap_icc_mhu_trans_get_message, ICC_MHU_STATUS_SUCCESS);
    status = icc_mhu_trans_get_msg_from_index(ICC_MSG_INDEX_RECEIVE, (icc_mhu_transport_message_t*)&message);
    assert_int_equal(status, ICC_MHU_TRANS_NO_CMD_MESSAGE_AVAILABLE);

    will_return(__wrap_icc_mhu_trans_get_message, ICC_MHU_INVALID_PARAMETER);
    status = icc_mhu_trans_get_msg_from_index(ICC_MSG_INDEX_RECEIVE, (icc_mhu_transport_message_t*)&message);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);
}