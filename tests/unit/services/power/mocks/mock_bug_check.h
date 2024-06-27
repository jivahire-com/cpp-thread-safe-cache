//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file bugcheck_mock.h
 * Header for bugcheck mock
 */

#pragma once

/*----------- Nested includes ------------*/

#include <setjmp.h>
#include <stdnoreturn.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define NORETURN        noreturn
#define BUG_CHECK_RETURN_VALUE 0x1

extern jmp_buf cd_mock_jump_buf;

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
*   Mocks CrashDumpBugCheck
*
*   @return
*      None (does not return)
*/
noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4);

/**
*   Handles noreturn behaviour of CrashDumpBugCheck
*
*   @return
*      0, saves the calling environment for later use
*      1, indictes return to the original calling environment
*/
#define BUGCHECK_MOCK_RETURN (setjmp(cd_mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN