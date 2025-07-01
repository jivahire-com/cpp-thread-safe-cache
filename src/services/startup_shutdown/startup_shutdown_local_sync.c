//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_local_sync.c
 * Implements the startup or shutdown local die sync for boot stages.
 */

/*------------- Includes -----------------*/
#include "startup_shutdown_events_i.h"
#include "startup_shutdown_i.h"

#include <IFpFwEventTracingGeneration.h>
#include <arm_intrinsic.h>
#include <cmsis_m7.h>
#include <hw_semaphore.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <mscp_exp_rmss_memory_map.h>
#include <semaphore_lib.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

static_assert(sizeof(uint32_t) * 2 >= (sizeof(ssi_startup_stage_t) * 2),
              "Size of ssi_startup_stage_t must be less than or equal to 2 * sizeof(uint32_t).");
static_assert(SCP_EXP_MCP_SCP_STARTUP_SHUTDOWN_SYNC_SIZE >= (sizeof(ssi_startup_stage_t) * 2),
              "SCP_EXP_MCP_SCP_STARTUP_SHUTDOWN_SYNC_SIZE must be equal to sizeof(ssi_startup_stage_t)*2.");

#define RMSS_EXP_RAM_SYNC_ADDR_OFFSET(cpu_type) ((cpu_type) == CPU_SCP ? 0x0 : sizeof(uint32_t))
#define RMSS_EXP_RAM_SYNC_ADDR(cpu_type) \
    (SCP_EXP_MCP_SCP_STARTUP_SHUTDOWN_SYNC_BASE + RMSS_EXP_RAM_SYNC_ADDR_OFFSET((cpu_type)))

/**
 * Override the memory addresses for testing purposes.
 */
#ifdef START_SHUTDOWN_TEST_OVERRIDE
    #undef RMSS_EXP_RAM_SYNC_ADDR
uint32_t test_memory[2] = {0}; // Mock memory for testing
    #define RMSS_EXP_RAM_SYNC_ADDR(cpu_type) ((intptr_t) & test_memory[(cpu_type) == CPU_SCP ? 0 : 1])
#endif

#define STARTUP_SHUTDOWN_LOCAL_SYNC_SEMAPHORE_KEY(cpu_type) \
    ((((cpu_type) << 16) | (0x5353 & 0xFFFF))) // Ascii of 'S''S' (Startup/Shutdown)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

bool wait_for_local_core_boot_stage(startup_shutdown_boot_stage_t current_boot_stage)
{

    if (!current_boot_stage.local_core_sync_required)
    {
        return true; // No local core sync required, so we can return immediately
    }

    FPFW_ET_LOG(LocalCoreSyncStageStart, current_boot_stage.stage);

    // We want to:
    // 1. Update the exp ram with local core's stage
    // 2. Read the exp ram for the remote core's stage
    // 3. If the remote core's stage matches the current stage, we can proceed, otherwise go back to step 2
    // 4. Return success on local core sync

    uint32_t semaphore_key = STARTUP_SHUTDOWN_LOCAL_SYNC_SEMAPHORE_KEY(CPU_SCP);

    uintptr_t rmss_exp_ram_sync_local_addr = RMSS_EXP_RAM_SYNC_ADDR(CPU_SCP);
    uintptr_t rmss_exp_ram_sync_remote_addr = RMSS_EXP_RAM_SYNC_ADDR(CPU_MCP);

    // Swap the above if on MCP
    if (idsw_get_cpu_type() == CPU_MCP)
    {
        semaphore_key = STARTUP_SHUTDOWN_LOCAL_SYNC_SEMAPHORE_KEY(CPU_MCP);

        rmss_exp_ram_sync_local_addr = RMSS_EXP_RAM_SYNC_ADDR(CPU_MCP);
        rmss_exp_ram_sync_remote_addr = RMSS_EXP_RAM_SYNC_ADDR(CPU_SCP);
    }

    // Acquire the semaphore and update the local core's stage
    wait_for_semaphore(STARTUP_SHUTDOWN_RMSS_EXP_SEMAPHORE_ID, semaphore_key);
    *((volatile uint32_t*)rmss_exp_ram_sync_local_addr) = (uint32_t)current_boot_stage.stage;
    __DSB();
    release_semaphore(STARTUP_SHUTDOWN_RMSS_EXP_SEMAPHORE_ID);

    // Wait for the remote core to update its stage
    uint32_t iteration = 0;
    while (true)
    {
        // Acquire the semaphore to read the remote core's stage
        wait_for_semaphore(STARTUP_SHUTDOWN_RMSS_EXP_SEMAPHORE_ID, semaphore_key);
        ssi_startup_stage_t remote_stage = *((volatile uint32_t*)rmss_exp_ram_sync_remote_addr);
        __DSB();
        release_semaphore(STARTUP_SHUTDOWN_RMSS_EXP_SEMAPHORE_ID);

        FPFW_ET_LOG(LocalCoreSyncStageRemoteRead, current_boot_stage.stage, remote_stage, iteration);

        // Check if the remote core's stage matches the current stage
        if (remote_stage == current_boot_stage.stage)
        {
            break;
        }
        iteration++;

        // This path is called from the Startup/Shutdown thread, and we want to
        // ensure that we can:
        //   1. Not complete this stage until the local core has reached same stage
        //   2. Allow other tasks to run while waiting for the remote core to reach that stage
        // So we sleep for a short duration to yield the CPU and allow other tasks to run
        tx_thread_sleep(1);
    }

    FPFW_ET_LOG(LocalCoreSyncStageRemoteEnd, current_boot_stage.stage, iteration);

    return true;
}
