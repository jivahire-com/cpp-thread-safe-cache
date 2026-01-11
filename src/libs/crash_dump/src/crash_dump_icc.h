//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_icc.h
 * Crash dump internal APIs for ICC (Inter Core Communication).
 */
#pragma once
#include <fpfw_status.h> // for fpfw_status_t
#include <stdbool.h>     // for bool
#include <stdint.h>      // for uint32_t

/*---------- Nested Includes -------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
/**
 * @brief Requests other cores and die to perform a crash dump.
 * 
 * @param is_ue If true, indicates that this is a UE (Uncorrected Error) crash dump trigger.
 * @param origin_core The core id of first crash core
 */
void crash_dump_remote_trigger(bool is_ue, uint32_t origin_core);

/**
 * @brief Notifies the HSP about crash dump events.
 * 
 */
fpfw_status_t crash_dump_notify_hsp_transfer_complete(uint32_t flags);

/**
 * @brief Requests MCP0 to transfer crash dump to BMC.
 * 
 * @return Status of the request.
 */
uint32_t crash_dump_request_transfer_dump();
