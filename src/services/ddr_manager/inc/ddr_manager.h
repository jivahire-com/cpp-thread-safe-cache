//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager.h
 * DDR Manager public API
 */

#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_icc_base.h>
#include <stddef.h>
#include <stdint.h>
#include <DfwkThreadXHost.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef enum
{
    DDR_CREATE_MEMORY_MAP_EVENT,
    DDR_CREATE_BDAT_EVENT,
    DDR_CREATE_SMBIOS_TABLES_EVENT,
    DDR_POLL_DIMMS_I3C_EVENT,
    DDR_I3C_DATA_READY_EVENT,
} DDR_MANAGER_MESSAGE_TYPE;

typedef struct
{
    struct
    {
        void* p_stack;              // Pointer to the thread stack
        size_t stack_size;          // Size of the provided memory
        uint32_t priority;          // ThreadX Thread priority
        uint32_t time_slice_option; // ThreadX Thread slicing option
        uint8_t die_number;         // Die number 0-1
    } thread_config;
    struct
    {
        ULONG initial_ticks;    // Initial ticks for the timer
        ULONG reschedule_ticks; // Reschedule ticks for the timer
    } timer_config;
    struct
    {
        void* p_queue;        // Pointer to the queue
        UINT msg_size;        //Size in no. of 32b words
        UINT queue_num_words; // Number of 32b words in the queue
    } queue_config;
} ddr_service_config_t;

typedef struct
{
    TX_THREAD work_thread;
    TX_QUEUE work_queue;
    TX_TIMER ddr_polling_timer;
} ddr_service_context_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void ddr_manager_init(ddr_service_context_t *pddr_service_ctx, ddr_service_config_t *pconfig, fpfw_icc_base_ctx_t *icc_ctx);
