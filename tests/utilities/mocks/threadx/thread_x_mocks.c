//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file thread_x_mocks.c
 * Mocks for threadx
 */

/*------------- Includes -----------------*/

#include <FpFwCMocka.h>
#include <thread_x_mocks.h>
#include <tx_api.h> // for UINT, ULONG, TX_QUEUE, TX_S...

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern uint32_t _tx_thread_system_state; // ThreadX global used by 'TX_THREAD_GET_SYSTEM_STATE'

/*------------- Functions ----------------*/

void threadx_mock_set_system_state(uint32_t state)
{
    _tx_thread_system_state = state;
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

UINT __wrap__txe_semaphore_create(TX_SEMAPHORE* semaphore_ptr, CHAR* name_ptr, ULONG initial_count, UINT semaphore_control_block_size)
{
    check_expected_ptr(semaphore_ptr);
    check_expected_ptr(name_ptr);
    check_expected(initial_count);
    check_expected(semaphore_control_block_size);
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

UINT __wrap__txe_queue_send(TX_QUEUE* queue_ptr, VOID* source_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(source_ptr);
    check_expected(wait_option);
    return mock_type(UINT);
}

UINT __wrap__txe_queue_receive(TX_QUEUE* queue_ptr, VOID* destination_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(destination_ptr);
    check_expected(wait_option);
    return mock_type(UINT);
}

UINT __wrap__txe_semaphore_get(TX_SEMAPHORE* semaphore_ptr, ULONG wait_option)
{
    check_expected_ptr(semaphore_ptr);
    check_expected(wait_option);
    return mock_type(UINT);
}

UINT __wrap__txe_semaphore_put(TX_SEMAPHORE* semaphore_ptr)
{
    check_expected_ptr(semaphore_ptr);
    return mock_type(UINT);
}
