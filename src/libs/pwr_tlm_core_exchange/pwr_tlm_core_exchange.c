//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_core_exchange.c
 * Guarded data object for data exchange between SCP and MCP.
 */

/*------------- Includes -----------------*/
#include "pwr_tlm_core_exchange.h"

#include <assert.h>
#include <mscp_exp_rmss_memory_map.h>
#include <string.h>

#ifdef PWR_TLM_EXCH_TEST_OVERRIDE

    #define SCB_CleanDCache_by_Addr(x, y)
    #define SCB_InvalidateDCache_by_Addr(x, y)

#else
    #include <cmsis_compiler.h>
    #include <cmsis_m7.h>
    #include <m-profile/armv7m_cachel1.h>
#endif

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

typedef struct
{
    uint64_t counts[NUM_AP_CORES_PER_DIE];
    uint8_t sequence_number; // 0-255, increments on each write, rolls over
} pwr_tlm_core_exch_droop_counts_t;

typedef struct
{
    uint64_t mpam_mem_counters[NUMBER_OF_MEM_CONTROLLERS_PER_DIE][NUMBER_OF_MPAMS_PER_MEM_AND_UNTRACK_CTRLR]; // MPAM memory counters
    uint8_t sequence_number; // 0-255, increments on each write, rolls over
    uint8_t padding[3];
} pwr_tlm_core_exch_mpam_memory_counts_t;

typedef struct
{
    pwr_tlm_core_exch_droop_counts_t droop_counts;
} pwr_tlm_core_exch_24hr_scp_to_mcp_t;

typedef struct
{
    pwr_tlm_core_exch_24hr_scp_to_mcp_t scp_to_mcp_24hr;
    pwr_tlm_core_exch_mpam_memory_counts_t mpam_pmu_counts;
} pwr_tlm_core_exch_t;

/*-------- Function Prototypes -----------*/
static void zero_fortified_memory(void* ptr, size_t size);

/*-- Declarations (Statics and globals) --*/
#ifdef PWR_TLM_EXCH_TEST_OVERRIDE

    #undef SCP_EXP_PWR_TLM_SCP_MCP_BASE
static uint8_t test_array[SCP_EXP_PWR_TLM_SCP_MCP_SIZE];
    #define SCP_EXP_PWR_TLM_SCP_MCP_BASE (test_array)
#endif

static pwr_tlm_core_exch_t* const s_pwr_tlm_core_exch = (pwr_tlm_core_exch_t* const)SCP_EXP_PWR_TLM_SCP_MCP_BASE;

/*------------- Functions ----------------*/
static_assert(sizeof(pwr_tlm_core_exch_t) <= SCP_EXP_PWR_TLM_SCP_MCP_SIZE,
              "pwr_tlm_core_exch_t size exceeds reserved memory region");

static void zero_fortified_memory(void* ptr, size_t size)
{
    memset((uint8_t*)ptr, 0, size);
}

void pwr_tlm_core_exch_init(void)
{
    zero_fortified_memory(s_pwr_tlm_core_exch, sizeof(pwr_tlm_core_exch_t));

    // Clean (flush) the data cache line
    SCB_CleanDCache_by_Addr(s_pwr_tlm_core_exch, sizeof(pwr_tlm_core_exch_t));
}

void pwr_tlm_core_exch_scp_write_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE])
{
    memcpy(s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts.counts,
           droop_count_array,
           sizeof(s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts.counts));
    s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts.sequence_number++;

    // Clean (flush) the data cache line
    SCB_CleanDCache_by_Addr(&s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts, sizeof(pwr_tlm_core_exch_droop_counts_t));
}

uint8_t pwr_tlm_core_exch_mcp_read_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE])
{ // Invalidate the data cache line
    SCB_InvalidateDCache_by_Addr(&s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts,
                                 sizeof(pwr_tlm_core_exch_droop_counts_t));

    memcpy(droop_count_array,
           s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts.counts,
           sizeof(s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts.counts));

    return s_pwr_tlm_core_exch->scp_to_mcp_24hr.droop_counts.sequence_number;
}

void pwr_tlm_core_exch_scp_write_mpam_pmu_counts(uint64_t* mpam_pmu_count_array)
{
    memcpy(s_pwr_tlm_core_exch->mpam_pmu_counts.mpam_mem_counters,
           mpam_pmu_count_array,
           sizeof(s_pwr_tlm_core_exch->mpam_pmu_counts.mpam_mem_counters));

    // Clean (flush) the data cache line
    SCB_CleanDCache_by_Addr(&s_pwr_tlm_core_exch->mpam_pmu_counts, sizeof(pwr_tlm_core_exch_mpam_memory_counts_t));
}

uint8_t pwr_tlm_core_exch_mcp_read_mpam_pmu_counts(uint64_t* mpam_pmu_count_array)
{ // Invalidate the data cache line
    SCB_InvalidateDCache_by_Addr(&s_pwr_tlm_core_exch->mpam_pmu_counts, sizeof(pwr_tlm_core_exch_mpam_memory_counts_t));

    memcpy(mpam_pmu_count_array,
           s_pwr_tlm_core_exch->mpam_pmu_counts.mpam_mem_counters,
           sizeof(s_pwr_tlm_core_exch->mpam_pmu_counts.mpam_mem_counters));
    return s_pwr_tlm_core_exch->mpam_pmu_counts.sequence_number;
}