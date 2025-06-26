//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_pldm.h
 * APIs for transferring CD through PLDM
 */

/*----------- Nested Includes ------------*/

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

/*-- Declarations (Statics and globals) --*/
