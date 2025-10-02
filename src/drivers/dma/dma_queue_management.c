//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dma_queue_management.c
 * This file supports all queue management functions for the DMA driver
 *
 * TODO: Task 2015105: DMA: ATU Mappings
 */

/*------------- Includes -----------------*/
#include "dma_dfwk.h"
#include "dma_private.h"

#include <DfwkDriver.h>   // for DfwkInterfaceInitialize, DfwkQueueInitialize
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FpFwUtils.h>    // for FPFW_UNUSED
#include <atu_lib.h>      // for atu_map
#include <bug_check.h>
#include <idsw_kng.h> // for idsw_get_cpu_type
#include <silibs_mcp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdbool.h> // for bool
#include <stdio.h>   // for DMA_LOG_INFO

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void move_request_to_channel_queue(pdma_device_t device, pdma_async_request_t request)
{
    int channel = request->dma_private.assigned_channel;

    // Move the request to the channel's queue
    DMA_LOG_INFO("Enqueue request to channel %d\n", channel);
    request->status = FPFW_DMA_STATUS_QUEUED;

    // Copy the request to the channel queue and increment channel's total outstanding byte count
    update_channel_struct_add_remaining_bytes(device, request);
    DfwkQueueEnqueueRequest(&device->Channel[channel].queue, (PDFWK_ASYNC_REQUEST_HEADER)request);
}

/**
 * @brief DMA driver request dispatcher.  This assumes the incoming request is already dequeued from
 * the DFWK managed queue and needs to go somewhere.  We will stage that queue here until a channel is available.
 *
 * @param request [in] DMA request.
 * @param context [in] DMA device as context.
 */
void dma_main_queue_dispatch(PDFWK_ASYNC_REQUEST_HEADER dfwk_request, void* context)
{
    pdma_device_t device = (pdma_device_t)context;
    pdma_async_request_t request = (pdma_async_request_t)dfwk_request;
    request->status = FPFW_DMA_STATUS_NOT_STARTED;

    atu_id_t atu_id = device->config->cpu_type == CPU_SCP ? ATU_ID_SCP_EXP : ATU_ID_MCP_EXP;
    uint32_t translated_mscp_atu_address = 0;

    if (!check_parameters(device, request))
    {
        // Invalid request
        request->status = FPFW_DMA_STATUS_FAIL;
        DfwkAsyncRequestComplete(dfwk_request);
        return;
    }

    // Check that destination address is currently handled by ATU map & get the window address
    int status = atu_translate_address(atu_id, request->input.dest_addr_32b_aligned, &translated_mscp_atu_address);
    if (status != SILIBS_SUCCESS)
    {
        // Unmapped & unhandled AP destination address
        DMA_LOG_INFO("Invalid dest addr: 0x%llX\n", request->input.dest_addr_32b_aligned);
        request->status = FPFW_DMA_STATUS_FAIL;
        DfwkAsyncRequestComplete(dfwk_request);
        return;
    }
    request->dma_private.translated_mscp_atu_address = translated_mscp_atu_address;

    switch (dfwk_request->RequestType)
    {
    case DMA_REQUEST_SINGLE_ASYNC:
        request->dma_private.assigned_channel = get_channel_with_shortest_queue(device);
        move_request_to_channel_queue(device, request);

        break;
    default:
        // Unsupported request type.
        BUG_ASSERT(false);
        break;
    }
}

uint32_t get_channel_with_shortest_queue(pdma_device_t device)
{
    uint32_t smaller_channel = CH_0;
    FPFW_LOCK_STATE ch_0_lock_state;
    FPFW_LOCK_STATE ch_1_lock_state;

    ch_0_lock_state = FpFwLockAcquire(&device->Channel[CH_0].lock);
    ch_1_lock_state = FpFwLockAcquire(&device->Channel[CH_1].lock);

    // Return the channel with the least number of bytes queued to transfer (including in progress not yet complete)
    if (device->Channel[CH_0].remaining_bytes_all_requests < device->Channel[CH_1].remaining_bytes_all_requests)
    {
        smaller_channel = CH_0;
    }
    else
    {
        smaller_channel = CH_1;
    }

    FpFwLockRelease(&device->Channel[CH_0].lock, ch_0_lock_state);
    FpFwLockRelease(&device->Channel[CH_1].lock, ch_1_lock_state);

    return smaller_channel;
}

void dma_channel_0_queue_dispatch(PDFWK_ASYNC_REQUEST_HEADER dfwk_request, void* context)
{
    pdma_device_t device = (pdma_device_t)context;

    // Double check that the channel is free
    BUG_ASSERT_PARAM(device->Channel[CH_0].current_request == NULL, device->Channel[CH_0].current_request, 0);

    switch (dfwk_request->RequestType)
    {
    case DMA_REQUEST_SINGLE_ASYNC:
        DMA_LOG_INFO("Start DMA transfer on channel 0\n");
        dma_start_transfer(device, CH_0, dfwk_request);
        break;
    default:
        // Unsupported request type.
        BUG_ASSERT(false);
        break;
    }

    // Note: Completion notification will either be polled via timers or an interrupt
}

void dma_channel_1_queue_dispatch(PDFWK_ASYNC_REQUEST_HEADER dfwk_request, void* context)
{
    pdma_device_t device = (pdma_device_t)context;

    // Double check that the channel is free
    BUG_ASSERT_PARAM(device->Channel[CH_1].current_request == NULL, device->Channel[CH_1].current_request, 0);

    switch (dfwk_request->RequestType)
    {
    case DMA_REQUEST_SINGLE_ASYNC:
        DMA_LOG_INFO("Start DMA transfer on channel 1\n");
        dma_start_transfer(device, CH_1, dfwk_request);
        break;
    default:
        // Unsupported request type.
        BUG_ASSERT(false);
        break;
    }

    // Note: Completion notification will either be polled via timers or an interrupt
}

void update_channel_struct_add_remaining_bytes(pdma_device_t device, pdma_async_request_t request)
{
    /*
       REMEMBER: This is called from the either the timer thread or DMA ISR - not DFWK.
       Need to mind thread safety.  (FpFwLock) Critical sections are implemented to protect against vampires
    */

    FPFW_LOCK_STATE lock_state;

    // Update the channel's total remaining bytes to transfer for all requests - not thread safe
    int channel = request->dma_private.assigned_channel;
    lock_state = FpFwLockAcquire(&device->Channel[channel].lock);

    device->Channel[channel].remaining_bytes_all_requests += request->input.byte_count;

    FpFwLockRelease(&device->Channel[channel].lock, lock_state);
}

void update_channel_struct_subtract_remaining_bytes(pdma_device_t device, pdma_async_request_t request)
{
    /*
       REMEMBER: This is called from the either the timer thread or DMA ISR - not DFWK.
       Need to mind thread safety.  (FpFwLock) Critical sections are implemented to ward off evil spirits
    */

    static size_t previous_remaining_bytes_all_requests = 0;
    static bool first_time = true;
    FPFW_LOCK_STATE lock_state;
    int channel = request->dma_private.assigned_channel;

    // Update the channel's total remaining bytes to transfer for all requests - not thread safe
    lock_state = FpFwLockAcquire(&device->Channel[channel].lock);
    device->Channel[channel].remaining_bytes_all_requests -= request->input.byte_count;

    // Check for negative remaining bytes
    if ((device->Channel[channel].remaining_bytes_all_requests > previous_remaining_bytes_all_requests) && !first_time)
    {
        DMA_LOG_INFO("Error: queue remaining bytes is negative\n");
        FPFwErrorRaise(FPFW_ERROR_DMA_REMAINING_BYTES_INVALID, 0, 0, 0, 0);
    }
    previous_remaining_bytes_all_requests = device->Channel[channel].remaining_bytes_all_requests;
    first_time = false;

    FpFwLockRelease(&device->Channel[channel].lock, lock_state);
}

bool check_parameters(pdma_device_t device, pdma_async_request_t request)
{
    // Check that the request is not NULL
    if (request == NULL)
    {
        // Invalid request
        DMA_LOG_INFO("Invalid request\n");
        return false;
    }

    // Check that number of bytes > 0
    if (request->input.byte_count == 0)
    {
        // Invalid byte count
        DMA_LOG_INFO("Invalid byte count: %u\n", request->input.byte_count);
        return false;
    }

    // Check src address is valid
    if (!is_valid_src_addr(device, request->input.src_addr_32b_aligned))
    {
        // Invalid source address
        DMA_LOG_INFO("Invalid src addr: 0x%X\n", (int)request->input.src_addr_32b_aligned);
        return false;
    }

    // Check dest address is 32b aligned
    if ((request->input.dest_addr_32b_aligned & 0x3) != 0)
    {
        // Invalid destination address
        DMA_LOG_INFO("Dest addr !32b aligned: 0x%llX\n", request->input.dest_addr_32b_aligned);
        return false;
    }

    // Check that the request is not NULL
    if (request->input.dest_addr_32b_aligned == 0)
    {
        // Invalid destination address
        DMA_LOG_INFO("Invalid dest addr: 0x%llX\n", request->input.dest_addr_32b_aligned);
        return false;
    }

    return true;
}

bool is_valid_src_addr(pdma_device_t device, uint32_t src_addr_32b_aligned)
{
    // if SCP, assign inst_ram_addr to SCP_TOP_SCP_INST_RAM_ADDRESS, otherwise assign to MCP_TOP_MCP_INST_RAM_ADDRESS
    uint32_t inst_ram_addr = device->config->cpu_type == CPU_SCP ? SCP_TOP_SCP_INST_RAM_ADDRESS : MCP_TOP_MCP_INST_RAM_ADDRESS;
    uint32_t inst_ram_size = device->config->cpu_type == CPU_SCP ? SCP_TOP_SCP_INST_RAM_SIZE : MCP_TOP_MCP_INST_RAM_SIZE;
    uint32_t data_ram_addr = device->config->cpu_type == CPU_SCP ? SCP_TOP_SCP_DATA_RAM_ADDRESS : MCP_TOP_MCP_DATA_RAM_ADDRESS;
    uint32_t data_ram_size = device->config->cpu_type == CPU_SCP ? SCP_TOP_SCP_DATA_RAM_SIZE : MCP_TOP_MCP_DATA_RAM_SIZE;

    // Check that the source address is not 0
    if (src_addr_32b_aligned == 0)
    {
        return false;
    }

    // Check that the source address is not within instruction RAM
    if (src_addr_32b_aligned > inst_ram_addr && src_addr_32b_aligned < (inst_ram_addr + inst_ram_size))
    {
        return false;
    }

    // Check that the source address is not within data RAM
    if (src_addr_32b_aligned > data_ram_addr && src_addr_32b_aligned < (data_ram_addr + data_ram_size))
    {
        return false;
    }

    // Check that the source address is 32bit aligned
    if ((src_addr_32b_aligned & 0x3) != 0)
    {
        return false;
    }

    return true;
}