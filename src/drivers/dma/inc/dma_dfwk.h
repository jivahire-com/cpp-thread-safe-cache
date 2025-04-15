//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dma_dfwk.h
 * This file describes the structs and public functions for the DMA driver framework.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>
#include <dmac.h>                    // for DMAC_MAX_CHANNELS
#include <fpfw_timer.h>              // for fpfw_timer_create, fpfw_timer...
#include <fpfw_timer_types.h>        // for fpfw_dur_t
#include <idsw_kng.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/*
   _____             __ _                 ______
  / ____|           / _(_)          _    |  ____|
 | |     ___  _ __ | |_ _  __ _   _| |_  | |__   _ __  _   _ _ __ ___  ___
 | |    / _ \| '_ \|  _| |/ _` | |_   _| |  __| | '_ \| | | | '_ ` _ \/ __|
 | |___| (_) | | | | | | | (_| |   |_|   | |____| | | | |_| | | | | | \__ \
  \_____\___/|_| |_|_| |_|\__, |         |______|_| |_|\__,_|_| |_| |_|___/
                           __/ |
                          |___/
*/
typedef enum {
    DMA_CONFIG_TYPE_POLLING,
    DMA_CONFIG_TYPE_INTERRUPT,
    DMA_CONFIG_TYPE_INVALID,
} DMA_CONFIG_TYPE;

typedef enum FPFW_DMA_STATUS
{
    FPFW_DMA_STATUS_NOT_STARTED,
    FPFW_DMA_STATUS_QUEUED,
    FPFW_DMA_STATUS_IN_PROGRESS,
    FPFW_DMA_STATUS_SUCCESS,
    FPFW_DMA_STATUS_FAIL
} FPFW_DMA_STATUS;

enum dma_async_request_type_idx
{
    DMA_REQUEST_NULL,           // Internal request to mark end of entry in Isr_req_queue.
    DMA_REQUEST_SINGLE_ASYNC,   // Request to register deferred single-block DMA transfer with completion callback.
    // DMA_REQUEST_MULTI_ASYNC, // Request to register deferred multi-block DMA transfer with completion callback.
    DMA_REQUEST_COUNT
};

typedef struct {
    uintptr_t base_address; // Base address of the DMA controller - Set in CMakeLists.txt based on SCP or MCP compile
    //
    // When set to DMA_CONFIG_TYPE_POLLING, async operations will be handled with timers.
    // When set to DMA_CONFIG_TYPE_INTERRUPT, async operations will be interrupt driven.
    //
    DMA_CONFIG_TYPE config_type;
    KNG_CPU_TYPE cpu_type;
    uint16_t stall_timeout_ms;
} dma_config_t;


/*
  _____  __  __                                            _____                            _
 |  __ \|  \/  |   /\         /\                          |  __ \                          | |
 | |  | | \  / |  /  \       /  \   ___ _   _ _ __   ___  | |__) |___  __ _ _   _  ___  ___| |_
 | |  | | |\/| | / /\ \     / /\ \ / __| | | | '_ \ / __| |  _  // _ \/ _` | | | |/ _ \/ __| __|
 | |__| | |  | |/ ____ \   / ____ \\__ \ |_| | | | | (__  | | \ \  __/ (_| | |_| |  __/\__ \ |_
 |_____/|_|  |_/_/    \_\ /_/    \_\___/\__, |_| |_|\___| |_|  \_\___|\__, |\__,_|\___||___/\__|
                                         __/ |                           | |
                                        |___/                            |_|
*/
typedef struct _dma_async_request_t
{
    DFWK_ASYNC_REQUEST_HEADER header;
    FPFW_DMA_STATUS status;
    struct
    {
        uint32_t src_addr_32b_aligned;
        uint64_t dest_addr_32b_aligned;
        // TODO: Task 2015101: Add support for Linked-List multi-block transfers + scatter/gather
        size_t byte_count;
    } input;

    struct
    {
        uint32_t assigned_channel;
        uint32_t translated_mscp_atu_address;
        size_t bytes_written;
    } dma_private;
} dma_async_request_t, *pdma_async_request_t;


/*
  _____  ________      _______ _____ ______            _____ _                            _
 |  __ \|  ____\ \    / /_   _/ ____|  ____|    _     / ____| |                          | |
 | |  | | |__   \ \  / /  | || |    | |__     _| |_  | |    | |__   __ _ _ __  _ __   ___| |
 | |  | |  __|   \ \/ /   | || |    |  __|   |_   _| | |    | '_ \ / _` | '_ \| '_ \ / _ \ |
 | |__| | |____   \  /   _| || |____| |____    |_|   | |____| | | | (_| | | | | | | |  __/ |
 |_____/|______|   \/   |_____\_____|______|          \_____|_| |_|\__,_|_| |_|_| |_|\___|_|
*/
typedef struct {
    DFWK_QUEUE queue;
    pdma_async_request_t current_request;
    size_t remaining_bytes_all_requests;
    FPFW_LOCK lock;
} dma_channel_queue_t;

typedef struct {
    DFWK_DEVICE_HEADER Header;
    DFWK_QUEUE Request_Queue;   // Single incoming Client-facing DMA transaction dispatch queue
    dma_channel_queue_t Channel[DMAC_MAX_CHANNELS];

    dma_config_t* config;

    TX_TIMER polling_write_timer[DMAC_MAX_CHANNELS];
    TX_TIMER stall_timer[DMAC_MAX_CHANNELS];    // Stall timer for DMA transactions, to abort
                                                // channel transaction if they continue for more than this time.
} dma_device_t, *pdma_device_t;


/*
  _____  ______        _              __  __ _
 |  __ \|  ____|      | |       _    |  \/  (_)
 | |  | | |____      _| | __  _| |_  | \  / |_ ___  ___
 | |  | |  __\ \ /\ / / |/ / |_   _| | |\/| | / __|/ __|
 | |__| | |   \ V  V /|   <    |_|   | |  | | \__ \ (__ _
 |_____/|_|    \_/\_/ |_|\_\         |_|  |_|_|___/\___(_)
*/
typedef struct {
    DFWK_INTERFACE_HEADER Header;
    dma_device_t* Device;
} dma_interface_t, *pdma_interface_t;

typedef struct {
    dma_device_t *device;
    union {
        uint32_t channel;
        uint32_t interrupt_id;
    };
} device_identifier_pair_t;

/*-- Declarations (Statics and globals) --*/
#define TIMER_INIT_TICKS       (1)
#define TIMER_RESCHEDULE_TICKS (1)

#define CH_0 (0)
#define CH_1 (1)

#define NUM_DMA_INTERRUPTS (DMAC_ATU_PARITY_INT - DMAC_CH0_INT + 1)

#define FPFW_ERROR_DMA_REMAINING_BYTES_INVALID (0x00000001)
#define FPFW_ERROR_DMA_INVALID_INTERRUPT       (0x00000002)
#define FPFW_ERROR_DMA_TIMER_ERROR             (0x00000003)
#define FPFW_ERROR_DMA_STALLED                 (0x00000004)

//*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

void dma_device_init(dma_device_t *device, dma_config_t *pconfig, DFWK_SCHEDULE *schedule);
void dma_interface_init(dma_device_t* device, dma_interface_t* driver_interface);
void dma_lib_init(dma_device_t *device);

#ifdef __cplusplus
}
#endif