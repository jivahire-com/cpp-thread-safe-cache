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

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static startup_shutdown_boot_stage_t scp_boot_stages[] = {
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_MCP_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_SDM_ITCM_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_SDM_DTCM_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_CDED_ITCM_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_CDED_DTCM_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_KMP_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_CLUSTER_CORE_INIT, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_AP_SOC_POWER_INIT, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_AP_SOC_POWER_INIT_POST_SYNC, DEFAULT_SOS_TIMEOUT_MS, false, true},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_BL31_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_STMM_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_BL33_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_HAFNIUM_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_RMM_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_SPMC_LOAD, DEFAULT_SOS_TIMEOUT_MS, false, false},
    {STARTUP_PHASE_AP_ASYNC, STARTUP_PRIMARY_AP_CORE_BOOT, DEFAULT_SOS_TIMEOUT_MS, false, true}};

static startup_shutdown_shutdown_stage_t scp_shutdown_stages[] = {{SHUTDOWN, DEFAULT_SOS_TIMEOUT_MS, false},
                                                                  {COLD_RESET, DEFAULT_SOS_TIMEOUT_MS, false},
                                                                  {MSCP_SUBSYS_RESET, DEFAULT_SOS_TIMEOUT_MS, false},
                                                                  {AP_WARM_RESET, DEFAULT_SOS_TIMEOUT_MS, false}};

/*------------- Functions ----------------*/

unsigned sos_core_boot_stage_count()
{
    return FPFW_ARRAY_SIZE(scp_boot_stages);
}

unsigned sos_core_shutdown_stage_count()
{
    return FPFW_ARRAY_SIZE(scp_shutdown_stages);
}

const startup_shutdown_boot_stage_t* sos_core_boot_stages()
{
    return scp_boot_stages;
}

const startup_shutdown_shutdown_stage_t* sos_core_shutdown_stages()
{
    return scp_shutdown_stages;
}

void sos_core_shutdown_handler(ssi_shutdown_type_t shutdown_type)
{
    // This is where the SCP would handle the shutdown request after all registered ssi interfaces have been
    // notified and responded. TO DO : Task 2066715 - Inform HSP of SCP shutdown completion

    // For now, we just print a message
    SOS_LOG_INFO("Received shutdown request: %d", (int)shutdown_type);
}

void sos_boot_timeout_override(sos_stage_timeout_t timeout)
{
    for (unsigned int idx = 0; idx < sos_core_boot_stage_count(); idx++)
    {
        if (scp_boot_stages[idx].stage == timeout.operation_stage.boot)
        {
            scp_boot_stages[idx].timeout_ms = timeout.timeout_ms;
            break;
        }
    }
}

uint32_t sos_boot_timeout(sos_stage_timeout_t current_stage)
{
    for (unsigned int idx = 0; idx < sos_core_boot_stage_count(); idx++)
    {
        if (scp_boot_stages[idx].stage == current_stage.operation_stage.boot)
        {
            return scp_boot_stages[idx].timeout_ms;
        }
    }

    return DEFAULT_SOS_TIMEOUT_MS;
}

void sos_shutdown_timeout_override(sos_stage_timeout_t timeout)
{
    for (unsigned int idx = 0; idx < sos_core_shutdown_stage_count(); idx++)
    {
        if (scp_shutdown_stages[idx].stage == timeout.operation_stage.shutdown)
        {
            scp_shutdown_stages[idx].timeout_ms = timeout.timeout_ms;
            break;
        }
    }
}

uint32_t sos_shutdown_timeout(sos_stage_timeout_t current_stage)
{
    for (unsigned int idx = 0; idx < sos_core_shutdown_stage_count(); idx++)
    {
        if (scp_shutdown_stages[idx].stage == current_stage.operation_stage.shutdown)
        {
            return scp_shutdown_stages[idx].timeout_ms;
        }
    }

    return DEFAULT_SOS_TIMEOUT_MS;
}

void sos_core_override_timeout(pstartup_reset_timeout_request_t request)
{
    if (request->timeout.stage_category == BOOT_STAGE)
    {
        sos_boot_timeout_override(request->timeout);
    }
    else if (request->timeout.stage_category == SHUTDOWN_STAGE)
    {
        sos_shutdown_timeout_override(request->timeout);
    }
    else
    {
        SOS_LOG_CRIT("Invalid stage type %d", request->timeout.stage_category);
        FPFW_RUNTIME_ASSERT(false);
    }
}