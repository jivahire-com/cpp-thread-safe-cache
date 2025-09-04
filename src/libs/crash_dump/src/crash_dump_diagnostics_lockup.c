//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_diagnostics_lockup.c
 * Crash Dump Diagnostics Lockup functions
 */

/*------------- Includes -----------------*/
#include "crash_dump_context.h"

#include <FpFwCli.h> // for FpFwCliPrint
#include <crash_dump_memory.h>
#include <tx_api.h> // for tx_thread_sleep

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void lockup_invalid_opcode_exception(void);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief Usage fault handler function
 *
 * @param none
 */
void lockup_usage_fault_handler(void)
{
    FpFwCliPrint("Lockup Usage fault handler\n");

    lockup_invalid_opcode_exception();
}

/**
 * @brief Hard fault handler function
 *
 * @param none
 */
void lockup_hardfault_handler(void)
{
    FpFwCliPrint("Lockup Hard fault handler\n");

    lockup_invalid_opcode_exception();
}

void lockup_invalid_opcode_exception(void)
{
    // ThreadX thread sleep to ensure buffer flush completion
    tx_thread_sleep(1);
#ifndef _WIN32
    // Trigger an exception by executing an invalid opcode
    asm volatile("UDF #1");
#endif
}