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

#include <DfwkHost.h>
#include <FPFwInterrupts.h>
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
#include <tx_api.h>
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
    .irq_num = 1,
};

// Test Configuration table used for primitives initialization
static icc_mhu_configuration_t d0_icc_mhu_config[] = {
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

static uint8_t test_buffer[SIZE_OF_MAILBOX_TEST];

static icc_mhu_request_t* test_request = (icc_mhu_request_t*)test_buffer;

static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);

    memset(mailbox_0, 0, sizeof(mailbox_0));
    memset(mailbox_1, 0, sizeof(mailbox_1));
    memset(test_buffer, 0, sizeof(test_buffer));

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_icc_mhu_trans_init, NULL, NULL)
{
    // Ensure that the MHU is initialized
    int status = icc_mhu_trans_init(d0_icc_mhu_config, sizeof(d0_icc_mhu_config) / sizeof(icc_mhu_configuration_t));
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

TEST_FUNCTION(test_icc_mhu_trans_set_configuration_table, test_setup, NULL)
{
    // Ensure that the MHU is initialized
    int status = icc_mhu_trans_set_configuration_table(NULL, 2);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);
    status = icc_mhu_trans_set_configuration_table(d0_icc_mhu_config, 0);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);

    status = icc_mhu_trans_set_configuration_table(d0_icc_mhu_config, 2);
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_get_message, test_setup, NULL)
{

    icc_msg_receive_t message;
    will_return(__wrap_icc_mhu_get_message, ICC_MHU_STATUS_SUCCESS);
    int status = __real_icc_mhu_trans_get_message(ICC_MSG_SCMI_RECEIVE_ID, &message);
    assert_int_equal(status, ICC_MHU_STATUS_SUCCESS);

    will_return(__wrap_icc_mhu_get_message, ICC_MHU_INVALID_PARAMETER);
    status = __real_icc_mhu_trans_get_message(ICC_MSG_SCMI_RECEIVE_ID, &message);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);
}

TEST_FUNCTION(test_icc_mhu_trans_get_cmd_msg_from_index, test_setup, NULL)
{

    test_request->header.msg_header.command = WRAPPER_ICC_COMMAND;
    test_request->header.msg_header.payload_size = sizeof(test_buffer) - sizeof(icc_mhu_header_t);

    will_return(__wrap_icc_mhu_trans_get_message, ICC_MHU_STATUS_SUCCESS);
    int status = icc_mhu_trans_get_cmd_msg_from_index(ICC_MSG_INDEX_RECEIVE, test_request);
    assert_int_equal(status, ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE);

    test_request->header.msg_header.command = WRAPPER_ICC_COMMAND_FAIL;
    will_return(__wrap_icc_mhu_trans_get_message, ICC_MHU_STATUS_SUCCESS);
    status = icc_mhu_trans_get_cmd_msg_from_index(ICC_MSG_INDEX_RECEIVE, test_request);
    assert_int_equal(status, ICC_MHU_TRANS_NO_CMD_MESSAGE_AVAILABLE);

    will_return(__wrap_icc_mhu_trans_get_message, ICC_MHU_INVALID_PARAMETER);
    status = icc_mhu_trans_get_cmd_msg_from_index(ICC_MSG_INDEX_RECEIVE, test_request);
    assert_int_equal(status, ICC_MHU_INVALID_PARAMETER);
}

TEST_FUNCTION(test_icc_mhu_trans_get_config_entries, test_setup, NULL)
{
    // Test properties entry
    icc_mhu_properties_t properties = icc_mhu_trans_get_config_entries();
    assert_int_equal(properties.pIcc_configuration_table, d0_icc_mhu_config);
}

TEST_FUNCTION(test_icc_mhu_transport_dfwk_device_init, test_setup, nullptr)
{

    int status = icc_mhu_transport_dfwk_device_init(NULL, &Schedule, &config);
    assert_int_not_equal(status, DFWK_SUCCESS);

    status = icc_mhu_transport_dfwk_device_init(&icc_mhu_trans_dev, NULL, &config);
    assert_int_not_equal(status, DFWK_SUCCESS);

    status = icc_mhu_transport_dfwk_device_init(&icc_mhu_trans_dev, &Schedule, NULL);
    assert_int_not_equal(status, DFWK_SUCCESS);

    expect_value(__wrap_FPFwCoreInterruptRegisterCallback, irqnum, 1);
    expect_any(__wrap_FPFwCoreInterruptRegisterCallback, handler);
    expect_any(__wrap_FPFwCoreInterruptRegisterCallback, arg);
    will_return(__wrap_FPFwCoreInterruptRegisterCallback, 0);

    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    status = icc_mhu_transport_dfwk_device_init(&icc_mhu_trans_dev, &Schedule, &config);
    assert_int_equal(status, DFWK_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_trans_dfwk_interface_init, test_setup, nullptr)
{
    int status = icc_mhu_trans_dfwk_interface_init(NULL, &icc_mhu_inf);
    assert_int_not_equal(status, DFWK_SUCCESS);

    status = icc_mhu_trans_dfwk_interface_init(&icc_mhu_trans_dev, NULL);
    assert_int_not_equal(status, DFWK_SUCCESS);

    status = icc_mhu_trans_dfwk_interface_init(&icc_mhu_trans_dev, &icc_mhu_inf);
    assert_true(icc_mhu_inf.device == &icc_mhu_trans_dev);
    assert_int_equal(status, DFWK_SUCCESS);
}

TEST_FUNCTION(test_icc_mhu_transport_dfwk_interface_open, test_setup, nullptr)
{
    DFWK_INTERFACE_HEADER* pInterface = (DFWK_INTERFACE_HEADER*)&icc_mhu_inf;
    int status = icc_mhu_transport_dfwk_interface_open(pInterface);
    assert_int_equal(status, DFWK_SUCCESS);
    assert_int_not_equal(icc_mhu_trans_dev.ref_count, 0);
}

TEST_FUNCTION(test_icc_mhu_transport_dfwk_interface_close, test_setup, nullptr)
{
    DFWK_INTERFACE_HEADER* pInterface = (DFWK_INTERFACE_HEADER*)&icc_mhu_inf;
    icc_mhu_transport_dfwk_interface_close(pInterface);
    assert_int_equal(icc_mhu_trans_dev.ref_count, 0);
}

TEST_FUNCTION(test_icc_mhu_transport_driver_dispatch_async_send, test_setup, nullptr)
{
    // Fill out the request
    test_request->header.msg_header.command = 0x00010002;
    test_request->header.msg_header.payload_size = sizeof(test_buffer);

    FPFW_ICC_TRANSPORT_ASYNC_SEND_REQUEST async_send_req = {.Input = {
                                                                .PayloadBuffer = (uintptr_t)test_request,
                                                                .BufferSizeBytes = sizeof(test_buffer),
                                                            }};

    //
    // A request that successfully sends right away
    //
    timer_active_status = TX_FALSE;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &async_send_req.Header);
    will_return(__wrap_icc_mhu_send_message, ICC_MHU_STATUS_SUCCESS);
    dispatch_async_send((PDFWK_ASYNC_REQUEST_HEADER)&async_send_req, &icc_mhu_trans_dev);

    //
    // A request that has triggered the timer and successfully sends via
    // the timer firing
    //
    timer_active_status = TX_TRUE;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &async_send_req.Header);
    will_return(__wrap_icc_mhu_send_message, ICC_MHU_STATUS_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    dispatch_async_send((PDFWK_ASYNC_REQUEST_HEADER)&async_send_req, &icc_mhu_trans_dev);

    //
    // A request that fails to send and starts the timer
    //
    timer_active_status = TX_FALSE;
    will_return(__wrap_icc_mhu_send_message, -1);
    will_return(__wrap__txe_timer_activate, TX_SUCCESS);
    dispatch_async_send((PDFWK_ASYNC_REQUEST_HEADER)&async_send_req, &icc_mhu_trans_dev);
}

TEST_FUNCTION(test_icc_mhu_transport_driver_dispatch_async_recv, test_setup, nullptr)
{

    FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST async_recv_req = {.Input = {
                                                                .PayloadBuffer = (uintptr_t)test_request,
                                                                .BufferSizeBytes = sizeof(test_buffer),
                                                            }};

    // Setup a request
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, 1);
    will_return(__wrap_FPFwCoreInterruptEnableVector, 0);
    dispatch_async_recv((PDFWK_ASYNC_REQUEST_HEADER)&async_recv_req, &icc_mhu_trans_dev);

    assert_ptr_equal(icc_mhu_trans_dev.async_recv_ctx.request_ref, &async_recv_req.Header);

    // Trigger the ISR
    expect_value(__wrap_FPFwCoreInterruptDisableVector, irqnum, 1);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &async_recv_req.Header);
    will_return(__wrap_FPFwCoreInterruptDisableVector, 0);
    will_return(__wrap_icc_mhu_check_message_received, ICC_MHU_STATUS_SUCCESS);
    will_return(__wrap_icc_mhu_trans_get_message, ICC_MHU_STATUS_SUCCESS);
    icc_mhu_isr((void*)&icc_mhu_trans_dev);
}

TEST_FUNCTION(test_icc_mhu_transport_driver_dispatch_sync, test_setup, nullptr)
{
    uint8_t data[64];

    DFWK_INTERFACE_HEADER* pInterface = (DFWK_INTERFACE_HEADER*)&icc_mhu_inf;
    icc_mhu_transport_dfwk_interface_open(pInterface);

    // Setup Request Parameters
    test_request->header.msg_header.command = 0x00010002;
    test_request->header.msg_header.payload_size = sizeof(data);

    // Checks for NULL
    fpfw_status_t status = icc_mhu_transport_driver_dispatch_sync(NULL);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    DFWK_SYNC_REQUEST_HEADER sync_req;
    sync_req.OwningInterface = NULL;
    status = icc_mhu_transport_driver_dispatch_sync(&sync_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST try_send_req;
    try_send_req.Header.RequestType = ICC_TRANSPORT_TRY_SEND_SYNC_REQUEST_ID;
    try_send_req.Input.PayloadBuffer = (uintptr_t)test_request;
    try_send_req.Input.BufferSizeBytes = sizeof(test_buffer);

    PDFWK_SYNC_REQUEST_HEADER Request = (PDFWK_SYNC_REQUEST_HEADER)&try_send_req;
    Request->OwningInterface = (PDFWK_INTERFACE_HEADER)&icc_mhu_inf;
    will_return(__wrap_icc_mhu_send_message, ICC_MHU_STATUS_SUCCESS);
    status = icc_mhu_transport_driver_dispatch_sync(Request);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);

    // Check if receiving messages
    FPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST try_recv_req;
    try_recv_req.Header.RequestType = ICC_TRANSPORT_TRY_RECV_SYNC_REQUEST_ID;
    try_recv_req.Input.PayloadBuffer = (uintptr_t)test_request;
    try_recv_req.Input.BufferSizeBytes = sizeof(test_buffer);

    Request = (PDFWK_SYNC_REQUEST_HEADER)&try_recv_req;
    Request->OwningInterface = (PDFWK_INTERFACE_HEADER)&icc_mhu_inf;
    will_return(__wrap_icc_mhu_trans_get_message, ICC_MHU_STATUS_SUCCESS);
    status = icc_mhu_transport_driver_dispatch_sync(Request);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);

    FPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST get_size_req;
    get_size_req.Header.RequestType = ICC_TRANSPORT_GET_MAX_MESG_SIZE_SYNC_REQUEST_ID;
    Request = (PDFWK_SYNC_REQUEST_HEADER)&get_size_req;
    Request->OwningInterface = (PDFWK_INTERFACE_HEADER)&icc_mhu_inf;
    status = icc_mhu_transport_driver_dispatch_sync(Request);
    assert_int_equal(get_size_req.Output.MaxMesgSize, 256);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
}
