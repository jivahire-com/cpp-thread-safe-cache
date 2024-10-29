//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mock_bug_check.c
 * Implementation of bug check mocks
 */

/*------------- Includes -----------------*/

#include "mock_bug_check.h"

#include <FpFwUtils.h>
#include <cmocka.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
jmp_buf cd_mock_jump_buf;

/*------------- Functions ----------------*/
_Noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Handle noreturn, allowing control to return to test
    longjmp(cd_mock_jump_buf, BUG_CHECK_RETURN_VALUE);
}
