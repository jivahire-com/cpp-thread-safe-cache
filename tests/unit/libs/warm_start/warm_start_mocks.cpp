//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file warm_start_test_mocks.cpp
 * Mocks for warm_start
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
extern "C" {
#include <tx_api.h> // for UINT, TX_MUTEX, CHAR, ULONG

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
// FPFwCDDumpDescriptor desc_list[];
TX_MUTEX* ws_mock_mutex;
/*----------- Mock Functions -------------*/

/*------------- Functions ----------------*/

void init_ws_mutex()
{
    ws_mock_mutex = NULL;
}

UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit, UINT mutex_control_block_size)
{
    assert_null(ws_mock_mutex); // Ensure this function is called first time
    assert_non_null(mutex_ptr); // Ensure the mutex pointer is not NULL
    ws_mock_mutex = mutex_ptr;  // Save the mutex pointer

    check_expected(name_ptr);
    FPFW_UNUSED(inherit);
    FPFW_UNUSED(mutex_control_block_size);

    function_called();

    return 0;
}

UINT __wrap__txe_mutex_get(TX_MUTEX* mutex_ptr, ULONG wait_option)
{
    assert_ptr_equal(ws_mock_mutex, mutex_ptr); // Ensure the mutex pointer is the same as the one initialized

    assert_int_equal(wait_option, TX_WAIT_FOREVER);

    function_called();

    return 0;
}

UINT __wrap__txe_mutex_put(TX_MUTEX* mutex_ptr)
{
    assert_ptr_equal(ws_mock_mutex, mutex_ptr); // Ensure the mutex pointer is the same as the one initialized

    function_called();
    return 0;
}
}