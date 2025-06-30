//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file bitrot_service.h
 * Header file for Bitrot Service
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>
#include <tx_api.h>

/*-------------- Typedefs ----------------*/
#define BITROT_THREAD_STACKSIZE ((TX_MINIMUM_STACK) + 1 * 1024)

typedef uint64_t mscp_mem_type;
typedef struct
{
    volatile mscp_mem_type* start_addr;
    char* p_mem_type_name;
    uint64_t len;
} mscp_mem_table_t;

typedef struct
{
    TX_THREAD s_bitrot_thread;
    ULONG sleep_period;
    size_t mem_table_len;
    mscp_mem_table_t* mem_table;
    uint8_t s_stack_pool_memory[BITROT_THREAD_STACKSIZE];
} mscp_bitrotservice_ctx_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Init for bitrot detection service
 *
 *
 * @retval
 * UINT
 */

void bitrot_thread_init(mscp_bitrotservice_ctx_t* context);
