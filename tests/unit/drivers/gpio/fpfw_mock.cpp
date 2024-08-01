//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file fpfw_mock.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwLock.h>  // for FpFwLockAcquire, FpFwLockRelease
#include <FpFwUtils.h> // for FPFW_UNUSED

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const char* id)
{
    check_expected_ptr(id);
    function_called();

    return mock_ptr_type(void*);
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    FPFW_UNUSED(Lock);
    return 0;
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    FPFW_UNUSED(Lock);
    FPFW_UNUSED(OldState);
}

//
// Tests
//

} // extern "C"