//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_status.c
 * Crash dump status management functions
 */

/*------------- Includes -----------------*/
#include "crash_dump_status.h"

#include "crash_dump_gpio.h"

#include <crash_dump.h>        // for GetCrashDumpConfig
#include <crash_dump_events.h> // for CRASH_DUMP_ET
#include <crash_dump_memory.h>
#include <idsw_kng.h> // for DIE_0, DIE_1
#include <modules/CdMemory.h>
#include <modules/dump_format.h>
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
    if (type_context->header->status == CRASH_DUMP_IN_USE && type_context->header->cores[core_id] == CRASH_DUMP_STATE_IN_PROGRESS)
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
            if (type_context->header->cores[i] == CRASH_DUMP_STATE_IN_PROGRESS)
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

bool crash_dump_get_is_dump_transferring(crash_dump_type_context_t* type_context)
{
    bool is_transferring = false;

    if (type_context == NULL)
    {
        // Check all types of dumps
        crash_dump_context_t* ctx = crash_dump_context();

        for (uint16_t i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
        {
            crash_dump_type_context_t* type_ctx = ctx->type_ctx[i];
            if (type_ctx != NULL)
            {
                wait_for_semaphore(type_ctx->semaphore.id, type_ctx->semaphore.key);
                is_transferring = (type_ctx->header->status == CRASH_DUMP_IN_TRANSFER);
                release_semaphore(type_ctx->semaphore.id);

                if (is_transferring)
                {
                    break;
                }
            }
        }
    }
    else
    {
        wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
        is_transferring = (type_context->header->status == CRASH_DUMP_IN_TRANSFER);
        release_semaphore(type_context->semaphore.id);
    }

    return is_transferring;
}

void initialize_crash_dump_header(crash_dump_type_context_t* type_context)
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (system_info_is_warm_start())
    {
        // Keep crash dump status if warm start.
        CRASH_DUMP_ET_WARNING(CRASH_DUMP_ET_TYPE_STATUS_WARM_START);

        // Check crash dump status and assert gpio if available.
        if (type_context->type == CRASH_DUMP_TYPE_FULL && type_context->header != NULL)
        {
            bool cd_available = false;

            wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);

            if (ctx->die_index == DIE_0 && ctx->core_index == CRASH_DUMP_CORE_SCP)
            {
                // Reset crashdump status.
                type_context->header->status = CRASH_DUMP_IN_USE;
            }

            for (uint16_t i = 0; i < CRASH_DUMP_CORE_NUM * 2; i++)
            {
                if (type_context->header->cores[i] == CRASH_DUMP_STATE_COMPLETED)
                {
                    cd_available = true;
                    break;
                }
            }

            if (type_context->header->cores[ctx->die_index * CRASH_DUMP_CORE_NUM + ctx->core_index] == CRASH_DUMP_STATE_IN_PROGRESS)
            {
                // If core was warm reset in In-progress state, set it to Ready state to recover.
                CRASH_DUMP_ET_WARNING_PARAM(CRASH_DUMP_ET_TYPE_IN_PROGRESS_RECOVERY,
                                            CRASH_DUMP_PROCESSOR_ID(ctx->die_index, ctx->core_index));

                type_context->header->cores[ctx->die_index * CRASH_DUMP_CORE_NUM + ctx->core_index] = CRASH_DUMP_STATE_READY;
                uint8_t* crash_dump_payload = CRASH_DUMP_CORE_ADDRESS(ctx->die_index, ctx->core_index);
                memset(crash_dump_payload, 0, CRASH_DUMP_FULL_SIZE_PER_CORE);
            }
            release_semaphore(type_context->semaphore.id);

            cd_gpio_assert_cd_available(cd_available);
        }
    }
    else
    {
        // Cold boot
        uint16_t my_core_id = ctx->die_index * CRASH_DUMP_CORE_NUM + ctx->core_index;

        if ((type_context->type == CRASH_DUMP_TYPE_MINI || ctx->die_index == DIE_0) && ctx->core_index == CRASH_DUMP_CORE_SCP)
        {
            // Initialize core status to CRASH_DUMP_STATE_NOT_AVAILABLE
            if (type_context->header != NULL)
            {
                wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);

                if (type_context->header->status == CRASH_DUMP_NOT_IN_USE)
                {
                    // Only update core states if CD status is not in use.
                    for (uint16_t i = 0; i < CRASH_DUMP_CORE_NUM * 2; i++)
                    {
                        type_context->header->cores[i] = CRASH_DUMP_STATE_NOT_AVAILABLE;
                    }

                    // Set this core state to ready.
                    type_context->header->cores[my_core_id] = CRASH_DUMP_STATE_READY;

                    // Set this region is ready for crash dump.
                    type_context->header->status = CRASH_DUMP_IN_USE;
                }
                else if (type_context->header->status == CRASH_DUMP_IN_TRANSFER)
                {
                    // If header status is IN_TRANSFER, it means crash dump transfer was interrupted.
                    // Reset status to IN_USE to allow new crash dump.
                    type_context->header->status = CRASH_DUMP_IN_USE;
                }
                release_semaphore(type_context->semaphore.id);
            }
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
                if (is_ready && type_context->header->cores[my_core_id] != CRASH_DUMP_STATE_COMPLETED)
                {
                    // Update my core state to ready if there is no dump in memory
                    type_context->header->cores[my_core_id] = CRASH_DUMP_STATE_READY;
                }
                release_semaphore(type_context->semaphore.id);
            }
        }
    }
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

crash_dump_state_t crash_dump_state(crash_dump_type_t type)
{
    crash_dump_context_t* ctx = crash_dump_context();
    crash_dump_state_t state = CRASH_DUMP_NOT_IN_USE;

    if (ctx != NULL && ctx->type_ctx[type] != NULL && ctx->type_ctx[type]->header != NULL)
    {
        wait_for_semaphore(ctx->type_ctx[type]->semaphore.id, ctx->type_ctx[type]->semaphore.key);
        state = (crash_dump_state_t)ctx->type_ctx[type]->header->status;
        release_semaphore(ctx->type_ctx[type]->semaphore.id);
    }

    return state;
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

crash_dump_core_state_t crash_dump_core_state(crash_dump_type_t type, uint32_t die_index, uint32_t core_index)
{
    crash_dump_context_t* ctx = crash_dump_context();
    crash_dump_core_state_t state = CRASH_DUMP_STATE_NOT_AVAILABLE;

    if (ctx != NULL && ctx->type_ctx[type] != NULL && ctx->type_ctx[type]->header != NULL)
    {
        wait_for_semaphore(ctx->type_ctx[type]->semaphore.id, ctx->type_ctx[type]->semaphore.key);
        state = (crash_dump_core_state_t)ctx->type_ctx[type]->header->cores[die_index * CRASH_DUMP_CORE_NUM + core_index];
        release_semaphore(ctx->type_ctx[type]->semaphore.id);
    }

    return state;
}

static const char* crash_dump_core_state_to_string(crash_dump_core_state_t state)
{
    switch (state)
    {
    case CRASH_DUMP_STATE_NOT_AVAILABLE:
        return "NA";
    case CRASH_DUMP_STATE_READY:
        return "RD";
    case CRASH_DUMP_STATE_IN_PROGRESS:
        return "IP";
    case CRASH_DUMP_STATE_COMPLETED:
        return "CP";
    default:
        return "Unknown";
    }
}

static void crash_dump_type_dump_status(crash_dump_type_context_t* type_context)
{
    if (type_context)
    {
        wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
        FPFwCDPrintf("CD[%d]: %02d [%s, %s, %s, %s, %s, %s] [%s, %s, %s, %s, %s, %s]\n",
                     type_context->type,
                     type_context->header->status,
                     crash_dump_core_state_to_string(type_context->header->cores[0]),   // MCP0
                     crash_dump_core_state_to_string(type_context->header->cores[1]),   // SCP0
                     crash_dump_core_state_to_string(type_context->header->cores[2]),   // HSP0
                     crash_dump_core_state_to_string(type_context->header->cores[3]),   // SDM0
                     crash_dump_core_state_to_string(type_context->header->cores[4]),   // CDED0
                     crash_dump_core_state_to_string(type_context->header->cores[5]),   // KMP0
                     crash_dump_core_state_to_string(type_context->header->cores[6]),   // MCP1
                     crash_dump_core_state_to_string(type_context->header->cores[7]),   // SCP1
                     crash_dump_core_state_to_string(type_context->header->cores[8]),   // HSP1
                     crash_dump_core_state_to_string(type_context->header->cores[9]),   // SDM1
                     crash_dump_core_state_to_string(type_context->header->cores[10]),  // CDED1
                     crash_dump_core_state_to_string(type_context->header->cores[11])); // KMP1
        release_semaphore(type_context->semaphore.id);
    }
}

void crash_dump_dump_status(crash_dump_type_context_t* type_context)
{
    if (type_context)
    {
        crash_dump_type_dump_status(type_context);
    }
    else
    {
        crash_dump_context_t* ctx = crash_dump_context();
        for (uint16_t i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
        {
            crash_dump_type_dump_status(ctx->type_ctx[i]);
        }
    }
}

void crash_dump_update_accel_state(ACCEL_ID accel_type, crash_dump_core_state_t state)
{
    crash_dump_context_t* ctx = crash_dump_context();
    crash_dump_type_context_t* type_context =
        ctx->type_ctx[CRASH_DUMP_TYPE_FULL]; // Accelerator crash dump is only for full dump.
    uint32_t accel_index = CRASH_DUMP_CORE_SDM;

    switch (accel_type)
    {
    case ACCEL_ID_SDM:
        accel_index = CRASH_DUMP_CORE_SDM;
        break;
    case ACCEL_ID_CDED:
        accel_index = CRASH_DUMP_CORE_CDED;
        break;
    default:
        // Not valid Accelerator ID
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_STATUS_INVALID_PARAMS, accel_type);
        return;
    }

    if (type_context != NULL && type_context->header != NULL)
    {
        wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
        // Do not transit accel cd state if it's going to ready from completed
        if (state != CRASH_DUMP_STATE_READY ||
            type_context->header->cores[ctx->die_index * CRASH_DUMP_CORE_NUM + accel_index] != CRASH_DUMP_STATE_COMPLETED)
        {
            type_context->header->cores[ctx->die_index * CRASH_DUMP_CORE_NUM + accel_index] = (uint8_t)state;
        }
        release_semaphore(type_context->semaphore.id);
    }
}

bool crash_dump_is_transferring(crash_dump_type_context_t* type_context)
{
    bool transferring = false;

    if (type_context)
    {
        wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
        transferring = type_context->header->status == CRASH_DUMP_IN_TRANSFER;
        release_semaphore(type_context->semaphore.id);
    }

    return transferring;
}

bool crash_dump_oob_transfer(void)
{
    crash_dump_context_t* ctx = crash_dump_context();
    bool has_completed_cd = false;
    bool has_inprogress_cd = false;

    if (ctx != NULL && ctx->type_ctx[CRASH_DUMP_TYPE_FULL] != NULL)
    {
        crash_dump_type_context_t* type_context = ctx->type_ctx[CRASH_DUMP_TYPE_FULL];

        wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
        if (type_context->header->status == CRASH_DUMP_IN_USE)
        {
            for (uint16_t i = 0; i < CRASH_DUMP_CORE_NUM * 2; i++)
            {
                if (type_context->header->cores[i] == CRASH_DUMP_STATE_COMPLETED)
                {
                    has_completed_cd = true;
                }
                else if (type_context->header->cores[i] == CRASH_DUMP_STATE_IN_PROGRESS)
                {
                    has_inprogress_cd = true;
                }
            }
        }
        release_semaphore(type_context->semaphore.id);
    }

    return has_completed_cd && !has_inprogress_cd;
}