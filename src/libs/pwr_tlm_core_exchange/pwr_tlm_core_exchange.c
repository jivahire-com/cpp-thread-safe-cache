//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_core_exchange.c
 * Guarded data object for data exchange between SCP and MCP.
 */

/*------------- Includes -----------------*/
#include "pwr_tlm_core_exchange.h"

#include <FpFwUtils.h>
#include <assert.h>
#include <mscp_exp_rmss_memory_map.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

typedef struct
{
    uint64_t droop_counts[NUM_AP_CORES_PER_DIE];

} pwr_tlm_core_exch_24hr_scp_to_mcp_t;

typedef struct
{

} pwr_tlm_core_exch_24hr_mcp_to_scp_t;

typedef struct
{
    pwr_tlm_core_exch_24hr_scp_to_mcp_t scp_to_mcp_24hr;
    pwr_tlm_core_exch_24hr_mcp_to_scp_t mcp_to_scp_24hr;

} pwr_tlm_core_exch_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#ifdef PWR_TLM_EXCH_TEST_OVERRIDE

    #undef SCP_EXP_PWR_TLM_SCP_MCP_BASE
// static uint8_t test_array[SCP_EXP_PWR_TLM_SCP_MCP_SIZE];
    #define SCP_EXP_PWR_TLM_SCP_MCP_BASE (test_array)
#endif

// static pwr_tlm_core_exch_t* const s_pwr_tlm_core_exch = (pwr_tlm_core_exch_t* const)SCP_EXP_PWR_TLM_SCP_MCP_BASE;

/*------------- Functions ----------------*/

static_assert(sizeof(pwr_tlm_core_exch_t) <= SCP_EXP_PWR_TLM_SCP_MCP_SIZE,
              "pwr_tlm_core_exch_t size exceeds reserved memory region");

// TODO:  stubbing out the interface for now
// https://azurecsi.visualstudio.com/Dev/_workitems/edit/2585203
// This is in a cacheable region so all of the api's will need to handle the appropriate cache flushes or invalidations

void pwr_tlm_core_exch_scp_write_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE])
{
    FPFW_UNUSED(droop_count_array);
    // memcpy(s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts,
    //        droop_count_array,
    //        sizeof(s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts));
}

void pwr_tlm_core_exch_mcp_read_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE])
{
    FPFW_UNUSED(droop_count_array);
    // memcpy(droop_count_array,
    //        s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts,
    //        sizeof(s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts));

    memset(droop_count_array, 0, sizeof(*droop_count_array));
}