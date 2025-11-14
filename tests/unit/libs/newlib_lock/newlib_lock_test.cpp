//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_stdio.cpp
 * STDIO tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {

#include <FpFwUtils.h>
#include <newlib_lock.h>
#include <stdbool.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static _LOCK_T test_lock = NULL;
extern volatile ULONG _tx_thread_system_state;
/*------------- Functions ----------------*/

// Mocks for dependencies
UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit)
{
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(inherit);
    FPFW_UNUSED(mutex_ptr);

    function_called();
    return TX_SUCCESS;
}

UINT __wrap__txe_mutex_delete(TX_MUTEX* mutex_ptr)
{
    FPFW_UNUSED(mutex_ptr);

    function_called();
    return TX_SUCCESS;
}

UINT __wrap__txe_mutex_get(TX_MUTEX* mutex_ptr, ULONG wait_option)
{
    FPFW_UNUSED(mutex_ptr);
    FPFW_UNUSED(wait_option);
    function_called();
    return TX_SUCCESS;
}

UINT __wrap__txe_mutex_put(TX_MUTEX* mutex_ptr)
{
    FPFW_UNUSED(mutex_ptr);
    function_called();
    return TX_SUCCESS;
}

TX_THREAD* __wrap__tx_thread_identify(VOID)
{
    return (_tx_thread_system_state == 0) ? (TX_THREAD*)0x12345678 : NULL;
}

} // extern "C"

int set_thread_system_state(void** state)
{
    FPFW_UNUSED(state);
    _tx_thread_system_state = 0;
    return 0;
}

TEST_FUNCTION(test__retarget_lock_normal, set_thread_system_state, NULL)
{
    // __retarget_lock_init
    expect_function_call(__wrap__txe_mutex_create);
    __retarget_lock_init(&test_lock);
    threadx_lock_t* threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->in_use, 1);

    // __retarget_lock_acquire
    expect_function_call(__wrap__txe_mutex_get);
    __retarget_lock_acquire(test_lock);

    // __retarget_lock_try_acquire
    expect_function_call(__wrap__txe_mutex_get);
    int status = __retarget_lock_try_acquire(test_lock);
    assert_int_equal(status, 0);

    // __retarget_lock_release
    expect_function_call(__wrap__txe_mutex_put);
    __retarget_lock_release(test_lock);

    // __retarget_lock_close
    expect_function_call(__wrap__txe_mutex_delete);
    __retarget_lock_close(test_lock);
    threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->in_use, 0);
}

TEST_FUNCTION(test__retarget_lock_recursive_basic, set_thread_system_state, NULL)
{
    // __retarget_lock_init_recursive
    expect_function_call(__wrap__txe_mutex_create);
    __retarget_lock_init_recursive(&test_lock);
    threadx_lock_t* threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->in_use, 1);

    // __retarget_lock_acquire_recursive
    expect_function_call(__wrap__txe_mutex_get);
    __retarget_lock_acquire_recursive(test_lock);
    threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->recursion, 1);

    // __retarget_lock_release_recursive
    expect_function_call(__wrap__txe_mutex_put);
    __retarget_lock_release_recursive(test_lock);
    threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->recursion, 0);

    // __retarget_lock_try_acquire_recursive
    expect_function_call(__wrap__txe_mutex_get);
    int status = __retarget_lock_try_acquire_recursive(test_lock);
    assert_int_equal(status, 0);

    // __retarget_lock_release_recursive
    expect_function_call(__wrap__txe_mutex_put);
    __retarget_lock_release_recursive(test_lock);
    threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->recursion, 0);

    // __retarget_lock_close_recursive
    expect_function_call(__wrap__txe_mutex_delete);
    __retarget_lock_close_recursive(test_lock);
    threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->in_use, 0);
}

TEST_FUNCTION(test__retarget_lock_recursive, set_thread_system_state, NULL)
{
    // __retarget_lock_init_recursive
    expect_function_call(__wrap__txe_mutex_create);
    __retarget_lock_init_recursive(&test_lock);
    threadx_lock_t* threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->in_use, 1);

    // __retarget_lock_acquire_recursive twice, only one mutex get expected
    expect_function_call(__wrap__txe_mutex_get);
    __retarget_lock_acquire_recursive(test_lock);
    __retarget_lock_acquire_recursive(test_lock);
    threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->recursion, 2);

    // __retarget_lock_release_recursive, no mutex put expected
    __retarget_lock_release_recursive(test_lock);
    threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->recursion, 1);

    // __retarget_lock_release_recursive, mutex put expected as last release
    expect_function_call(__wrap__txe_mutex_put);
    __retarget_lock_release_recursive(test_lock);
    threadx_lock = (threadx_lock_t*)test_lock;
    assert_int_equal(threadx_lock->recursion, 0);
}