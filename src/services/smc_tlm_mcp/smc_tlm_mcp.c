//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file smc_tlm_mcp.c
 * SMC TLM MCP request processing
 */

/*------------- Includes -----------------*/
#include "smc_tlm_mcp_events.h"

#include <DbgPrint.h>
#include <FpFwAssert.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <ddrss_reserved_regions.h>
#include <idsw_kng.h>
#include <kng_scp_tfa_shared.h>
#include <silibs_common.h>
#include <silibs_platform.h>
#include <smc_tlm_mcp.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SMC_TLM_NUM_CORES_PER_DIE 68 // Number of cores to monitor per die
#define SMC_TLM_INITIAL_POLL_INTERVAL_MS \
    (TX_TIMER_TICKS_PER_SECOND * 50) // 50s initial delay for ap core boot
#define SMC_TLM_POLL_INTERVAL_MS (FPFW_MAX((TX_TIMER_TICKS_PER_SECOND / (25UL)), (1UL))) // 250ms

/*------------- Typedefs -----------------*/
//! Structure for per-core_id SMC telemetry data
typedef struct per_core_smc_telemetry
{
    uint64_t smc_fid;         // SMC function ID
    uint64_t entry_timestamp; // Entry timestamp (offset 8)
    uint64_t exit_timestamp;  // Exit timestamp (offset 16)
} per_core_smc_telemetry_t;

typedef struct smc_telemetry_context
{
    uint32_t min_core_id;
    uint32_t max_core_id;
} smc_telemetry_context_t;

/*-------- Function Prototypes -----------*/
static void smc_tlm_timer_callback(ULONG input);

/*-- Declarations (Statics and globals) --*/
static TX_TIMER smc_tlm_timer;
static atu_map_entry_t smc_telemetry_buffer_atu_map_struct;

static smc_telemetry_context_t smc_tlm_context[2] = {
    {
        .min_core_id = 0,
        .max_core_id = SMC_TLM_NUM_CORES_PER_DIE - 1,
    },
    {
        .min_core_id = SMC_TLM_NUM_CORES_PER_DIE,
        .max_core_id = (2 * SMC_TLM_NUM_CORES_PER_DIE) - 1,
    },
};

/*-------- Public Functions -----------*/

/**
 * @brief Read SMC telemetry data from shared buffer for a given core_id
 * @param core_id Core ID to read telemetry for
 * @param entry_ts_us Pointer to store entry timestamp
 * @param exit_ts_us Pointer to store exit timestamp
 * @param smc_fid Pointer to store SMC function ID
 */
static void smc_tlm_buffer_read(uint32_t core_id, uint64_t* entry_ts_us, uint64_t* exit_ts_us, uint64_t* smc_fid)
{
    uint32_t idx = core_id * sizeof(per_core_smc_telemetry_t);
    *smc_fid = MMIO_READ64(smc_telemetry_buffer_atu_map_struct.mscp_start_address + idx);
    *entry_ts_us = MMIO_READ64(smc_telemetry_buffer_atu_map_struct.mscp_start_address + idx + 8);
    *exit_ts_us = MMIO_READ64(smc_telemetry_buffer_atu_map_struct.mscp_start_address + idx + 16);
}

/**
 * @brief Clear SMC telemetry data in shared buffer for a given core_id
 * @param core_id Core ID to clear telemetry for
 */
static void smc_tlm_buffer_clear(uint32_t core_id)
{
    uint32_t idx = core_id * sizeof(per_core_smc_telemetry_t);
    MMIO_WRITE64(smc_telemetry_buffer_atu_map_struct.mscp_start_address + idx, 0);
    MMIO_WRITE64(smc_telemetry_buffer_atu_map_struct.mscp_start_address + idx + 8, 0);
    MMIO_WRITE64(smc_telemetry_buffer_atu_map_struct.mscp_start_address + idx + 16, 0);
}

/**
 * @brief Timer callback that periodically reads shared buffer and emits event traces
 * @param input Unused parameter
 */
static void smc_tlm_timer_callback(ULONG input)
{
    volatile uint64_t entry_ts_us;
    volatile uint64_t exit_ts_us;
    volatile uint64_t smc_fid;

    smc_telemetry_context_t* context = (smc_telemetry_context_t*)input;

    // Read SMC telemetry for each core_id and emit event traces
    for (uint32_t core_id = context->min_core_id; core_id <= context->max_core_id; core_id++)
    {
        // Read telemetry data from shared buffer
        smc_tlm_buffer_read(core_id, (uint64_t*)&entry_ts_us, (uint64_t*)&exit_ts_us, (uint64_t*)&smc_fid);

        // Only process if we have valid smc fid and timestamps (exit must be after entry)
        if (smc_fid != 0 && entry_ts_us != 0 && exit_ts_us > entry_ts_us)
        {
            // Calculate time taken (in microseconds or whatever unit the timestamps use)
            uint64_t time_taken = exit_ts_us - entry_ts_us;

            // Emit event trace with SMC FID, core_id ID, and time taken
            AP_SMC_TELEMETRY_INFO(smc_fid, time_taken, core_id);

            // Clear timestamps after processing to avoid re-processing
            smc_tlm_buffer_clear(core_id);
        }
    }
}

/**
 * @brief Initialize SMC TLM MCP service
 */
void smc_tlm_mcp_init()
{
    unsigned int status;

    atu_entry_attr_t smc_telemetry_buffer = {ATU_BUS_ATTR_NS};

    smc_telemetry_buffer_atu_map_struct.ap_base_address = AP_SMC_TELEMETRY_BUFFER_BASE;
    smc_telemetry_buffer_atu_map_struct.mscp_start_address = 0;
    smc_telemetry_buffer_atu_map_struct.mscp_end_address = AP_SMC_TELEMETRY_BUFFER_SIZE - 1;
    smc_telemetry_buffer_atu_map_struct.attribute.as_uint32 = smc_telemetry_buffer.as_uint32;
    status = atu_map(ATU_ID_MSCP, &smc_telemetry_buffer_atu_map_struct);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

    FPFW_DBGPRINT_INFO("[AP_SMC_TLM_MCP] SMC Telemetry buffer mapped");

    if ((idsw_get_cpu_type() == CPU_MCP) && (idsw_get_die_id() == DIE_0))
    {
        // Create periodic timer for SMC telemetry collection
        status = tx_timer_create(&smc_tlm_timer,                   // timer_ptr
                                 "smc_tlm_timer",                  // name_ptr
                                 smc_tlm_timer_callback,           // expiration_function
                                 (ULONG)&smc_tlm_context[0],       // input
                                 SMC_TLM_INITIAL_POLL_INTERVAL_MS, // initial_ticks
                                 SMC_TLM_POLL_INTERVAL_MS,         // reschedule_ticks
                                 TX_AUTO_ACTIVATE                  // auto_activate
        );

        if (status != TX_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR(
                "[AP_SMC_TLM_MCP] Failed to create SMC telemetry timer for Die 0! Status: %d\n",
                status);
            BUG_ASSERT_PARAM(false, status, 0);
        }

        FPFW_DBGPRINT_INFO("[AP_SMC_TLM_MCP] SMC telemetry timer created for Die 0.\n");
    }
    else if ((idsw_get_cpu_type() == CPU_MCP) && (idsw_get_die_id() == DIE_1))
    {
        // Create periodic timer for SMC telemetry collection
        status = tx_timer_create(&smc_tlm_timer,                   // timer_ptr
                                 "smc_tlm_timer",                  // name_ptr
                                 smc_tlm_timer_callback,           // expiration_function
                                 (ULONG)&smc_tlm_context[1],       // input
                                 SMC_TLM_INITIAL_POLL_INTERVAL_MS, // initial_ticks
                                 SMC_TLM_POLL_INTERVAL_MS,         // reschedule_ticks
                                 TX_AUTO_ACTIVATE                  // auto_activate
        );

        if (status != TX_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR(
                "[AP_SMC_TLM_MCP] Failed to create SMC telemetry timer for Die 1! Status: %d\n",
                status);
            BUG_ASSERT_PARAM(false, status, 0);
        }
        FPFW_DBGPRINT_INFO("[AP_SMC_TLM_MCP] SMC telemetry timer created for Die 1.\n");
    }
}
