#pragma once
#include "bugcheck.h"

// Staticly assert a condition to be true
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

// Indicate failure on a condition that is expected to be impossible in production
#define FAIL_FAST(message) _bugcheck(BUGCHECK_FAILFAST, (uintptr_t)(message), (uintptr_t)(__LINE__), (uintptr_t)(__FILE__))

static inline void _assert(uintptr_t expression, int line, char* file)
{
    if (!expression) {
        _bugcheck(BUGCHECK_ASSERT, expression, (uintptr_t)line, (uintptr_t)file);
    }
}

static inline void _assert_eq(uintptr_t static_expression, uintptr_t check_expression, int line, char* file)
{
    if (static_expression != check_expression)
    {
        _bugcheck(BUGCHECK_ASSERT_EQ, check_expression, (uintptr_t)line, (uintptr_t)file);
    }
}

static inline void _assert_neq(uintptr_t static_expression, uintptr_t check_expression, int line, char* file)
{
    if (static_expression != check_expression)
    {
        _bugcheck(BUGCHECK_ASSERT_NEQ, check_expression, (uintptr_t)line, (uintptr_t)file);
    }
}

static inline void _assert_tx_success(int result, int line, char* file)
{
    if (result != 0) {
        _bugcheck(BUGCHECK_ASSERT_TX_SUCCESS, result, (uintptr_t)line, (uintptr_t)file);
    }
}

// Assert that an expression evaluates to TRUE
#define ASSERT(e) _assert((uintptr_t)(e), __LINE__, __FILE__)

// Assert that an expression is equal to a value
// The results of the second expression's evaluation will be reported in the exception
// if the expressions do not evaluate to equal
#define ASSERT_EQ(static_expression, check_expression) _assert_eq((uintptr_t)(static_expression), (uintptr_t)(check_expression), __LINE__, __FILE__)

// Assert that an expression is not equal to a value
// The results of the second expression's evaluation will be reported in the exception
// if the expressions do not evaluate to equal
#define ASSERT_NEQ(static_expression, check_expression) _assert_neq((uintptr_t)(static_expression), (uintptr_t)(check_expression), __LINE__, __FILE__)

// Assert that an expression is TX_SUCCESS
#define ASSERT_TX_SUCCESS(result) _assert_tx_success((int)result, __LINE__, __FILE__)
