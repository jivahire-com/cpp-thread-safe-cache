//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_diagnostics_lockup.h
 * Crash Dump Diagnostics Lockup functions
 */
#pragma once

/*---------- Nested Includes -------------*/
#include <crash_dump.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Usage fault handler function
 *
 * @param none
 */
void lockup_usage_fault_handler(void);

/**
 * @brief Hard fault handler function
 *
 * @param none
 */
void lockup_hardfault_handler(void);

/**
 * @brief Invalid opcode exception function
 *
 * @param none
 */
void lockup_invalid_opcode_exception(void);
#ifdef __cplusplus
}
#endif
