//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file stdio_textio_mocks.c
 * Mocks for the stdio textio library
 */

/*------------- Includes -----------------*/

#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <stdio_textio.h>

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