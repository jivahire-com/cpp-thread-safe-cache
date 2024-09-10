//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_remote_sync.c
 * Implements the startup or shutdown thread for SSI notifications
 */

/*------------- Includes -----------------*/
#include "startup_shutdown_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DIE_SYNC_EVENT_NAME "DIE_SYNC_EVENT"
#define MS_TO_TX_TICKS(ms)  (((ms)*TX_TIMER_TICKS_PER_SECOND) / 1000)
#define DIE0_MASK           0x80000000
#define DIE1_MASK           0x40000000
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static TX_EVENT_FLAGS_GROUP die_sync_flags = {0};

/*------------- Functions ----------------*/
void remote_die_sync_init()
{
    //  ICC Init will be done once ICC DIE2DIE is available
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1821528/

    int status = tx_event_flags_create(&die_sync_flags, DIE_SYNC_EVENT_NAME);
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);
}

void send_current_boot_stage_to_remote_die(startup_shutdown_boot_stage_t current_boot_stage)
{
    int status;
    ULONG event_mask = (idsw_get_die_id() == DIE_0) ? DIE1_MASK : DIE0_MASK;

    // Implement will be done once ICC DIE2DIE is available
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1821528/

    // for now, signal the event so that wait routine can proceed
    status = tx_event_flags_set(&die_sync_flags, (event_mask | current_boot_stage.stage), TX_OR);
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);
}

UINT wait_for_remote_die_boot_stage_to_be(startup_shutdown_boot_stage_t boot_stage)
{
    // Implement will be done once ICC DIE2DIE is available
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1821528/

    // Wait for the event to be set by the ICC receive callback, timeout if necessary
    ULONG flags = 0;
    ULONG event_mask = (idsw_get_die_id() == DIE_0) ? DIE1_MASK : DIE0_MASK;

    return tx_event_flags_get(&die_sync_flags,
                              (ULONG)(event_mask | boot_stage.stage),
                              TX_AND_CLEAR,
                              &flags,
                              MS_TO_TX_TICKS(boot_stage.timeout_ms));
}

bool wait_for_remote_die_boot_stage(startup_shutdown_boot_stage_t current_boot_stage, startup_shutdown_boot_stage_t desired_remote_boot)
{
    UINT status = TX_SUCCESS;

    if (idhw_is_single_die_boot_en() == false)
    {
        if (current_boot_stage.remote_die_sync_required)
        {
            send_current_boot_stage_to_remote_die(current_boot_stage);
            status = wait_for_remote_die_boot_stage_to_be(desired_remote_boot);

            if (status != TX_SUCCESS && status != TX_NO_EVENTS)
            {
                FPFW_RUNTIME_ASSERT(false);
            }
        }
    }

    return status == TX_SUCCESS;
}
