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
#include "dma_dfwk.h"       // For dma_device_t, etc.
#include "dma_events.h"     // For DMA event trace

/*-- Symbolic Constant Macros (defines) --*/
#define MODULE_NAME "[DMA] "
#define NEWLINE     "\n"
#define INFO
#define WARN        "[WARN] "
#define CRIT        "[CRIT] "

// set to 1 for more verbose logs
#define DMA_ENABLE_TRACE_LEVEL_LOG 0

#if DMA_ENABLE_TRACE_LEVEL_LOG
#define DMA_LOG_TRACE(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#else
#define DMA_LOG_TRACE(fmt, ...)
#endif
#define DMA_LOG_INFO(fmt, ...) printf(MODULE_NAME INFO fmt NEWLINE, ##__VA_ARGS__)
#define DMA_LOG_WARN(fmt, ...) printf(MODULE_NAME WARN fmt NEWLINE, ##__VA_ARGS__)
#define DMA_LOG_CRIT(fmt, ...) printf(MODULE_NAME CRIT fmt NEWLINE, ##__VA_ARGS__)

// Channel Interrupts Mask Aliases
#define DMAC_CH_INT_CHANNEL_ABORTED                 (0x1ULL << 31)
#define DMAC_CH_INT_CHANNEL_DISABLED                (0x1ULL << 30)
#define DMAC_CH_INT_CHANNEL_SUSPENDED               (0x1ULL << 29)
#define DMAC_CH_INT_CHANNEL_LOCK_CLEARED            (0x1ULL << 27)
#define DMAC_CH_INT_SHADOWREG_OR_LLI_INVALID_ERR    (0x1ULL << 13)
#define DMAC_CH_INT_LLI_WR_SLV_ERR                  (0x1ULL << 12)
#define DMAC_CH_INT_LLI_RD_SLV_ERR                  (0x1ULL << 11)
#define DMAC_CH_INT_LLI_WR_DEC_ERR                  (0x1ULL << 10)
#define DMAC_CH_INT_LLI_RD_DEC_ERR                  (0x1ULL << 9)
#define DMAC_CH_INT_DST_SLV_ERR                     (0x1ULL << 8)
#define DMAC_CH_INT_SRC_SLV_ERR                     (0x1ULL << 7)
#define DMAC_CH_INT_DST_TRANSCOMP                   (0x1ULL << 4)
#define DMAC_CH_INT_SRC_TRANSCOMP                   (0x1ULL << 3)
#define DMAC_CH_INT_DMA_TFR_DONE                    (0x1ULL << 1)
#define DMAC_CH_INT_BLOCK_TFR_DONE                  (0x1ULL << 0)

/* Mask for all enabled channel interrupts */
#define DMAC_ENABLED_CH_INTS_MASK                                                                   \
(                                                                                                   \
    DMAC_CH_INT_CHANNEL_ABORTED | DMAC_CH_INT_CHANNEL_DISABLED | DMAC_CH_INT_CHANNEL_SUSPENDED |    \
    DMAC_CH_INT_CHANNEL_LOCK_CLEARED | DMAC_CH_INT_SHADOWREG_OR_LLI_INVALID_ERR |                   \
    DMAC_CH_INT_LLI_WR_SLV_ERR | DMAC_CH_INT_LLI_RD_SLV_ERR | DMAC_CH_INT_LLI_WR_DEC_ERR |          \
    DMAC_CH_INT_LLI_RD_DEC_ERR | DMAC_CH_INT_DST_SLV_ERR | DMAC_CH_INT_SRC_SLV_ERR |                \
    DMAC_CH_INT_DST_TRANSCOMP | DMAC_CH_INT_SRC_TRANSCOMP | DMAC_CH_INT_DMA_TFR_DONE |              \
    DMAC_CH_INT_BLOCK_TFR_DONE                                                                      \
)

/* Mask for all enabled channel ERROR interrupts */
#define DMAC_ENABLED_CH_ERR_INTS_MASK               \
(                                                   \
    DMAC_CH_INT_SHADOWREG_OR_LLI_INVALID_ERR |      \
    DMAC_CH_INT_LLI_WR_SLV_ERR |                    \
    DMAC_CH_INT_LLI_RD_SLV_ERR |                    \
    DMAC_CH_INT_LLI_WR_DEC_ERR |                    \
    DMAC_CH_INT_LLI_RD_DEC_ERR |                    \
    DMAC_CH_INT_DST_SLV_ERR |                       \
    DMAC_CH_INT_SRC_SLV_ERR                         \
)


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
void dma_complete_request(dma_device_t* device, uint32_t channel, FPFW_DMA_STATUS req_status);
void update_channel_struct_add_remaining_bytes(pdma_device_t device, pdma_async_request_t request);
void update_channel_struct_subtract_remaining_bytes(pdma_device_t device, pdma_async_request_t request);
uint32_t get_channel_with_shortest_queue(pdma_device_t device);
bool dma_is_channel_free(dma_device_t* device, uint32_t channel);
void dma_abort_transaction(dma_device_t* device, uint32_t channel);

// Timers
uint32_t initialize_tx_timer(dma_device_t* device, uint32_t channel);
uint32_t initialize_stall_timer(dma_device_t* device, uint32_t channel);
void disable_timer(TX_TIMER* timer, uint32_t channel);

// Others
bool check_parameters(pdma_device_t device, pdma_async_request_t request);
bool is_valid_src_addr(pdma_device_t device, uint32_t src_addr_32b_aligned);



