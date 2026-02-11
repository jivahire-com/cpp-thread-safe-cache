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
#include <stdint.h>

extern "C" {

// Define FpFwLock types locally for mocking
typedef struct _FPFW_LOCK
{
    int dummy;
} FPFW_LOCK, *PFPFW_LOCK;
typedef uint32_t FPFW_LOCK_STATE;

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static PFPFW_LOCK ws_mock_lock = NULL;

/*----------- Mock Functions -------------*/

/*------------- Functions ----------------*/

void __wrap_FpFwLockInitialize(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);
    ws_mock_lock = Lock;

    function_called();
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);
    assert_ptr_equal(ws_mock_lock, Lock);

    function_called();

    return mock_type(FPFW_LOCK_STATE);
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    assert_non_null(Lock);
    assert_ptr_equal(ws_mock_lock, Lock);
    check_expected(OldState);

    function_called();
}
}