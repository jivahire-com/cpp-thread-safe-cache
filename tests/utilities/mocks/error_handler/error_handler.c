//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file error_handler.c
 *   Public functions for error handler mock.
 */

/*------------- Includes -----------------*/

#include <ErrorHandler.h>  // for FPFwErrorRaise
#include <FpFwCMocka.h>    // for check_expected
#include <error_handler.h> // for set_error_handler_return
#include <setjmp.h>        // for longjmp, jmp_buf, setjmp
#include <stdbool.h>       // for bool
#include <stdint.h>        // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static jmp_buf jump_buf;

/*------------- Functions ----------------*/

void FPFwErrorRaise(uint32_t error, ...)
{
    check_expected(error);

    // Handle noreturn, allowing control to return to test
    longjmp(jump_buf, 1);
}

bool set_error_handler_return()
{
    return setjmp(jump_buf);
}
