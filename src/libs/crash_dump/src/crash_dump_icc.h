//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_icc.h
 * Crash dump internal APIs for ICC (Inter Core Communication).
 */
#pragma once
#include <stdbool.h> // for bool
#include <stdint.h>  // for uint32_t

/*---------- Nested Includes -------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
/**
 * @brief Requests other cores and die to perform a crash dump.
 * 
 * @param is_ue If true, indicates that this is a UE (Uncorrected Error) crash dump trigger.
 */
void crash_dump_remote_trigger(bool is_ue);

/**
 * @brief Requests HSP to perform a warm reset.
 * 
 */
void crash_dump_request_hsp_warm_reset();

/**
 * @brief Requests MCP0 to transfer crash dump to BMC.
 * 
 * @return Status of the request.
 */
uint32_t crash_dump_request_transfer_dump();