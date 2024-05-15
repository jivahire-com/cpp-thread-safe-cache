//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_driver.c
 * This file contains the implementation of the AVS module driver
 * interface and related functionality.
 */

/*------------- Includes -----------------*/
#include <DfwkDriver.h>
#include <DfwkHost.h> // for DfwkDeviceInitialize
#include <DfwkThreadXHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <arm_intrinsic.h>
#include <fpfw_init.h>
#include <nvic.h>
#include <scp_avs.h>
#include <scp_avs_cli.h>
#include <scp_avs_driver.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
// Maximum of 16 commands programmed into command memory (// AVSBus Master APB IP Databook | [Link](https://dev.azure.com/ms-tsd/Cedar_Crest/_git/pi_3rd_party_avs?path=/src/avs_top/docs/sdvt_avsbus_master_apb_microsoft_iip_databook.pdf))
#define AVS_CMD_BUFF_SIZE 16
/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

/*
 * An IRQ is triggered if the transaction has been completed successfully or
 * if the transaction has been aborted.
 */
void scp_avs_isr(void* context)
{
    pscp_avs_device device = (pscp_avs_device)context;
    int status = SILIBS_SUCCESS;

    // TODO (https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484968) check for errors.
    status = avs_clear_interrupt_status(device->avs_bus_num, device->config.avs_irq);

    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);
    device->isr_request.outstanding_client_request = device->outstanding_request;
    DfwkQueueEnqueueRequest(&device->avs_isr_resp_queue, (PDFWK_ASYNC_REQUEST_HEADER)&device->isr_request);
    __DSB();
}

void scp_avs_dispatch(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    FPFW_UNUSED(Context);
    pscp_avs_interface Interface = (pscp_avs_interface)Request->OwningInterface;
    pscp_avs_device device = Interface->Device;               // Device associated with this request
    pscp_avs_request avs_request = (pscp_avs_request)Request; // this has the cmd_type, rail, etc.
    int status = SILIBS_SUCCESS;

    avs_master_command_t avs_buffer[AVS_CMD_BUFF_SIZE] = {};

    avs_master_command_t* avs_command = (avs_master_command_t*)&avs_buffer;
    avs_command[0].command_data_type = avs_request->avs_params.cmd_type; // used for single reads/writes.
    avs_command[0].command_type = avs_request->avs_params.rail_id;
    avs_command[0].command_group = AVS_CGROUP;

    printf("\n scp_avs_dispatch\n");

    switch (Request->RequestType)
    {
    case AVS_REQUEST_READ_DATA:
        device->outstanding_request = avs_request;
        printf("\n AVS_REQUEST_READ_DATA, avs_bus = %x, rail = %x, cmd = %x\n",
               device->avs_bus_num,
               avs_request->avs_params.rail_id,
               avs_request->avs_params.cmd_type);
        avs_command[0].command_control = AVS_CMD_READ;

        status = avs_send_cmd_frame(device->avs_bus_num, 1, avs_command);
        break;

    case AVS_REQUEST_WRITE_DATA:
        avs_command[0].command_control = AVS_CMD_WRITE_COMMIT;
        device->outstanding_request = avs_request;
        // This is where the actual write to the AVS goes.
        // int avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t *cmd_mem)
        break;

    case AVS_REQUEST_READ_ALL_VCT:
        device->outstanding_request = avs_request;
        avs_command[0].command_control = AVS_CMD_READ;
        // This is where the actual read all to the AVS goes.
        // int avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t *cmd_mem)
        break;

    case AVS_REQUEST_READ_MULTI:
        device->outstanding_request = avs_request;
        avs_command[0].command_control = AVS_CMD_READ;
        // This is where the actual read multi to the AVS goes.
        // int avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t *cmd_mem)
        break;

    case AVS_REQUEST_WRITE_MULTI:
        device->outstanding_request = avs_request;
        avs_command[0].command_control = AVS_CMD_WRITE_COMMIT;
        // This is where the actual write multi to the AVS goes.
        // int avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t *cmd_mem)
        break;

    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);
}

void scp_avs_isr_dispatch(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    pscp_avs_request original_request = ((pscp_avs_isr_request)Request)->outstanding_client_request;
    pscp_avs_interface Interface = (pscp_avs_interface)original_request->Header.OwningInterface;
    pscp_avs_device device = Interface->Device; // Device associated with this request

    FPFW_UNUSED(Context);
    printf("\n scp_avs_isr_dispatch, RequestType = %x\n", (uint8_t)Request->RequestType);

    uint32_t scp_avs_resp_buf[AVS_CMD_BUFF_SIZE];
    uint32_t scp_avs_resp_idx = 0;
    uint32_t scp_avs_resp_num = 0;

    switch (original_request->Header.RequestType)
    {
    case AVS_REQUEST_READ_DATA:
        scp_avs_resp_idx = 0;
        scp_avs_resp_num = 1;
        avs_get_cmd_resp_data(device->avs_bus_num, scp_avs_resp_idx, scp_avs_resp_num, scp_avs_resp_buf);
        break;

    case AVS_REQUEST_WRITE_DATA:
    case AVS_REQUEST_READ_ALL_VCT:
    case AVS_REQUEST_READ_MULTI:
    case AVS_REQUEST_WRITE_MULTI:
        break;

    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    for (uint32_t j = scp_avs_resp_idx; j < scp_avs_resp_num; j++)
    {
        printf(" AVS data %02x: %08x\n", (int)j, ((int)scp_avs_resp_buf[j] & 0xFFFF));
    }
    DfwkAsyncRequestComplete(&original_request->Header);
}

int32_t scp_avs_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER Request)
{
    switch (Request->RequestType)
    {
    case AVS_GET_ERROR_COUNTS:
        // TODO: (https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484968) Get error counts for the client.
        break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    printf("\n scp_avs_dispatch_sync\n");
    return 0; //(DFWK_SUCCESS);
}

void scp_avs_driver_initialize(pscp_avs_device Device)
{
    nvic_status_t status;

    // Set up the queue for each driver, based on the driver config. Any event that is put on the queue will call scp_avs_dispatch.
    DfwkQueueInitialize(&Device->avs_queue, &Device->Header, scp_avs_dispatch, &Device->Header, DfwkQueueType_SerializedDispatch);
    // Set up the queue for each driver to hold the AVS data read.
    DfwkQueueInitialize(&Device->avs_isr_resp_queue, &Device->Header, scp_avs_isr_dispatch, &Device->Header, DfwkQueueType_SerializedDispatch);

    avs_init((uint32_t)Device->avs_bus_num);

    printf("\nAVS bus num =  %d, AVS IRQ =  %d \n", Device->avs_bus_num, Device->config.avs_irq);

    status = nvic_irq_set_isr_with_param(Device->config.avs_irq, scp_avs_isr, Device);
    FPFW_RUNTIME_ASSERT(status == NVIC_STATUS_SUCCESS);

    status = nvic_irq_clear_pending(Device->config.avs_irq);
    FPFW_RUNTIME_ASSERT(status == NVIC_STATUS_SUCCESS);

    status = nvic_irq_enable(Device->config.avs_irq);
    FPFW_RUNTIME_ASSERT(status == NVIC_STATUS_SUCCESS);

    avs_enable_interrupt((uint32_t)Device->avs_bus_num, (uint32_t)Device->config.avs_irq);
}

void scp_avs_interface_initialize(pscp_avs_device Device, pscp_avs_interface Interface)
{
    DfwkInterfaceInitialize(&Interface->Header, &Device->Header, &Device->avs_queue, scp_avs_dispatch_sync);
    Interface->Device = Device;
    Interface->Device->avs_response_errors = 0;
    DfwkAsyncRequestInititalize((PDFWK_ASYNC_REQUEST_HEADER)&Device->isr_request, sizeof(Device));
}

// Completion routine for an async request.
void scp_request_completion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(CompletionContext);
    FPFW_UNUSED(Request);
}

void scp_avs_init(pscp_avs_device avsDevice, DFWK_SCHEDULE* scheduler)
{
    DfwkDeviceInitialize(&avsDevice->Header, scheduler);
    scp_avs_driver_initialize(avsDevice);
}