//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_svc_test.cpp
 * This file implements unit test functions for event_trace service
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // for TEST_FUNCTION
#include <cstddef>         // for NULL

extern "C" {
#include <et_mts_client.h>               // for event_trace_mts_client_notify_new_msg_cb
#include <etc_etd_svc.h>                 // for get_etc_buffer_size, get_etc_buffer_address
#include <mts_platform_specialization.h> // for p_trp_msg_t
#include <thread_x_mocks.h>              // for set_txe_queue_receive_callback_func
#include <transfer_relay_protocol.h> // for trp_msg_t, TRP_MSG_ID_READ_INTERCORE_BLOCK, TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE
#include <tx_api.h> // for tx_queue_receive, TX_NO_WAIT, UINT

/*------------------------------- Mock Functions ----------------------------*/

void set_tx_queue_receive_value(VOID* destination_ptr)
{
    // Copy the parameter to the destination
    *(uint32_t*)destination_ptr = mock_type(int);
}

UINT __wrap__txe_block_release(VOID* block_ptr)
{
    FPFW_UNUSED(block_ptr);
    function_called();

    return mock_type(UINT);
}

uint32_t __wrap_mts_get_this_die_id()
{
    return 0;
}

uint32_t __wrap_mts_get_this_core_id()
{
    return 0; // Return a fixed core ID for testing
}

uint32_t __wrap_fpfw_crc32(uint32_t crc, const void* data, size_t length)
{
    FPFW_UNUSED(crc);
    FPFW_UNUSED(data);
    FPFW_UNUSED(length);

    // This is a mock implementation, so we return a fixed CRC value for testing purposes.
    return 0x12345678; // Return a fixed CRC value for testing
}

void __wrap_mts_client_send_trp_response(p_trp_msg_t p_trp_msg)
{
    FPFW_UNUSED(p_trp_msg);
}

// uint32_t __wrap_get_etc_buffer_size()
// {
//     // Return a fixed buffer size for testing purposes
//     return ETC_CORE_BUFFERS_MEMORY_SIZE;
// }

// void* __wrap_get_etc_buffer_address()
// {
//     // Return a fixed buffer address for testing purposes
//     return (void*)0x12345678;
// }
}

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*------------------------------- Test Functions ----------------------------*/

/**
 * @brief Test function for getting the ETC buffer size
 *
 * @return * Test
 */

TEST_FUNCTION(test_get_etc_buffer_size, NULL, NULL)
{
    // Call the function to get the buffer size
    size_t buffer_size = ETC_CORE_BUFFERS_MEMORY_SIZE;

    // Assert that the buffer size is equal to the expected size
    assert_true(buffer_size == ETC_CORE_BUFFERS_MEMORY_SIZE);
}

/**
 * @brief Test function for getting the ETC buffer address
 *
 * @return * Test
 */

TEST_FUNCTION(test_get_etc_buffer_address, NULL, NULL)
{
    // Call the function to get the buffer address
    void* buffer_address = get_etc_buffer_address();

    // Assert that the buffer address is not NULL
    assert_non_null(buffer_address);
}

TEST_FUNCTION(test_event_trace_mts_client_notify_new_msg_cb_queue_receive_fail, NULL, NULL)
{
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_ERROR);

    event_trace_mts_client_notify_new_msg_cb();
}

TEST_FUNCTION(test_event_trace_mts_client_notify_new_msg_cb_null_msg, NULL, NULL)
{
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    event_trace_mts_client_notify_new_msg_cb();
}

TEST_FUNCTION(test_event_trace_mts_client_notify_new_msg_cb_rd_intercore_block_response, NULL, NULL)
{
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    trp_msg_t trp_msg;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK;
    will_return(set_tx_queue_receive_value, (uint32_t)&trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    event_trace_mts_client_notify_new_msg_cb();
}

TEST_FUNCTION(test_event_trace_mts_client_notify_new_msg_cb_rd_intercore_block_complete, NULL, NULL)
{
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    trp_msg_t trp_msg;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE;
    will_return(set_tx_queue_receive_value, (uint32_t)&trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    event_trace_mts_client_notify_new_msg_cb();
}

TEST_FUNCTION(test_event_trace_mts_client_notify_new_msg_cb_rd_default, NULL, NULL)
{
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    trp_msg_t trp_msg;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE;
    will_return(set_tx_queue_receive_value, (uint32_t)&trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    event_trace_mts_client_notify_new_msg_cb();
}
