//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_icc.h
 * Crash dump internal APIs for ICC (Inter Core Communication).
 */
#pragma once

/*---------- Nested Includes -------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
/**
 * @brief Requests other cores and die to perform a crash dump.
 * 
 */
void crash_dump_remote_trigger();

/**
 * @brief Transfers full crash dump to BMC.
 * 
 */
void crash_dump_transfer_full_dump_to_bmc();