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
#include <et_mts_client.h>               // for et_mts_rx_msg_handler
#include <etc_etd_svc.h>                 // for get_etc_buffer_size, get_etc_buffer_byte_pool_address
#include <idsw_kng.h>                    // for DIE_0, CPU_SCP, CPU_MCP
#include <mscp_exp_rmss_memory_map.h>    // for ETC_CORE_BUFFERS_MEMORY_SIZE
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
    return CPU_SCP; // Return a fixed core ID for testing
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

void __wrap_mts_client_send_new_trp_msg(p_trp_msg_t p_trp_msg)
{
    FPFW_UNUSED(p_trp_msg);
}

UINT __wrap__txe_block_pool_create(TX_BLOCK_POOL* pool_ptr, CHAR* name_ptr, ULONG block_size, VOID* pool_start, ULONG pool_size, UINT pool_control_block_size)
{
    check_expected_ptr(pool_ptr);
    check_expected_ptr(name_ptr);
    check_expected(block_size);
    check_expected_ptr(pool_start);
    check_expected(pool_size);
    check_expected(pool_control_block_size);

    return mock_type(UINT);
}

void __wrap_FPFwETControllerFlushBuffer(uint8_t die_id, uint8_t core_id, uintptr_t addr, uint32_t dsize)
{
    FPFW_UNUSED(die_id);
    FPFW_UNUSED(core_id);
    FPFW_UNUSED(addr);
    FPFW_UNUSED(dsize);

    function_called();
}

void __wrap_SCB_CleanDCache_by_Addr(uint32_t* addr, int32_t dsize)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(dsize);
}
}

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*------------------------------- Test Functions ----------------------------*/

int test_setup(void** ppContext)
{
    FPFW_UNUSED(ppContext);

    // Setup the threadx expectations
    expect_any(__wrap__txe_block_pool_create, pool_ptr);
    expect_any(__wrap__txe_block_pool_create, name_ptr);
    expect_value(__wrap__txe_block_pool_create, block_size, MAX_TRP_MSG_BLOCK_SIZE);
    expect_any(__wrap__txe_block_pool_create, pool_start);
    expect_any(__wrap__txe_block_pool_create, pool_size);
    expect_any(__wrap__txe_block_pool_create, pool_control_block_size);
    will_return(__wrap__txe_block_pool_create, TX_SUCCESS);

    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_any(__wrap__txe_thread_create, thread_ptr);
    expect_any(__wrap__txe_thread_create, name_ptr);
    expect_any(__wrap__txe_thread_create, entry_function);
    expect_any(__wrap__txe_thread_create, entry_input);
    expect_any(__wrap__txe_thread_create, stack_start);
    expect_any(__wrap__txe_thread_create, stack_size);
    expect_any(__wrap__txe_thread_create, priority);
    expect_any(__wrap__txe_thread_create, preempt_threshold);
    expect_any(__wrap__txe_thread_create, time_slice);
    expect_any(__wrap__txe_thread_create, auto_start);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    event_trace_mts_client_init();

    return TX_SUCCESS;
}

int test_teardown(void** ppContext)
{
    FPFW_UNUSED(ppContext);

    return TX_SUCCESS;
}

TEST_FUNCTION(test_et_mts_rx_msg_handler_trp_rd_intercore_block_complete, NULL, NULL)
{
    // Set up expectations for a queue receive error
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    trp_msg_t trp_msg;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE;
    trp_msg.payload.read_intercore_block_complete.block_id = ETC_SERVICE_CORE_BUFFER_COUNT - 1;
    will_return(set_tx_queue_receive_value, (uint32_t)&trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_any_always(__wrap_FPFwETControllerRecycleBuffer, pTraceController);
    expect_any_always(__wrap_FPFwETControllerRecycleBuffer, bufIndex);
    will_return(__wrap_FPFwETControllerRecycleBuffer, 0);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    et_mts_worker_thread_func(0);
}

TEST_FUNCTION(test_et_mts_rx_msg_handler_trp_rd_intercore_block_complete_invalid_block, NULL, NULL)
{
    // Set up expectations for queue receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    trp_msg_t trp_msg;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE;
    trp_msg.payload.read_intercore_block_complete.block_id = ETC_SERVICE_CORE_BUFFER_COUNT + 1;
    will_return(set_tx_queue_receive_value, (uint32_t)&trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    et_mts_worker_thread_func(0);
}

TEST_FUNCTION(test_et_mts_rx_msg_handler_rd_default, NULL, NULL)
{
    // Set up expectations for queue receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    trp_msg_t trp_msg;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE;
    will_return(set_tx_queue_receive_value, (uint32_t)&trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    et_mts_worker_thread_func(0);
}

TEST_FUNCTION(test_mts_notify_buffer_complete, NULL, NULL)
{
    FPFW_ET_CORE_BUFFER_HEADER core_buffer;

    etc_service_completion_request_t completion_request = {.core_buffer_request = {
                                                               .p_core_buffer = &core_buffer, // Use the valid core buffer
                                                               .p_origin_controller = nullptr // No specific controller for this test
                                                           }};

    for (int i = 0; i < ETC_SERVICE_CORE_BUFFER_COUNT; i++)
    {
        core_buffer.BufferId = i; // Set a valid buffer ID
        et_mts_notify_buffer_complete(nullptr, &completion_request);
    }
}

TEST_FUNCTION(test_mts_send_final_message, NULL, NULL)
{
    // expect_any(__wrap_FPFwETControllerFlushBuffer, die_id);
    // expect_any(__wrap_FPFwETControllerFlushBuffer, core_id);
    // expect_any(__wrap_FPFwETControllerFlushBuffer, addr);
    // expect_any(__wrap_FPFwETControllerFlushBuffer, dsize);
    expect_function_call(__wrap_FPFwETControllerFlushBuffer);
    event_trace_mts_send_final_message();
}