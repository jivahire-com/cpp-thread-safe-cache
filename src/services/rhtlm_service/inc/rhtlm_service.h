//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file rh_tlm_service.h
 * Header file for Row Hammer Telemetry
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>
#include <tx_api.h>

/*-------------- Typedefs ----------------*/
#define RHTLM_THREAD_STACKSIZE ((TX_MINIMUM_STACK) + 1 * 1024)
#define RHTLM_MC_DIE0_COUNT_NR 12
#define RHTLM_MC_DIE1_COUNT_NR 24

typedef struct
{
    TX_THREAD s_rhtlm_thread;
    ULONG sleep_period;
    uint8_t die_id;
    uint8_t s_stack_pool_memory[RHTLM_THREAD_STACKSIZE];
    bool active;
} rhtlm_service_ctx_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief  : Init for Row Hammer Telemetry Service
 *
 * @param  : context - pointer to this thread service specific structure 
 * @retval : void 
 */

void rhtlm_thread_init(rhtlm_service_ctx_t* context);

/**
 * @brief  : Print status Information about the service
 * @param  : void
 * @retval : void
 */
void rhtlm_print_service_status();
