//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dma.c
 *    DMA driver top level implementation.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>   // for DfwkClientInterfaceOpen
#include <DfwkDriver.h>   // for DfwkInterfaceInitialize, DfwkQueueInitialize
#include <DfwkHost.h>     // for DfwkDeviceInitialize
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FPFwInterrupts.h>
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <dma_dfwk.h>
#include <dma_private.h>
#include <dmac.h> // for Silicon Libs DMAC library
#include <dw_axi_dmac_regs.h>
#include <fpfw_init.h> // for fpfw_init_get_handle
#include <idsw_kng.h>  // for idsw_get_cpu_type
#include <interrupts.h>
#include <kng_error.h> // for KNG_E_INVALIDARG, KNG_E_NOTIMPL
#include <nvic.h>      // for nvic_irq_clear_pending, nvic_irq_enable, nvic...
#include <stdint.h>    // for uint32_t
#include <stdio.h>
#include <tx_api.h> // for TX_NO_TIME_SLICE, TX_SUCCESS, TX_WAIT_FOREVER, ...

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void dma_complete_request(dma_device_t* device, uint32_t channel)
{
    /*
       REMEMBER: This is called from the either the timer thread or DMA ISR - not DFWK.
       Need to mind thread safety.  Critical sections are implemented in
       `update_channel_struct_subtract_remaining_bytes`
    */

    // If completed, have dfwk process the callback
    pdma_async_request_t request = device->Channel[channel].current_request;
    update_channel_struct_subtract_remaining_bytes(device, request);

    DfwkAsyncRequestComplete(&(request->header));
    device->Channel[channel].current_request = NULL;
}

void dma_device_init(dma_device_t* device, dma_config_t* pconfig, DFWK_SCHEDULE* schedule)
{
    // Initialize the Queues
    DfwkQueueInitialize(&device->Request_Queue, &device->Header, dma_main_queue_dispatch, &device->Header, DfwkQueueType_ImmediateDispatch);
    DfwkQueueInitialize(&device->Channel[0].queue, &device->Header, dma_channel_0_queue_dispatch, &device->Header, DfwkQueueType_SerializedDispatch);
    DfwkQueueInitialize(&device->Channel[1].queue, &device->Header, dma_channel_1_queue_dispatch, &device->Header, DfwkQueueType_SerializedDispatch);

    // Copy config parameters to the device
    device->config = pconfig;
    device->config->base_address =
        (uintptr_t)DMA_BASE_ADDR; // Defined in CMakeLists - will have correct base addr for SCP or MCP
    device->config->cpu_type = idsw_get_cpu_type();

    // Set up ASYNC handling depending on config type (polling/interrupt) for both channels
    switch (pconfig->config_type)
    {
    case DMA_CONFIG_TYPE_POLLING: {
        uint32_t status = initialize_tx_timer(device, CH_0);
        if (TX_SUCCESS != status)
        {
            FPFwErrorRaise(FPFW_ERROR_DMA_TIMER_ERROR, CH_0, status, 0, 0); // channel, status, unique instance, unused
        }

        status = initialize_tx_timer(device, CH_1);
        if (TX_SUCCESS != status)
        {
            FPFwErrorRaise(FPFW_ERROR_DMA_TIMER_ERROR, CH_1, status, 1, 0); // channel, status, unique instance, unused
        }
        break;
    }
    case DMA_CONFIG_TYPE_INTERRUPT: {
        DMA_LOG_INFO("Enabling DMA interrupts\n");
        dma_enable_nvic_interrupts(device);
        break;
    }
    default:
        // No other configuration type is supported
        FPFW_RUNTIME_ASSERT(false);
    }

    // Initialize each channel's lock for thread safety when modifying remaining bytes
    FpFwLockInitialize(&device->Channel[CH_0].lock);
    FpFwLockInitialize(&device->Channel[CH_1].lock);

    DMA_LOG_INFO("Initializing DMA device\n");
    DfwkDeviceInitialize(&device->Header, schedule);
}

void dma_interface_init(dma_device_t* device, dma_interface_t* driver_interface)
{
    DMA_LOG_INFO("Initializing DMA interface\n");
    DfwkInterfaceInitialize(&driver_interface->Header, &device->Header, &device->Request_Queue, NULL);
    driver_interface->Device = device;
    DfwkClientInterfaceOpen(&driver_interface->Header);
}

void dma_lib_init(dma_device_t* device)
{
    uint64_t DMAC_INT_ALL = 0xFFFFFFFFFFFFFFFF;

    // Task 2015102: Update DMA interrupt handling with latest details from architecture team
    // TODO: interrupt enable registers of DMAC
    // From pg. 75 of DW_axi_dmac_databook:
    //      The channel block transfer completion interrupt is enabled
    //      (CHx_InStatus_EnableReg.Enable_BLOCK_TFR_DONE_IntStat = 1 AND
    //      CHx_IntSignal_EnableReg. Enable_BLOCK_TFR_DONE_IntSignal = 1 AND
    //      CHx_CTL.IOC_BLKTFR = 1)

    DMA_LOG_INFO("Initializing DMAC lib\n");
    dmac_init(device->config->base_address, DMAC_RESET_TIMEOUT);

    DMA_LOG_INFO("Enabling DMAC interrupts\n");
    dmac_clear_common_interrupts(device->config->base_address, DMAC_INT_ALL);
    dmac_enable_common_interrupts(device->config->base_address, DMAC_INT_ALL);

    dmac_clear_ch_interrupts(device->config->base_address, DMAC_INT_ALL, CH_0);
    dmac_clear_ch_interrupts(device->config->base_address, DMAC_INT_ALL, CH_1);

    dmac_enable_ch_interrupts(device->config->base_address, DMAC_ENABLED_CH_INTS_MASK, CH_0);
    dmac_enable_ch_interrupts(device->config->base_address, DMAC_ENABLED_CH_INTS_MASK, CH_1);
}

/**
 * @brief Enable DMA interrupts with NVIC
 *
 */
void dma_enable_nvic_interrupts(dma_device_t* device)
{
    uint32_t intr_status = 0;
    static device_identifier_pair_t device_interrupt_pair[NUM_DMA_INTERRUPTS] = {0};

    // SCP and MCP have different interrupt vectors - Defined in CMakelists.txt
    for (uint32_t interrupt_id = DMA_FIRST_IRQ; interrupt_id <= DMA_LAST_IRQ; interrupt_id++)
    {
        device_interrupt_pair[interrupt_id - DMA_FIRST_IRQ].device = device;
        device_interrupt_pair[interrupt_id - DMA_FIRST_IRQ].interrupt_id = interrupt_id;

        intr_status = FPFwCoreInterruptRegisterCallback(interrupt_id,
                                                        (FPFwCoreInterruptHandler)dma_isr,
                                                        (void*)&device_interrupt_pair[interrupt_id - DMA_FIRST_IRQ]);
        if (intr_status != 0)
        {
            DMA_LOG_INFO("Register interrupt callback failed %d\n", (int)interrupt_id);
        }

        intr_status |= FPFwCoreInterruptEnableVector(interrupt_id);
        if (intr_status != 0)
        {
            DMA_LOG_INFO("Failed to enable interrupt %d\n", (int)interrupt_id);
        }

        FPFW_RUNTIME_ASSERT(intr_status == 0);
    }
}
