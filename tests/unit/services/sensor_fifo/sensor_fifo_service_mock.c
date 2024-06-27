//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_mock.c
 * Provide mock functions for sensor fifo driver tests
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h> // for PDFWK_INTERFACE_HEADER
#include <FpFwCMocka.h> // for check_expected, check_expected_ptr, mock_type
#include <stdint.h>     // for int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

int32_t __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER Interface)
{
    check_expected_ptr(Interface);
    return mock_type(int32_t);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}