//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file stdio_textio_mocks.c
 * Mocks for the stdio textio library
 */

/*------------- Includes -----------------*/

#include "DfwkPtrTypes.h" // for PDFWK_INTERFACE_HEADER

#include <FpFwCMocka.h>   // for function_called
#include <FpFwUtils.h>    // for FPFW_UNUSED
#include <stdio_textio.h> // for stdio_textio_init

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void stdio_textio_init(PDFWK_INTERFACE_HEADER textio_interface)
{
    FPFW_UNUSED(textio_interface);
    function_called();
}