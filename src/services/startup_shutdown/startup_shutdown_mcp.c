//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_mcp.c
 * Implements the MCP-specific portion of the startup/shutdown service.
 */

/*------------- Includes -----------------*/

#include "startup_shutdown.h"
#include "startup_shutdown_i.h"
#include "startup_shutdown_init.h"

#include <FpFwUtils.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static startup_shutdown_boot_stage_t mcp_boot_stages[] = {
    // On the MCP this stage needs to sync with the die local SCP, but does not need to sync with the other
    // die MCP.
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_AP_SOC_POWER_INIT_POST_SYNC, DEFAULT_SOS_TIMEOUT_MS, true, false},
    {STARTUP_PHASE_MSCP_ASYNC, STARTUP_AP_SOC_POWER_INIT_POST_SYNC_SCF, DEFAULT_SOS_TIMEOUT_MS, true, false},
};

/*------------- Functions ----------------*/

unsigned sos_core_boot_stage_count()
{
    return FPFW_ARRAY_SIZE(mcp_boot_stages);
}

const startup_shutdown_boot_stage_t* sos_core_boot_stages()
{
    return mcp_boot_stages;
}

KNG_STATUS sos_core_shutdown_handler(ssi_shutdown_type_t shutdown_type)
{
    return sos_request_shutdown(shutdown_type);
}

void sos_boot_timeout_override(sos_stage_timeout_t* timeout)
{
    for (unsigned int idx = 0; idx < sos_core_boot_stage_count(); idx++)
    {
        if (mcp_boot_stages[idx].stage == timeout->operation_stage.boot)
        {
            mcp_boot_stages[idx].timeout_ms = timeout->timeout_ms;
            break;
        }
    }
}

uint32_t sos_boot_timeout(sos_stage_timeout_t* current_stage)
{
    for (unsigned int idx = 0; idx < sos_core_boot_stage_count(); idx++)
    {
        if (mcp_boot_stages[idx].stage == current_stage->operation_stage.boot)
        {
            return mcp_boot_stages[idx].timeout_ms;
        }
    }

    return DEFAULT_SOS_TIMEOUT_MS;
}

void sos_shutdown_timeout_override(sos_stage_timeout_t* timeout)
{
    // No shutdown stages defined for MCP. Leaving in if shutdown stages
    // are required in the future and to support common override path.
    FPFW_UNUSED(timeout);
}

uint32_t sos_shutdown_timeout(sos_stage_timeout_t* current_stage)
{
    // No shutdown stages defined for MCP. Leaving in if shutdown stages
    // are required in the future and to support common override path.
    FPFW_UNUSED(current_stage);

    return DEFAULT_SOS_TIMEOUT_MS;
}
