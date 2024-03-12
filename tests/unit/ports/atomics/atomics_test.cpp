//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file atomics_test.cpp
 * Test for atomics port
 */

/*------------- Includes -----------------*/

extern "C" {
#include <AtomicOperations.h>
#include <CMockaWrapper.h>
#include <cstddef>
#include <cstdint>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

TEST_FUNCTION(atomic_add, NULL, NULL)
{
    uint32_t x = 0;
    assert_int_equal(2, FPFwAtomicAdd(&x, 2));
}

TEST_FUNCTION(atomic_sub, NULL, NULL)
{
    uint32_t x = 2;
    assert_int_equal(0, FPFwAtomicSubtract(&x, 2));
}

TEST_FUNCTION(atomic_com_ex_no_change, NULL, NULL)
{
    uint32_t x = 2;
    assert_int_equal(2, FPFwAtomicCompareExchange(&x, 0, 0));
    assert_int_equal(2, x);
}

TEST_FUNCTION(atomic_com_ex_change, NULL, NULL)
{
    uint32_t x = 0;
    assert_int_equal(0, FPFwAtomicCompareExchange(&x, 1, 0));
    assert_int_equal(1, x);
}
