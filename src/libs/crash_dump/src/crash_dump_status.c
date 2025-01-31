//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_status.c
 * Crash dump status management functions
 */

/*------------- Includes -----------------*/
#include "crash_dump_status.h"

#include <crash_dump.h> // for GetCrashDumpConfig
#include <idsw_kng.h>   // for DIE_0, DIE_1
#include <stdint.h>     // for uint32_t, uint64_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void initialize_crash_dump_header_lock(crash_dump_config_t* config)
{
    // ToDo: Remove this when HSP initialize HW semaphore.
    if (config->die_index == DIE_0 && config->core_index == CRASH_DUMP_CORE_SCP)
    {
        initialize_semaphore(config->cd_semaphore.semaphore_id);
    }
}

crash_dump_status_t* GetCrashDumpStatus(crash_dump_config_t** ppConfig)
{
    crash_dump_config_t* config = GetCrashDumpConfig();

    if (ppConfig != NULL)
    {
        *ppConfig = config;
    }

    return config ? config->cd_status : NULL;
}

void crash_dump_update_state(crash_dump_state_t state)
{
    crash_dump_config_t* config = NULL;
    crash_dump_status_t* cd_status = GetCrashDumpStatus(&config);

    if (cd_status != NULL)
    {
        wait_for_semaphore(config->cd_semaphore.semaphore_id, config->cd_semaphore.semaphore_key);
        cd_status->cd_status = (uint16_t)state;
        release_semaphore(config->cd_semaphore.semaphore_id);
    }
}

void crash_dump_update_core_state(crash_dump_core_state_t state)
{
    crash_dump_config_t* config = NULL;
    crash_dump_status_t* cd_status = GetCrashDumpStatus(&config);

    if (cd_status != NULL)
    {
        wait_for_semaphore(config->cd_semaphore.semaphore_id, config->cd_semaphore.semaphore_key);
        cd_status->cores[config->die_index * CRASH_DUMP_CORE_NUM + config->core_index] = (uint8_t)state;
        release_semaphore(config->cd_semaphore.semaphore_id);
    }
}