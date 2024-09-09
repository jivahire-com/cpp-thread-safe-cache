//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dma_interrupts.c
 * This file supports all interrupt related tasks for the DMA driver
 *
 * @TODO: (Maya) Clarify which interrupts should be enabled & where the channel / common interrupts should be checked
 */

/*------------- Includes -----------------*/
#include "dma_dfwk.h"
#include "dma_private.h"

#include <DfwkDriver.h>   // for DfwkInterfaceInitialize, DfwkQueueInitialize
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FpFwAssert.h>   // for FPFW_RUNTIME_ASSERT#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <arm_intrinsic.h>
#include <dw_axi_dmac_regs.h>
#include <interrupts.h>
#include <nvic.h>  // for nvic_irq_clear_pending, nvic_irq_enable, __DSB
#include <stdio.h> // for printf

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void dma_isr(void* context)
{
    device_identifier_pair_t* device_interrupt_pair = (device_identifier_pair_t*)context;
    dma_device_t* device = device_interrupt_pair->device;
    uint64_t INT_MASK = 0xFFFFFFFFFFFFFFFF; // Todo: Task 2015102: Update DMA interrupt handling
    uint32_t interrupt_id = device_interrupt_pair->interrupt_id;
    uint32_t status;

    // TODO: Clarify where this should go
    status = dmac_get_common_interrupt_status(device->config->base_address);
    FPFW_UNUSED(status);
    dmac_clear_common_interrupts(device->config->base_address, INT_MASK); // TODO: Task 2015102: Update DMA interrupt handling

    if (interrupt_id == DMAC_CH0_INT)
    {
        // Handle Channel 0 interrupt
        dma_channel_isr(device, CH_0);
    }
    else if (interrupt_id == DMAC_CH1_INT)
    {
        // Handle Channel 1 interrupt
        dma_channel_isr(device, CH_1);
    }
    else if (interrupt_id == DMAC_REG_INT)
    {
        // Handle the interrupt
        dma_reg_isr(device);
    }
    else if (interrupt_id == DMAC_ATU_PARITY_INT)
    {
        // Handle the interrupt
        dma_atu_parity_isr(device);
    }
    else
    {
        // Invalid interrupt
        FPFwErrorRaise(FPFW_ERROR_DMA_INVALID_INTERRUPT, 0, 0, 0, 0);
    }

    __DSB();
}

void dma_channel_isr(dma_device_t* device, uint32_t channel)
{
    uint64_t INT_MASK = 0xFFFFFFFFFFFFFFFF; // Todo: Task 2015102: Update DMA interrupt handling
    uint64_t status;

    // Handle Channel x interrupt
    status = dmac_get_ch_interrupt_status(device->config->base_address, channel);
    dmac_clear_ch_interrupts(device->config->base_address, INT_MASK, channel); // TODO: Task 2015102: Update DMA interrupt handling
    FPFW_UNUSED(status);

    // If completed:
    // Set completion status
    device->Channel[channel].current_request->status = FPFW_DMA_STATUS_SUCCESS; // or FPFW_DMA_STATUS_FAIL;

    // Have dfwk process the callback
    dma_complete_request(device, channel);
}

void dma_reg_isr(dma_device_t* device)
{
    // Handle the interrupt
    FPFW_UNUSED(device);
}

void dma_atu_parity_isr(dma_device_t* device)
{
    // Handle the interrupt
    FPFW_UNUSED(device);
}