//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tx_mock.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <tx_api.h>    // for tx_queue_create, tx_queue_receive, tx_queue_send, tx_thread_create

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
UINT __wrap__txe_queue_create(TX_QUEUE* queue_ptr, CHAR* name_ptr, UINT message_size, VOID* queue_start, ULONG queue_size, UINT queue_control_block_size)
{
    FPFW_UNUSED(queue_ptr);
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(message_size);
    FPFW_UNUSED(queue_start);
    FPFW_UNUSED(queue_size);
    FPFW_UNUSED(queue_control_block_size);

    function_called();

    return 0;
}

UINT __wrap__txe_thread_create(TX_THREAD* thread_ptr,
                               CHAR* name_ptr,
                               VOID (*entry_function)(ULONG entry_input),
                               ULONG entry_input,
                               VOID* stack_start,
                               ULONG stack_size,
                               UINT priority,
                               UINT preempt_threshold,
                               ULONG time_slice,
                               UINT auto_start,
                               UINT thread_control_block_size)
{
    FPFW_UNUSED(thread_ptr);
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(entry_function);
    FPFW_UNUSED(entry_input);
    FPFW_UNUSED(stack_start);
    FPFW_UNUSED(stack_size);
    FPFW_UNUSED(priority);
    FPFW_UNUSED(preempt_threshold);
    FPFW_UNUSED(time_slice);
    FPFW_UNUSED(auto_start);
    FPFW_UNUSED(thread_control_block_size);

    function_called();

    return 0;
}

ULONG generate_message()
{
    return mock_type(ULONG);
}

UINT __wrap__txe_queue_receive(TX_QUEUE* queue_ptr, VOID* destination_ptr, ULONG wait_option)
{
    assert_non_null(queue_ptr);
    assert_non_null(destination_ptr);
    check_expected(wait_option);

    ULONG* message = (ULONG*)destination_ptr;
    *message = generate_message();

    function_called();

    return mock_type(UINT);
}

UINT __wrap__txe_queue_send(TX_QUEUE* queue_ptr, VOID* source_ptr, ULONG wait_option)
{
    assert_non_null(queue_ptr);
    check_expected(*((uint32_t*)source_ptr));
    FPFW_UNUSED(wait_option);

    function_called();

    return 0;
}

//
// Tests
//

} // extern "C"