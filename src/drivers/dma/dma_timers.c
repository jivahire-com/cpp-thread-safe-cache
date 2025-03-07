//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dma_timers.c
 * This file supports the POLLING configuration and WDT timers for the each DMA channel.
 */

/*------------- Includes -----------------*/
#include "dma_dfwk.h"
#include "dma_private.h"

#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FpFwAssert.h>   // for FPFW_RUNTIME_ASSERT
#include <stdio.h>        // for DMA_LOG_INFO
#include <tx_api.h>       // for TX_NO_TIME_SLICE, TX_SUCCESS, TX_WAIT_FOREVER, ...

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void dma_write_async_polling_timer_cb(ULONG input);

/*-- Declarations (Statics and globals) --*/
static device_identifier_pair_t device_channel_pair[DMAC_MAX_CHANNELS] = {{.channel = 0}, {.channel = 1}};

/*------------- Functions ----------------*/
uint32_t initialize_tx_timer(dma_device_t* device, uint32_t channel)
{
    if (channel >= DMAC_MAX_CHANNELS)
    {
        DMA_LOG_INFO("Invalid DMA channel %d\n", (int)channel);
        return TX_TIMER_ERROR;
    }

    // Initialize the device_channel_pair as a context for the timer callback
    device_channel_pair[channel].device = device;

    // Initialize the timer for the DMA channel
    DMA_LOG_INFO("Init polling timer for DMA channel %d\n", (int)channel);
    UINT status = TX_TIMER_ERROR;
    // clang-format off
    status = tx_timer_create(
        &device->polling_write_timer[channel],
        "DMA_Polling",
        dma_write_async_polling_timer_cb,
        (ULONG)&device_channel_pair[channel],
        TIMER_INIT_TICKS,
        TIMER_INIT_TICKS,
        TX_NO_ACTIVATE
    );

    if (TX_SUCCESS != status)
    {
        DMA_LOG_INFO("Failed DMA polling timer, DMA channel %d. Status=0x%X\n", (int)channel, status);
        // FPFwErrorRaise will be called by the caller
    }

    // clang-format on
    return status;
}

void dma_write_async_polling_timer_cb(ULONG input)
{
    /* REMEMBER: This is called from the timer's thread - not DFWK.  Need to mind thread safety */

    device_identifier_pair_t* device_channel_pair = (device_identifier_pair_t*)input;
    dma_device_t* device = device_channel_pair->device;
    uint32_t channel = device_channel_pair->channel;

    // Check if the pending write request is complete, stop the timer.
    if (dma_is_channel_free(device, channel))
    {
        DMA_LOG_INFO("Polling timer cb: DMA channel %d is ready\n", (int)channel);
        DMA_LOG_INFO("Disabling timer %d\n", (int)channel);
        uint32_t status = tx_timer_deactivate(&device->polling_write_timer[channel]);
        if (TX_SUCCESS != status)
        {
            FPFwErrorRaise(FPFW_ERROR_DMA_TIMER_ERROR, channel, status, 3, 0); // channel, status, unique instance, unused
        }

        // Process the completed write request
        DMA_LOG_INFO("write request completed\n");

        // Set completion status
        // Read the status from the DMA controller / library
        // ? handle_rx_interrupt_status(intr_status, device);
        device->Channel[channel].current_request->status = FPFW_DMA_STATUS_SUCCESS; // or FPFW_DMA_STATUS_FAIL;

        dma_complete_request(device, channel);
    }
    else
    {
        DMA_LOG_INFO("Polling timer cb: DMA channel %d is not available\n", (int)channel);
        return;
    }

    // If the pending write request is NULL, stop the timer.
    if (device->Channel[channel].current_request == NULL)
    {
        DMA_LOG_INFO("Disabling timer %d timer, request is NULL\n", (int)channel);
        uint32_t status = tx_timer_deactivate(&device->polling_write_timer[channel]);
        if (TX_SUCCESS != status)
        {
            FPFwErrorRaise(FPFW_ERROR_DMA_TIMER_ERROR, channel, status, 4, 0); // channel, status, unique instance, unused
        }
    }
}
