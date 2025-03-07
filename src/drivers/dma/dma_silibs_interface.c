//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dma_library_interface.c
 * This file supports the interactions with Silicon Libs DMAC library
 */

/*------------- Includes -----------------*/
#include "dma_dfwk.h"
#include "dma_private.h"

#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FpFwAssert.h>   // for FPFW_RUNTIME_ASSERT
#include <dmac.h>         // for dmac_init
#include <stdio.h>        // for DMA_LOG_INFO
#include <tx_api.h>       // for TX_NO_TIME_SLICE, TX_SUCCESS, TX_WAIT_FOREVER, ...

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
bool dma_is_channel_free(dma_device_t* device, uint32_t channel)
{
    return (dmac_is_channel_available(device->config->base_address, channel));
}

void dma_start_transfer(dma_device_t* device, uint32_t channel, PDFWK_ASYNC_REQUEST_HEADER dfwk_request)
{
    FPFW_RUNTIME_ASSERT(device != NULL && dfwk_request != NULL);

    pdma_async_request_t request = (pdma_async_request_t)dfwk_request;
    device->Channel[channel].current_request = request;

    // If polling mode, start the timer
    if (device->config->config_type == DMA_CONFIG_TYPE_POLLING)
    {
        DMA_LOG_INFO("Starting polling write timer, channel %d\n", (int)channel);
        uint32_t status = tx_timer_activate(&device->polling_write_timer[channel]);
        if (TX_SUCCESS != status)
        {
            DMA_LOG_INFO("Failed to activate timer, DMA channel %d. Status=0x%X\n", (int)channel, (int)status);
            FPFwErrorRaise(FPFW_ERROR_DMA_TIMER_ERROR, channel, status, 2, 0); // channel, status, unique instance, unused
        }
    }

    // Start the DMA transfer
    request->status = FPFW_DMA_STATUS_IN_PROGRESS;

    dmac_transfer_cfg_t dma_tr_cfg = {0};
    dma_tr_cfg.channel_id = channel;
    dma_tr_cfg.dmac_src_addr = request->input.src_addr_32b_aligned;
    dma_tr_cfg.dmac_dest_addr = request->dma_private.translated_mscp_atu_address;
    dma_tr_cfg.transfer_size = request->input.byte_count;

    dmac_cfg_t dma_cfg = {0};
    dma_cfg.dmac_transfer_cfg = &dma_tr_cfg;
    dma_cfg.max_block_ts_bytes = DMAC_MAX_BLOCK_TS_BYTES;
    dma_cfg.dmac_src_tr_width = DMAC_TRANSFER_WIDTH_32_BITS;
    dma_cfg.dmac_dest_tr_width = DMAC_TRANSFER_WIDTH_32_BITS;

    int sts = dmac_start_single_block_transfer(device->config->base_address, &dma_cfg);
    DMA_LOG_INFO("Single block transfer was initiated: %d\n", sts);
}