//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_core_exchange.h
 * This file contains the interface for the data exchange between SCP and MCP for power telemetry
 * This library is setup for single writer, single reader. Communication synchronization is done outside of this
 * library.  This lib only provides safe data access.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <stdint.h>



/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Write the droop counts for the specified core.
 *
 * @param[in] droop_count_array Pointer to the array of droop counts to write.
 */
void pwr_tlm_core_exch_scp_write_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE]);

/**
 * @brief Read the droop counts for the specified core.
 *
 * @param[out] droop_count_array destination pointer to array where droop counts will be written.
 */
void pwr_tlm_core_exch_mcp_read_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE]);