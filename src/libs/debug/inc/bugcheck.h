//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file bugcheck.h
 * Public header file for bugchecker
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <inttypes.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
#define BUGCHECK_FAILFAST    0
#define BUGCHECK_ASSERT      1
#define BUGCHECK_ASSERT_EQ   2
#define BUGCHECK_ASSERT_NEQ  3
#define BUGCHECK_ASSERT_TX_SUCCESS  4

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
void _bugcheck(uint32_t bugcheck_code, uint32_t arg0, uint32_t arg1, uint32_t arg2) __attribute((noreturn));
