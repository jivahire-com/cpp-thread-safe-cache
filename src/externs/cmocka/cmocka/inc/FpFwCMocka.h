//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file FpFwCMocka.h
 * This file serves as wrapper to mocks
 */

#pragma once

/*----------- Nested includes ------------*/

#include <stdarg.h> // IWYU pragma: keep
#include <setjmp.h> // IWYU pragma: keep
#include <stddef.h> // IWYU pragma: keep
#include <cmocka.h> // IWYU pragma: export

/*-- Symbolic Constant Macros (defines) --*/

#ifndef FPFW_NORETURN
    // FPFW_NORETURN required for static analysis.
    #ifdef __has_attribute // Some GCC versions do not define __has_attribute (introduced in gcc version 5.x.x)
        // __has_attribute is defined, assume clang front end.
        #if __has_attribute(analyzer_noreturn)
            // Define FPFW_NORETURN to apply analyzer_noreturn attribute (we want to allow it to return for unit testing, but consider it noreturn for the analyzer)
            #define FPFW_NORETURN __attribute__((analyzer_noreturn))
        #elif __has_attribute(noreturn)
            // Define FPFW_NORETURN to apply noreturn attribute if compiler doesn't support analyzer_noreturn
            #define FPFW_NORETURN __attribute__((noreturn))
        #endif
    #endif

    #ifndef FPFW_NORETURN
        #define FPFW_NORETURN
    #endif
#endif

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
