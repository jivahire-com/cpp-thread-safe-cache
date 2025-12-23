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
void __wrap_FpFwLockInitialize(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();

    return mock_type(FPFW_LOCK_STATE);
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    assert_non_null(Lock);
    check_expected(OldState);

    function_called();
}

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
    expect_function_call(__wrap_FpFwLockInitialize);
    __retarget_lock_init(&test_lock);
    assert_int_equal(test_lock->in_use, 1);

    // __retarget_lock_acquire
    will_return(__wrap_FpFwLockAcquire, 0xF1F2F3F4);
    expect_function_call(__wrap_FpFwLockAcquire);
    __retarget_lock_acquire(test_lock);
    assert_int_equal(test_lock->lock_state, 0xF1F2F3F4);

    // __retarget_lock_try_acquire
    int status = __retarget_lock_try_acquire(test_lock);
    assert_int_equal(status, -1);

    // __retarget_lock_release
    expect_value(__wrap_FpFwLockRelease, OldState, 0xF1F2F3F4);
    expect_function_call(__wrap_FpFwLockRelease);
    __retarget_lock_release(test_lock);

    // __retarget_lock_close
    __retarget_lock_close(test_lock);
    assert_int_equal(test_lock->in_use, 0);
}

TEST_FUNCTION(test__retarget_lock_recursive_basic, set_thread_system_state, NULL)
{
    // __retarget_lock_init_recursive
    expect_function_call(__wrap__txe_mutex_create);
    __retarget_lock_init_recursive(&test_lock);
    assert_int_equal(test_lock->in_use, 1);

    // __retarget_lock_acquire_recursive
    expect_function_call(__wrap__txe_mutex_get);
    __retarget_lock_acquire_recursive(test_lock);

    // __retarget_lock_release_recursive
    expect_function_call(__wrap__txe_mutex_put);
    __retarget_lock_release_recursive(test_lock);

    // __retarget_lock_try_acquire_recursive
    expect_function_call(__wrap__txe_mutex_get);
    int status = __retarget_lock_try_acquire_recursive(test_lock);
    assert_int_equal(status, 0);

    // __retarget_lock_release_recursive
    expect_function_call(__wrap__txe_mutex_put);
    __retarget_lock_release_recursive(test_lock);

    // __retarget_lock_close_recursive
    expect_function_call(__wrap__txe_mutex_delete);
    __retarget_lock_close_recursive(test_lock);
    assert_int_equal(test_lock->in_use, 0);
}

TEST_FUNCTION(test__retarget_lock_recursive, set_thread_system_state, NULL)
{
    // __retarget_lock_init_recursive
    expect_function_call(__wrap__txe_mutex_create);
    __retarget_lock_init_recursive(&test_lock);
    assert_int_equal(test_lock->in_use, 1);

    // __retarget_lock_acquire_recursive twice
    expect_function_call(__wrap__txe_mutex_get);
    __retarget_lock_acquire_recursive(test_lock);

    expect_function_call(__wrap__txe_mutex_get);
    __retarget_lock_acquire_recursive(test_lock);

    // __retarget_lock_release_recursive
    expect_function_call(__wrap__txe_mutex_put);
    __retarget_lock_release_recursive(test_lock);

    // __retarget_lock_release_recursive, mutex put expected as last release
    expect_function_call(__wrap__txe_mutex_put);
    __retarget_lock_release_recursive(test_lock);
}