//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcs_mock.c
 * Provide mock functions for data_collection tests
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // for check_expected, check_expected_ptr, mock_type
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <fpfw_icc_base.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCEEDED, fpf...
#include <stddef.h>      // for size_t
#include <stdint.h>      // for int32_t
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __wrap_FpFwAssertWithArgs(int expression, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    FPFW_UNUSED(expression);
    FPFW_UNUSED(arg0);
    FPFW_UNUSED(arg1);
    FPFW_UNUSED(arg2);
    FPFW_UNUSED(arg3);

    function_called();
}

fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(payload_buffer);
    FPFW_UNUSED(buffer_size);

    return mock_type(fpfw_status_t);
}

UINT __wrap__txe_block_pool_create(TX_BLOCK_POOL* pool_ptr, CHAR* name_ptr, ULONG block_size, VOID* pool_start, ULONG pool_size, UINT pool_control_block_size)
{
    FPFW_UNUSED(pool_ptr);
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(block_size);
    FPFW_UNUSED(pool_start);
    FPFW_UNUSED(pool_size);
    FPFW_UNUSED(pool_control_block_size);

    function_called();

    return mock_type(UINT);
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    check_expected_ptr(icc_ctx);
    check_expected_ptr(params);

    return mock_type(fpfw_status_t);
}

UINT __wrap__txe_block_allocate(TX_BLOCK_POOL* pool_ptr, VOID** block_ptr, ULONG wait_option)
{
    FPFW_UNUSED(pool_ptr);
    FPFW_UNUSED(block_ptr);
    FPFW_UNUSED(wait_option);
    *block_ptr = mock_type(VOID**);
    function_called();

    return mock_type(UINT);
}

UINT __wrap__txe_block_release(VOID* block_ptr)
{
    FPFW_UNUSED(block_ptr);
    function_called();

    return mock_type(UINT);
}

UINT __wrap__txe_queue_receive(TX_QUEUE* queue_ptr, VOID* destination_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(destination_ptr);
    check_expected(wait_option);

    size_t mockSize = mock_type(size_t);
    void* pMockData = mock_ptr_type(void*);

    if (destination_ptr && pMockData)
    {
        memcpy(destination_ptr, pMockData, mockSize); // NOLINT memcpy ok for mock
    }

    return mock_type(UINT);
}

UINT __wrap__txe_thread_create(TX_THREAD* thread_ptr,
                               CHAR* name_ptr,
                               VOID (*entry_function)(ULONG id),
                               ULONG entry_input,
                               VOID* stack_start,
                               ULONG stack_size,
                               UINT priority,
                               UINT preempt_threshold,
                               ULONG time_slice,
                               UINT auto_start,
                               UINT thread_control_block_size)
{
    check_expected_ptr(thread_ptr);
    check_expected_ptr(name_ptr);
    check_expected_ptr(entry_function);
    check_expected(entry_input);
    check_expected_ptr(stack_start);
    check_expected(stack_size);
    check_expected(priority);
    check_expected(preempt_threshold);
    check_expected(time_slice);
    check_expected(auto_start);
    check_expected(thread_control_block_size);
    return mock_type(UINT);
}

UINT __wrap__txe_queue_create(TX_QUEUE* queue_ptr, CHAR* name_ptr, UINT message_size, VOID* queue_start, ULONG queue_size, UINT queue_control_block_size)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(name_ptr);
    check_expected(message_size);
    check_expected_ptr(queue_start);
    check_expected(queue_size);
    check_expected(queue_control_block_size);
    return mock_type(UINT);
}

UINT __wrap__txe_queue_send(TX_QUEUE* queue_ptr, VOID* source_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(source_ptr);
    check_expected(wait_option);
    return mock_type(UINT);
}

UINT __wrap__txe_event_flags_get(TX_EVENT_FLAGS_GROUP* group_ptr, ULONG requested_flags, UINT get_option, ULONG* actual_flags_ptr, ULONG wait_option)
{
    check_expected_ptr(group_ptr);
    check_expected(requested_flags);
    check_expected(get_option);
    check_expected_ptr(actual_flags_ptr);
    check_expected(wait_option);

    // We are assigning expected value once and not-expected value once.
    // This helps in covering all necessary testcases
    static int count = 0x0;

    if (count == 0)
    {
        *actual_flags_ptr = requested_flags;
        count++;
    }
    else
    {
        *actual_flags_ptr = ~requested_flags;
        count = 0x0;
    }

    return mock_type(UINT);
}

UINT __wrap__txe_event_flags_set(TX_EVENT_FLAGS_GROUP* group_ptr, ULONG flags_to_set, UINT set_option)
{
    check_expected_ptr(group_ptr);
    check_expected(flags_to_set);
    check_expected(set_option);

    return mock_type(UINT);
}

UINT __wrap__txe_event_flags_create(TX_EVENT_FLAGS_GROUP* group_ptr, CHAR* name_ptr, UINT event_control_block_size)
{
    check_expected_ptr(group_ptr);
    check_expected_ptr(name_ptr);
    check_expected(event_control_block_size);
    return mock_type(UINT);
}
