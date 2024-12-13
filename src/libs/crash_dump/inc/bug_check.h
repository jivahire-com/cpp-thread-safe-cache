//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file bug_check.h
 * Public interface for creating a bug check which generates a crash dump.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <debug.h>
#include <inttypes.h>
#include <kng_error.h>
#include <crash_dump.h>

/*-- Symbolic Constant Macros (defines) --*/
/**
 * The BUG_CHECK() macro will hang the software and generate a crash dump.
 * The file name and line number of the crash are logged with the bug check
 * information.
 *
 * Interupts are disabled to prevent a thread context switch or ISR
 * from running during the bug check and changing the state of the
 * system prior to the crash.
 *
 *  @param errorCode
 *      Error code, should be defined in kng_error.h
 *
 * @param p1, p2
 *      User parameters included with the bug check information.
 */
#define BUG_CHECK(errorCode, p1, p2) \
    DBGPRINT("Bug Check: [%"PRId32"] File [%s:%d] P1: [%"PRIu32"] P2: [%"PRIu32"]\n", (int32_t)(errorCode), __FILE__, __LINE__, (uint32_t)(p1), (uint32_t)(p2)); \
    crash_dump_bug_check((errorCode), (uint32_t)__FILE__, __LINE__, (uint32_t)(p1), (uint32_t)(p2))

/**
 * The BUG_CHECK_EXTERNAL() macro will hang the software and
 * generate a crash dump by external core signal.
 * 
 */
#define BUG_CHECK_EXTERNAL()    crash_dump_bug_check_external();

/**
 * The BUG_ASSERT() macro will generate a bug check if the condition parameter
 * evaluates to false.
 *
 *  @param cond
 *      Condition parameter. If this parameter is false, then a bug check is
 *      generated.
 */
#define BUG_ASSERT(cond) \
    if (!(cond)) { \
        BUG_CHECK(KNG_BGCHK_BUGCHECK, 0, 0); \
    }

/**
 * The BUG_ASSERT_KNG_STATUS() macro will generate a bug check if the status parameter
 * is a SEVERITY_ERROR status.
 *
 *  @param status
 *      KNG_STATUS parameter. If the KNG_STATUS is a SEVERITY_ERROR status, then a bug check is
 *      generated, with the KNG_STATUS set to bug check parameter 3.
 */
#define BUG_ASSERT_KNG_STATUS(status) \
    if (KNG_FAILED(status)) { \
        BUG_CHECK(KNG_BGCHK_BUGCHECK, status, 0); \
    }

/**
 * The BUG_ASSERT_PARAM() macro will generate a bug check if the condition parameter
 * evaluates to false. Param1 and param2 will be forwarded to the BUG_CHECK macro.
 *
 *  @param cond
 *      Condition parameter. If this parameter is false, then a bug check is
 *      generated.
 *
 *  @param param1
 *      parameter 1 for bugcheck
 *
 *  @param param2
 *      parameter 2 for bugcheck
 */
#define BUG_ASSERT_PARAM(cond, param1, param2) \
    if (!(cond)) { \
        BUG_CHECK(KNG_BGCHK_BUGCHECK, param1, param2); \
    }

/**
 * NOTE: The ASSERT() macro is only defined in debug builds.
 *
 * The ASSERT() macro will generate a bug check if the condition parameter
 * evaluates to false.
 *
 *  @param cond
 *      Condition parameter. If this parameter is false, then a bug check is
 *      generated.
 */
#ifndef NDEBUG
    #ifndef ASSERT
        #define ASSERT(cond) \
            if (!(cond)) { \
                BUG_CHECK(KNG_BGCHK_ASSERT, 0, 0); \
            }
    #endif
#else
    #ifndef ASSERT
        #define ASSERT(cond) (void)(cond)
    #endif
#endif

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
