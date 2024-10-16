//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_icc_mhu_trans.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:icc_mhu

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include "icc_mhu_trans_ut.h" // local defines

#include <FpFwUtils.h>                    // for FPFW_UNUSED
#include <fpfw_icc_transport_interface.h> // Leverage the transport library interrface
#include <fpfw_status.h>                  // for fpfw_status_t
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <icc_mhu_trans_dfwk.h>   // for icc mhu
#include <icc_mhu_trans_dfwk_i.h> // for icc mhu
#include <icc_mhu_trans_prim.h>   // for icc mhu
#include <icc_mhu_trans_prim_i.h>
#include <kng_icc_shared.h>
#include <kng_scmi_shared.h>
#include <kng_scp_tfa_shared.h>
#include <mhuv3.h>
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
DFWK_SCHEDULE Schedule;
icc_mhu_transport_device_t icc_mhu_trans_dev = {};
icc_mhu_transport_intrf_t icc_mhu_inf;
icc_mhu_transport_config_t config = {
    .send_core_2_core_id = MHU_INTERFACE_ID(SCP_LOCAL, AP_CORE_SEC),
    .receive_core_2_core_id = MHU_INTERFACE_ID(AP_CORE_SEC, SCP_LOCAL),
};

// Test Configuration table used for primitives initialization
static icc_mhu_configuration_t icc_mhu_configuration[] = {
    // SCP TO AP S Send Response -(TFA reponse)
    {
        .channel_id = MHU_INTERFACE_ID(SCP_LOCAL, AP_CORE_SEC),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_SCMI_ON_ICC,
                .direction = ICC_SEND_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_AP_S_SEND_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (uintptr_t)mailbox_0,
        .mailbox_size = SIZE_OF_MAILBOX_TEST,
    },
    // AP TO SCP S Receive Message Configuration
    {
        .channel_id = MHU_INTERFACE_ID(AP_CORE_SEC, SCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_SCMI_ON_ICC,
                .direction = ICC_RECEIVE_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_AP_S_REC_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (uintptr_t)mailbox_1,
        .mailbox_size = SIZE_OF_MAILBOX_TEST,
    },
};

typedef struct
{
    uint32_t command;
    uint16_t size;
    uint8_t data[SIZE_OF_MSG_BUFFER];
} transport_message_t;


//
// Tests
//
TEST_FUNCTION(test_icc_mhu_trans_init, NULL, NULL)
{
    // Ensure that the MHU is initialized
    int status = icc_mhu_trans_init(icc_mhu_configuration, sizeof(icc_mhu_configuration) / sizeof(icc_mhu_configuration_t));
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_check_scmi_status_bit, NULL, NULL)
{
    icc_mhu_trans_check_scmi_status_bit(ICC_MSG_INDEX_SEND);
    icc_mhu_trans_check_scmi_status_bit(ICC_MSG_INDEX_RECEIVE);

    // Check if bits are set, note that mailbox_1 is used for received
    picc_mhu_packet_t packet = (picc_mhu_packet_t)mailbox_1;
    scmi_icc_packet_t* scmi_packet = (scmi_icc_packet_t*)packet->payload;
    assert_int_equal(scmi_packet->smt_header.status, 1);
}

TEST_FUNCTION(test_icc_mhu_trans_enable_interrupts, NULL, NULL)
{
    // Simple interrupt test, may need to expand further
    int status = icc_mhu_trans_enable_interrupts();
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_set_configuration_table, NULL, NULL)
{
    // Ensure that the MHU is initialized
    int status = icc_mhu_trans_set_configuration_table(NULL, 2);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);
    status = icc_mhu_trans_set_configuration_table(icc_mhu_configuration, 0);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);

    status = icc_mhu_trans_set_configuration_table(icc_mhu_configuration, 2);
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_send_message_idx, NULL, NULL)
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

    will_return(__wrap_icc_mhu_send_message, ICC_MHU_INVALID_INDEX);
    status = icc_mhu_trans_send_message_idx(ICC_MSG_INDEX_SEND, WRAPPER_ICC_COMMAND, data, sizeof(data));
    assert_int_equal(status, ICC_MHU_INVALID_INDEX);

}

TEST_FUNCTION(test_icc_mhu_trans_get_message, NULL, NULL)
{

    icc_msg_receive_t message;
    will_return(__wrap_icc_mhu_get_message, ICC_MHU_STATUS_SUCCESS);
    int status = __real_icc_mhu_trans_get_message(ICC_MSG_SCMI_RECEIVE_ID, &message);
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);

    will_return(__wrap_icc_mhu_get_message, ICC_MHU_INVALID_PARAMETER);
    status = __real_icc_mhu_trans_get_message(ICC_MSG_SCMI_RECEIVE_ID, &message);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);
}

TEST_FUNCTION(test_icc_mhu_trans_get_msg_from_index, NULL, NULL)
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

TEST_FUNCTION(test_icc_mhu_trans_get_config_entries, NULL, NULL)
{
    // Test properties entry
    icc_mhu_properties_t properties = icc_mhu_trans_get_config_entries();
    assert_int_equal(properties.pIcc_configuration_table, icc_mhu_configuration);
}

TEST_FUNCTION(test_icc_mhu_transport_dfwk_device_init, nullptr, nullptr)
{

    int status = icc_mhu_transport_dfwk_device_init(NULL, &Schedule, &config);
    assert_int_not_equal(status, DFWK_SUCCESS);

    status = icc_mhu_transport_dfwk_device_init(&icc_mhu_trans_dev, NULL, &config);
    assert_int_not_equal(status, DFWK_SUCCESS);

    status = icc_mhu_transport_dfwk_device_init(&icc_mhu_trans_dev, &Schedule, NULL);
    assert_int_not_equal(status, DFWK_SUCCESS);

    status = icc_mhu_transport_dfwk_device_init(&icc_mhu_trans_dev, &Schedule, &config);
    assert_int_equal(status, DFWK_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_dfwk_interface_init, nullptr, nullptr)
{
    int status = icc_mhu_trans_dfwk_interface_init(NULL, &icc_mhu_inf);
    assert_int_not_equal(status, DFWK_SUCCESS);

    status = icc_mhu_trans_dfwk_interface_init(&icc_mhu_trans_dev, NULL);
    assert_int_not_equal(status, DFWK_SUCCESS);

    status = icc_mhu_trans_dfwk_interface_init(&icc_mhu_trans_dev, &icc_mhu_inf);
    assert_true(icc_mhu_inf.device == &icc_mhu_trans_dev);
    assert_int_equal(status, DFWK_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_transport_dfwk_interface_open, nullptr, nullptr)
{
    DFWK_INTERFACE_HEADER* pInterface = (DFWK_INTERFACE_HEADER*)&icc_mhu_inf;
    int status = icc_mhu_transport_dfwk_interface_open(pInterface);
    assert_int_equal(status, DFWK_SUCCESS);
    assert_int_not_equal(icc_mhu_trans_dev.ref_count, 0);
}

TEST_FUNCTION(test_icc_mhu_transport_dfwk_interface_close, nullptr, nullptr)
{
    DFWK_INTERFACE_HEADER* pInterface = (DFWK_INTERFACE_HEADER*)&icc_mhu_inf;
    icc_mhu_transport_dfwk_interface_close(pInterface);
    assert_int_equal(icc_mhu_trans_dev.ref_count, 0);
}

TEST_FUNCTION(test_icc_mhu_transport_driver_dispatch_async, nullptr, nullptr)
{
    // Baseline tests, will fill in later when we fill in the implementations when needed
    DFWK_ASYNC_REQUEST_HEADER Request;
    Request.RequestType = ICC_TRANSPORT_RECV_ASYNC_REQUEST_ID;
    icc_mhu_transport_driver_dispatch_async(&Request, NULL);

    Request.RequestType = ICC_TRANSPORT_SEND_ASYNC_REQUEST_ID;
    icc_mhu_transport_driver_dispatch_async(&Request, NULL);
}

TEST_FUNCTION(test_icc_mhu_transport_driver_dispatch_sync, nullptr, nullptr)
{
    uint8_t data[64];

    DFWK_INTERFACE_HEADER* pInterface = (DFWK_INTERFACE_HEADER*)&icc_mhu_inf;
    icc_mhu_transport_dfwk_interface_open(pInterface);

    // Setup Request Parameters

    icc_mhu_transport_sync_message_t send_buffer;
    send_buffer.command = 0x00010002;
    send_buffer.payload = (uint8_t*)&data;
    send_buffer.size =  sizeof(data);

    // Checks for NULL

    fpfw_status_t status = icc_mhu_transport_driver_dispatch_sync(NULL);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    DFWK_SYNC_REQUEST_HEADER test_request;
    test_request.OwningInterface = NULL;
    status = icc_mhu_transport_driver_dispatch_sync(&test_request);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);


    FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST try_send_req;
    try_send_req.Header.RequestType = ICC_TRANSPORT_TRY_SEND_SYNC_REQUEST_ID;
    try_send_req.Input.PayloadBuffer = (uintptr_t)&send_buffer;
    try_send_req.Input.BufferSizeBytes = sizeof(send_buffer);

    PDFWK_SYNC_REQUEST_HEADER Request = (PDFWK_SYNC_REQUEST_HEADER) &try_send_req;
    Request->OwningInterface = (PDFWK_INTERFACE_HEADER)&icc_mhu_inf;
    will_return(__wrap_icc_mhu_send_message, ICC_MHU_STATUS_SUCCESS);
    status = icc_mhu_transport_driver_dispatch_sync(Request);
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);

    // Check if receiving messages
    FPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST try_recv_req;
    try_recv_req.Header.RequestType = ICC_TRANSPORT_TRY_RECV_SYNC_REQUEST_ID;
    try_recv_req.Input.PayloadBuffer = (uintptr_t)&send_buffer;
    try_recv_req.Input.BufferSizeBytes = sizeof(send_buffer);

    Request = (PDFWK_SYNC_REQUEST_HEADER) &try_recv_req;
    Request->OwningInterface = (PDFWK_INTERFACE_HEADER)&icc_mhu_inf;
    // fpfw_status_t status = icc_mhu_transport_driver_dispatch_sync(Request);
    will_return(__wrap_icc_mhu_trans_get_message, ICC_MHU_STATUS_SUCCESS);
    status = icc_mhu_transport_driver_dispatch_sync(Request);
    assert_int_equal(status, ICC_MHU_TRANS_INTF_MSG_AVAILABLE);

    FPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST get_size_req;
    get_size_req.Header.RequestType = ICC_TRANSPORT_GET_MAX_MESG_SIZE_SYNC_REQUEST_ID;
    Request = (PDFWK_SYNC_REQUEST_HEADER) &get_size_req;
    Request->OwningInterface = (PDFWK_INTERFACE_HEADER)&icc_mhu_inf;
    status = icc_mhu_transport_driver_dispatch_sync(Request);
    assert_int_equal(get_size_req.Output.MaxMesgSize, 256);
    assert_int_equal(status, DFWK_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_dfwk_send_sync_message, nullptr, nullptr)
{

    uint8_t data[8];
    fpfw_status_t status = icc_mhu_trans_dfwk_send_sync_message(NULL, 0, data, sizeof(data));
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    status = icc_mhu_trans_dfwk_send_sync_message((DFWK_INTERFACE_HEADER*)&icc_mhu_inf, 0, NULL, sizeof(data));
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    status = icc_mhu_trans_dfwk_send_sync_message((DFWK_INTERFACE_HEADER*)&icc_mhu_inf, 0, data, 512);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR);

    // fpfw_status_t status = icc_mhu_transport_driver_dispatch_sync(Request);
    will_return(__wrap_fpfw_icc_transport_try_send_sync_req, ICC_MHU_STATUS_SUCCESS);
    status = icc_mhu_trans_dfwk_send_sync_message((DFWK_INTERFACE_HEADER*)&icc_mhu_inf, 0, data, sizeof(data));
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_dfwk_recv_sync_message, nullptr, nullptr)
{

    uint8_t data[8];
    fpfw_status_t status = icc_mhu_trans_dfwk_recv_sync_message(NULL, data, sizeof(data));
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    status = icc_mhu_trans_dfwk_recv_sync_message((DFWK_INTERFACE_HEADER*)&icc_mhu_inf, NULL, sizeof(data));
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    status = icc_mhu_trans_dfwk_recv_sync_message((DFWK_INTERFACE_HEADER*)&icc_mhu_inf, data, 512);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR);

}

TEST_FUNCTION(test_icc_mhu_trans_dfwk_chk_recv_cmd_sync, nullptr, nullptr)
{

    uint8_t data[8];
    fpfw_status_t status = icc_mhu_trans_dfwk_chk_recv_cmd_sync(NULL, 0, data, sizeof(data));
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    status = icc_mhu_trans_dfwk_chk_recv_cmd_sync((DFWK_INTERFACE_HEADER*)&icc_mhu_inf, 0, NULL, sizeof(data));
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    status = icc_mhu_trans_dfwk_chk_recv_cmd_sync((DFWK_INTERFACE_HEADER*)&icc_mhu_inf, 0, data, 512);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR);

}