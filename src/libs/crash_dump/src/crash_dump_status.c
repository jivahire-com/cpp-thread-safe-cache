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
#include <stdint.h>     // for uint32_t, uint64_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
crash_dump_status_t* GetCrashDumpStatus()
{
    crash_dump_config_t* config = GetCrashDumpConfig();
    return config ? config->cd_status : NULL;
}

void crash_dump_update_state(crash_dump_state_t state)
{
    crash_dump_status_t* cd_status = GetCrashDumpStatus();

    if (cd_status != NULL)
    {
        FPFwSpinLockAcquire(&cd_status->lock);
        cd_status->cd_status = (uint16_t)state;
        FPFwSpinLockRelease(&cd_status->lock);
    }
}

void crash_dump_update_core_state(crash_dump_core_state_t state)
{
    crash_dump_config_t* config = GetCrashDumpConfig();
    crash_dump_status_t* cd_status = GetCrashDumpStatus();

    if (cd_status != NULL)
    {
        FPFwSpinLockAcquire(&cd_status->lock);
        cd_status->cores[config->die_index * CRASH_DUMP_CORE_NUM + config->core_index] = (uint8_t)state;
        FPFwSpinLockRelease(&cd_status->lock);
    }
}