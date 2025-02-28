//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_status.c
 * Crash dump status management functions
 */

/*------------- Includes -----------------*/
#include "crash_dump_status.h"

#include <crash_dump.h>  // for GetCrashDumpConfig
#include <idsw_kng.h>    // for DIE_0, DIE_1
#include <system_info.h> // for system_info_is_warm_start

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static bool crash_dump_get_is_single_dump_complete(crash_dump_type_context_t* type_context, uint16_t core_id)
{
    bool is_complete = true;

    wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
    if (type_context->header->status == CRASH_DUMP_IN_USE &&
        (type_context->header->cores[core_id] == CRASH_DUMP_STATE_READY ||
         type_context->header->cores[core_id] == CRASH_DUMP_STATE_IN_PROGRESS))
    {
        is_complete = false;
    }
    release_semaphore(type_context->semaphore.id);

    return is_complete;
}

static bool crash_dump_get_are_all_dump_complete(crash_dump_type_context_t* type_context)
{
    bool is_complete = true;

    wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
    if (type_context->header->status == CRASH_DUMP_IN_USE)
    {
        for (uint16_t i = 0; i < CRASH_DUMP_CORE_NUM * 2; i++)
        {
            if (type_context->header->cores[i] == CRASH_DUMP_STATE_READY || type_context->header->cores[i] == CRASH_DUMP_STATE_IN_PROGRESS)
            {
                is_complete = false;
                break;
            }
        }
    }
    release_semaphore(type_context->semaphore.id);

    return is_complete;
}

bool crash_dump_get_is_dump_complete(crash_dump_type_context_t* type_context)
{
    crash_dump_context_t* ctx = crash_dump_context();
    bool is_complete = true;

    if (type_context == NULL)
    {
        // Check all types of dumps
        for (uint16_t i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
        {
            if (ctx->type_ctx[i] != NULL)
            {
                is_complete = ctx->single_core_mode ? crash_dump_get_is_single_dump_complete(
                                                          ctx->type_ctx[i],
                                                          ctx->die_index * CRASH_DUMP_CORE_NUM + ctx->core_index)
                                                    : crash_dump_get_are_all_dump_complete(ctx->type_ctx[i]);
                if (!is_complete)
                {
                    break;
                }
            }
        }
    }
    else
    {
        is_complete =
            ctx->single_core_mode
                ? crash_dump_get_is_single_dump_complete(type_context, ctx->die_index * CRASH_DUMP_CORE_NUM + ctx->core_index)
                : crash_dump_get_are_all_dump_complete(type_context);
    }

    return is_complete;
}

void initialize_crash_dump_header(crash_dump_type_context_t* type_context)
{
    if (system_info_is_warm_start())
    {
        // Keep crash dump status if warm start.
        return;
    }

    crash_dump_context_t* ctx = crash_dump_context();

    if ((type_context->type == CRASH_DUMP_TYPE_MINI || ctx->die_index == DIE_0) && ctx->core_index == CRASH_DUMP_CORE_SCP)
    {
        // ToDo: Remove this when HSP initialize HW semaphore.
        initialize_semaphore(type_context->semaphore.id);

        // Initialize core status to CRASH_DUMP_STATE_NOT_AVAILABLE
        if (type_context->header != NULL)
        {
            wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
            for (uint16_t i = 0; i < CRASH_DUMP_CORE_NUM * 2; i++)
            {
                type_context->header->cores[i] = CRASH_DUMP_STATE_NOT_AVAILABLE;
            }
            release_semaphore(type_context->semaphore.id);
        }

        // Set this region is ready for crash dump.
        crash_dump_update_state(type_context, CRASH_DUMP_IN_USE);
    }
    else
    {
        // MCP or DIE_1 SCP for full dump wait until DIE_0 SCP initialize header.
        bool is_ready = false;

        // Polling until crash dump header is ready.
        while (!is_ready)
        {
            wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
            is_ready = type_context->header->status == CRASH_DUMP_IN_USE;
            release_semaphore(type_context->semaphore.id);
        }
    }

    // Set this core state to ready.
    crash_dump_update_core_state(type_context, CRASH_DUMP_STATE_READY);
}

void crash_dump_update_state(crash_dump_type_context_t* type_context, crash_dump_state_t state)
{
    if (type_context->header != NULL)
    {
        wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
        type_context->header->status = (uint16_t)state;
        release_semaphore(type_context->semaphore.id);
    }
}

void crash_dump_update_core_state(crash_dump_type_context_t* type_context, crash_dump_core_state_t state)
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (type_context->header != NULL)
    {
        wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
        type_context->header->cores[ctx->die_index * CRASH_DUMP_CORE_NUM + ctx->core_index] = (uint8_t)state;
        release_semaphore(type_context->semaphore.id);
    }
}