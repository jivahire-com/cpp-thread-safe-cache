//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump.c
 *  Crash dump handler.
 */

/*--------------- Includes ---------------*/
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <crash_dump.h> // for crash_dump_handler
#include <stdint.h>     // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*------------- Functions ----------------*/
void crash_dump_handler(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    // ToDo: Implement crash dump handler
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
}
