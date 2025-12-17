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
#include <FpFwUtils.h>    // for FPFW_UNUSED
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
    uint64_t status;

    // Handle Chan x interrupt
    status = dmac_get_ch_interrupt_status(device->config->base_address, channel);
    dmac_clear_ch_interrupts(device->config->base_address, DMAC_ENABLED_CH_INTS_MASK, channel);

    if (status & DMAC_ENABLED_CH_ERR_INTS_MASK)
    {
        if (status & DMAC_CH_INT_SHADOWREG_OR_LLI_INVALID_ERR)
        {
            DMA_LOG_CRIT("Chan %u: LLI found with 0 Valid Bit. DMA Tx suspended", (unsigned int)channel);
            DMA_ET_ERROR_PARAM(DMA_ET_TYPE_LLI_INVALID, channel);
        }

        if (status & DMAC_CH_INT_LLI_WR_SLV_ERR)
        {
            DMA_LOG_CRIT("Chan %u: Unable to write-back to LLI tail. Chan Disabled", (unsigned int)channel);
            DMA_ET_ERROR_PARAM(DMA_ET_TYPE_LLI_WR_SLV_ERR, channel);
        }

        if (status & DMAC_CH_INT_LLI_RD_SLV_ERR)
        {
            DMA_LOG_CRIT("Chan %u: Unable to read LLI. Chan Disabled", (unsigned int)channel);
            DMA_ET_ERROR_PARAM(DMA_ET_TYPE_LLI_RD_SLV_ERR, channel);
        }

        if (status & DMAC_CH_INT_LLI_WR_DEC_ERR)
        {
            DMA_LOG_CRIT("Chan %u: LLI tail Addr inaccessible to Write-back. Chan Disabled", (unsigned int)channel);
            DMA_ET_ERROR_PARAM(DMA_ET_TYPE_LLI_WR_DEC_ERR, channel);
        }

        if (status & DMAC_CH_INT_LLI_RD_DEC_ERR)
        {
            DMA_LOG_CRIT("Chan %u: LLI Addr inaccessible to Read. Chan Disabled", (unsigned int)channel);
            DMA_ET_ERROR_PARAM(DMA_ET_TYPE_LLI_RD_DEC_ERR, channel);
        }

        if (status & DMAC_CH_INT_DST_SLV_ERR)
        {
            DMA_LOG_CRIT("Chan %u: Dest Addr inaccessible. Chan Disabled", (unsigned int)channel);
            DMA_ET_ERROR_PARAM(DMA_ET_TYPE_DST_SLV_ERR, channel);
        }

        if (status & DMAC_CH_INT_SRC_SLV_ERR)
        {
            DMA_LOG_CRIT("Chan %u: Src Addr inaccessible. Chan Disabled", (unsigned int)channel);
            DMA_ET_ERROR_PARAM(DMA_ET_TYPE_SRC_SLV_ERR, channel);
        }

        /* Notify Requestor that the DMA Tx failed */
        dma_complete_request(device, channel, FPFW_DMA_STATUS_FAIL);
    }

    if (status & DMAC_CH_INT_CHANNEL_ABORTED)
    {
        DMA_LOG_WARN("Chan %u: Abort INT recvd. Chan Txfer has been Aborted", (unsigned int)channel);
        DMA_ET_WARNING_PARAM(DMA_ET_TYPE_CHANNEL_ABORTED, channel);
        dma_complete_request(device, channel, FPFW_DMA_STATUS_FAIL);
    }

    if (status & DMAC_CH_INT_CHANNEL_DISABLED)
    {
        DMA_LOG_INFO("Chan %u: Disable INT recvd", (unsigned int)channel);
    }

    if (status & DMAC_CH_INT_CHANNEL_SUSPENDED)
    {
        DMA_LOG_INFO("Chan %u: Suspend INT recvd", (unsigned int)channel);
    }

    if (status & DMAC_CH_INT_CHANNEL_LOCK_CLEARED)
    {
        DMA_LOG_INFO("Chan %u: Lock Clear INT recvd", (unsigned int)channel);
    }

    if (status & DMAC_CH_INT_DST_TRANSCOMP)
    {
        DMA_LOG_INFO("Chan %u: Dest Txaction Complete INT recvd", (unsigned int)channel);
    }

    if (status & DMAC_CH_INT_SRC_TRANSCOMP)
    {
        DMA_LOG_INFO("Chan %u: Src Txaction Complete INT recvd", (unsigned int)channel);
    }

    if (status & DMAC_CH_INT_BLOCK_TFR_DONE)
    {
        DMA_LOG_INFO("Chan %u: Block Txfer Done INT recvd", (unsigned int)channel);
    }

    /* Transfer Complete successfully */
    if (status & DMAC_CH_INT_DMA_TFR_DONE)
    {
        DMA_LOG_INFO("Chan %u: DMA Txfer Done INT recvd", (unsigned int)channel);
        dma_complete_request(device, channel, FPFW_DMA_STATUS_SUCCESS);
    }
}

void dma_reg_isr(dma_device_t* device)
{
    uint64_t status;

    // Handle DMA Common Reg interrupt
    status = dmac_get_common_interrupt_status(device->config->base_address);

    DMA_LOG_INFO("Common Register INT %llx recvd", status);
}

void dma_atu_parity_isr(dma_device_t* device)
{
    // Handle the interrupt
    FPFW_UNUSED(device);
}
