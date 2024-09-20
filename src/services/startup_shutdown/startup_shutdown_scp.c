//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_scp.c
 * Implements the SCP-specific portion of the startup/shutdown service.
 */

/*------------- Includes -----------------*/

#include "startup_shutdown.h"
#include "startup_shutdown_i.h"
#include "startup_shutdown_init.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <debug.h>
#include <fpfw_init.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DEFAULT_TIMEOUT_MS 120000
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static const startup_shutdown_boot_stage_t scp_boot_stages[] = {
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_MCP_LOAD, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_KMP_LOAD, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_CLUSTER_CORE_INIT, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_AP_SOC_POWER_INIT, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_BL31_LOAD, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_STMM_LOAD, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_BL33_LOAD, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_HAFNIUM_LOAD, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_RMM_LOAD, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_SPMC_LOAD, DEFAULT_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_PRIMARY_AP_CORE_BOOT, DEFAULT_TIMEOUT_MS, false, true}};

/*------------- Functions ----------------*/

unsigned sos_core_boot_stage_count()
{
    return FPFW_ARRAY_SIZE(scp_boot_stages);
}

const startup_shutdown_boot_stage_t* sos_core_boot_stages()
{
    return scp_boot_stages;
}

void sos_core_shutdown_handler(ssi_shutdown_type_t shutdown_type)
{
    // This function is called when the system is requested to shutdown
    // This is where the SCP would handle the shutdown request after all registered ssi interfaces have been notified and responded.

    // For now, we just print a message
    SOS_LOG_INFO("Received shutdown request: %d", (int)shutdown_type);
}