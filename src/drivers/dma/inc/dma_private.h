//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dma_private.h
 * This file describes the private functions internal to the DMA driver.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MODULE_NAME "[DMA] "
#define NEWLINE     "\n"

// set to 1 for more verbose logs
#define DMA_ENABLE_TRACE_LEVEL_LOG 0

#if DMA_ENABLE_TRACE_LEVEL_LOG
#define DMA_LOG_TRACE(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#else
#define DMA_LOG_TRACE(fmt, ...)
#endif
#define DMA_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define DMA_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define DMA_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

/*--------- Function Prototypes ----------*/
// Interrupts
void dma_isr(void* context);
void dma_channel_isr(dma_device_t *device, uint32_t channel);
void dma_reg_isr(dma_device_t *device);
void dma_atu_parity_isr(dma_device_t *device);
void dma_enable_nvic_interrupts(dma_device_t *device);

// Queue management
void move_request_to_channel_queue(pdma_device_t device, pdma_async_request_t request);
void dma_main_queue_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context);
void dma_channel_0_queue_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context);
void dma_channel_1_queue_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context);
void dma_start_transfer(dma_device_t* device, uint32_t channel, PDFWK_ASYNC_REQUEST_HEADER request);
void dma_complete_request(dma_device_t* device, uint32_t channel);
void update_channel_struct_add_remaining_bytes(pdma_device_t device, pdma_async_request_t request);
void update_channel_struct_subtract_remaining_bytes(pdma_device_t device, pdma_async_request_t request);
uint32_t get_channel_with_shortest_queue(pdma_device_t device);
bool dma_is_channel_free(dma_device_t* device, uint32_t channel);

// Timers
uint32_t initialize_tx_timer(dma_device_t* device, uint32_t channel);

// Others
bool check_parameters(pdma_device_t device, pdma_async_request_t request);
bool is_valid_src_addr(pdma_device_t device, uint32_t src_addr_32b_aligned);



