//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_pldm.h
 * APIs for transferring CD through PLDM
 */

/*----------- Nested Includes ------------*/
#include <fpfw_pldm_service.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
/**
 * Transfers dump to BMC via PLDM
 *
 * @return None
 */
void crash_dump_pldm_transfer_dump();

/**
 * Returns whether dump transfer has completed
 *
 * @return bool
 */
bool crash_dump_pldm_transfer_completed();

/**
 * @brief Callback for PLDM transfer events
 *
 * @param ctx Context pointer passed during transfer initiation
 * @param dest Destination address for the transfer
 * @param offset Offset within the destination
 * @param numBytes Number of bytes to transfer
 */
void crash_dump_transfer_dump_platform_event_cb(void* ctx, void* dest, size_t offset, size_t numBytes);

/**
 * Callback for PLDM transfer completion
 *
 * @param completionCode Completion code from PLDM transfer
 * @param ctx Context pointer passed during transfer initiation
 */
void crash_dump_pldm_on_ppe_complete(fpfw_pldm_cc_t completionCode, void* ctx);