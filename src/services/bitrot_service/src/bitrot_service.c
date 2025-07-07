//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file bitrot_service_init.c
 * This file contains the bitrot services
 */

/*-------------------------------- Includes ---------------------------------*/
#include <ErrorHandler.h>
#include <FpFwCli.h>
#include <FpFwUtils.h>
#include <bitrot_service.h>
#include <idsw.h> // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <mu_public.h>
#include <stdint.h> // for uint8_t
#include <stdio.h>  // for printf
#include <tx_api.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
// bitrot thread priority is the lowest possible but higher than watchdog value

#define BITROT_THREAD_PRIORITY          (TX_MAX_PRIORITIES - 2)
#define BITROT_THREAD_PREEMPT_THRESHOLD (BITROT_THREAD_PRIORITY)
#define BITROT_THREAD_NAME              "Bitrot Thread"
#define BITROT_TICKS_PER_SECOND         (100u)
#define MILISECONDS_PER_SECOND          (1000u)
/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

void bitrot_thread_worker_function(ULONG thread_input);

void bitrot_walk_function(mscp_bitrotservice_ctx_t* context);

/*------------------- Declarations (Statics and globals) --------------------*/

/*----------------------------- Static Functions ----------------------------*/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC push_options
#pragma GCC optimize("O0")
static void sdm_mem_walk(volatile mscp_mem_type* start, uint64_t len)
{
    for (uint64_t mem_read_count = (len / sizeof(*start)); mem_read_count > 0; mem_read_count--)
    {
        *start++;
    }
}
#pragma GCC pop_options
#pragma GCC diagnostic pop

/*----------------------------- Global Functions ----------------------------*/

void bitrot_thread_init(mscp_bitrotservice_ctx_t* context)
{
    UINT status;
    uint64_t bitrot_tcm_delay;

    bitrot_tcm_delay = config_get_bitrot_TCM_delay();
    context->sleep_period = (BITROT_TICKS_PER_SECOND * bitrot_tcm_delay) / MILISECONDS_PER_SECOND;

    status = tx_thread_create(&context->s_bitrot_thread,
                              BITROT_THREAD_NAME,
                              bitrot_thread_worker_function,
                              (ULONG)(uintptr_t)context,
                              (void*)context->s_stack_pool_memory,
                              sizeof(context->s_stack_pool_memory),
                              BITROT_THREAD_PRIORITY,
                              BITROT_THREAD_PREEMPT_THRESHOLD,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        FpFwCliPrint("[Bitrot Service] Failed to create worker thread! TX_STATUS: %d\n", status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }
}

void bitrot_walk_function(mscp_bitrotservice_ctx_t* context)
{
    context->sleep_period = (BITROT_TICKS_PER_SECOND * config_get_bitrot_TCM_delay()) / MILISECONDS_PER_SECOND;
    if (context->sleep_period > 0)
    {
        for (size_t i = 0; i < context->mem_table_len; i++)
        {
            if (context->mem_table[i].len > 0)
            {
                FpFwCliPrint("[Bitrot Service] Reading %s\n", context->mem_table[i].p_mem_type_name);
                sdm_mem_walk(context->mem_table[i].start_addr, context->mem_table[i].len);
            }
        }
        // sleep for 8 hours @100 ticks per second
        FpFwCliPrint("[Bitrot Service] Sleeping for %d ms.\n", context->sleep_period);
        tx_thread_sleep(context->sleep_period);
    }
    else
    {
        /* Delay is set to 0 ms. Assuming bitrot service was meant to be disabled, so do a dummy sleep for 1 second . */
        tx_thread_sleep(BITROT_TICKS_PER_SECOND);
    }
}

void bitrot_thread_worker_function(ULONG thread_input)
{
    mscp_bitrotservice_ctx_t* context = (mscp_bitrotservice_ctx_t*)thread_input;
    while (true)
    {
        bitrot_walk_function(context);
    }
}
