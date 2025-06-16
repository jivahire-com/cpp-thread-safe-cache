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
void dma_stall_timer_cb(ULONG input);

/*-- Declarations (Statics and globals) --*/
static device_identifier_pair_t device_channel_pair[DMAC_MAX_CHANNELS] = {{.channel = 0}, {.channel = 1}};

/*------------- Functions ----------------*/
void disable_timer(TX_TIMER* timer, uint32_t channel)
{
    UINT status = tx_timer_deactivate(timer);
    if (TX_SUCCESS != status)
    {
        FPFwErrorRaise(FPFW_ERROR_DMA_TIMER_ERROR, channel, status, 0, 0); // channel, status, unique instance, unused
    }
}

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
        disable_timer(&device->polling_write_timer[channel], channel);

        // Process the completed write request
        DMA_LOG_INFO("write request completed\n");

        // Set completion status
        // Read the status from the DMA controller / library
        // ? handle_rx_interrupt_status(intr_status, device);
        dma_complete_request(device, channel, FPFW_DMA_STATUS_SUCCESS);
    }
    else
    {
        DMA_LOG_INFO("Polling timer cb: DMA channel %d is not available\n", (int)channel);

        // If the pending write request is NULL, stop the timer.
        if (device->Channel[channel].current_request == NULL)
        {
            DMA_LOG_INFO("Disabling timer %d timer, request is NULL\n", (int)channel);
            disable_timer(&device->polling_write_timer[channel], channel);
        }
    }
}

uint32_t initialize_stall_timer(dma_device_t* device, uint32_t channel)
{
    uint32_t timeout_ms = device->config->stall_timeout_ms;

    if (channel >= DMAC_MAX_CHANNELS)
    {
        DMA_LOG_INFO("Invalid DMA channel %d\n", (int)channel);
        return TX_TIMER_ERROR;
    }

    // Initialize the device_channel_pair as a context for the timer callback
    device_channel_pair[channel].device = device;

    // Initialize the stall timer for the DMA channel
    DMA_LOG_INFO("Init stall timer for DMA channel %d\n", (int)channel);
    UINT status = TX_TIMER_ERROR;
    // clang-format off
    status = tx_timer_create(
        &device->stall_timer[channel],
        "DMA_Stall",
        dma_stall_timer_cb,
        (ULONG)&device_channel_pair[channel],
        ((TX_TIMER_TICKS_PER_SECOND * timeout_ms) / 1000UL),
        0U,
        TX_NO_ACTIVATE
    );

    if (TX_SUCCESS != status)
    {
        DMA_LOG_INFO("Failed DMA stall timer, DMA channel %d. Status=0x%X\n", (int)channel, status);
        // FPFwErrorRaise will be called by the caller
    }

    // clang-format on
    return status;
}

void dma_stall_timer_cb(ULONG input)
{
    /* REMEMBER: This is called from the timer's thread - not DFWK.  Need to mind thread safety */

    device_identifier_pair_t* device_channel_pair = (device_identifier_pair_t*)input;
    dma_device_t* device = device_channel_pair->device;
    uint32_t channel = device_channel_pair->channel;

    // Check if DMA transaction has completed.
    if (!dma_is_channel_free(device, channel))
    {
        DMA_LOG_INFO("Stall timer cb: DMA channel %d has stalled !! Aborting\n", (int)channel);
        dma_abort_transaction(device, channel);
        dma_complete_request(device, channel, FPFW_DMA_STATUS_FAIL);
        // No need to disable timer, as it is One-Shot
    }
    else
    {
        DMA_LOG_INFO("Stall timer cb: Tx on channel %d is done\n", (int)channel);
        // No need for `dma_complete_request()` as it would have been called
        // after the request completion
        // No need to disable timer, as it is One-Shot
    }
}
