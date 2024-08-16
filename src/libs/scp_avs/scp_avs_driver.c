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

    /**
     * 1. Get the interrupt status
     * 2. Clear the CMD DONE interrupt status (will clear it even if not set).
     * 3. If the CMD DONE interrupt status bit is set enqueue the request
     *
     * TODO: Update status handling for possible statuses
     *       ADO: (https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484968)
     */

    uint32_t intr_status = 0;
    int status = avs_get_interrupt_status(device->avs_bus_num, &intr_status);
    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);

    status = avs_clear_interrupt_status(device->avs_bus_num, AVS_IRQ_CMD_DONE);
    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);

    if ((intr_status & (AVS_IRQ_CMD_DONE)))
    {
        device->isr_request.outstanding_client_request = device->outstanding_request;
        DfwkQueueEnqueueRequest(&device->avs_isr_resp_queue, (PDFWK_ASYNC_REQUEST_HEADER)&device->isr_request);
    }

    __DSB();
}

void scp_avs_dispatch(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    FPFW_UNUSED(Context);
    pscp_avs_interface_t Interface = (pscp_avs_interface_t)Request->OwningInterface;
    pscp_avs_device device = Interface->Device;               // Device associated with this request
    pscp_avs_request avs_request = (pscp_avs_request)Request; // this has the cmd_type, rail, etc.
    int status = SILIBS_SUCCESS;

    avs_master_command_t avs_buffer[AVS_CMD_BUFF_SIZE] = {};
    uint32_t scp_avs_cmd_num = 0; // Entry number in the command frame array.

    avs_buffer[0].command_data_type = avs_request->avs_params.avs_cmd_info.cmd_type;
    avs_buffer[0].command_type = avs_request->avs_params.avs_cmd_info.rail_id;

    avs_buffer[0].command_group = AVS_CGROUP;

    AVS_LOG_TRACE("scp_avs_dispatch");

    switch (Request->RequestType)
    {
    case AVS_REQUEST_READ_DATA:
        scp_avs_cmd_num = 1;
        device->outstanding_request = avs_request;
        avs_buffer[0].command_control = AVS_CMD_READ;

        status = avs_send_cmd_frame(device->avs_bus_num, scp_avs_cmd_num, avs_buffer);
        break;

    case AVS_REQUEST_WRITE_DATA:
        scp_avs_cmd_num = 0;
        device->outstanding_request = avs_request;
        avs_buffer[scp_avs_cmd_num].command_control = AVS_CMD_WRITE_COMMIT;
        avs_buffer[scp_avs_cmd_num].command_data = avs_request->avs_params.avs_data;

        scp_avs_cmd_num++;
        avs_buffer[scp_avs_cmd_num].command_type = avs_request->avs_params.avs_cmd_info.rail_id;
        avs_buffer[scp_avs_cmd_num].command_data_type = AVS_VOLTAGE_RW;
        avs_buffer[scp_avs_cmd_num].command_group = AVS_CGROUP;
        avs_buffer[scp_avs_cmd_num].command_control = AVS_CMD_READ;

        scp_avs_cmd_num++; // two commands sent, write then read to verify data written

        status = avs_send_cmd_frame(device->avs_bus_num, scp_avs_cmd_num, avs_buffer);
        break;

    case AVS_REQUEST_READ_ALL_VCT:
        scp_avs_cmd_num = 0;
        device->outstanding_request = avs_request;
        avs_buffer[scp_avs_cmd_num].command_type = avs_request->avs_params.avs_cmd_info.rail_id;
        avs_buffer[scp_avs_cmd_num].command_data_type = AVS_VOLTAGE_RW;
        avs_buffer[scp_avs_cmd_num].command_group = AVS_CGROUP;
        avs_buffer[scp_avs_cmd_num].command_control = AVS_CMD_READ;

        scp_avs_cmd_num++;
        avs_buffer[scp_avs_cmd_num].command_type = avs_request->avs_params.avs_cmd_info.rail_id;
        avs_buffer[scp_avs_cmd_num].command_data_type = AVS_CURRENT_READ;
        avs_buffer[scp_avs_cmd_num].command_group = AVS_CGROUP;
        avs_buffer[scp_avs_cmd_num].command_control = AVS_CMD_READ;

        scp_avs_cmd_num++;
        avs_buffer[scp_avs_cmd_num].command_type = avs_request->avs_params.avs_cmd_info.rail_id;
        avs_buffer[scp_avs_cmd_num].command_data_type = AVS_TEMPERATURE_READ;
        avs_buffer[scp_avs_cmd_num].command_group = AVS_CGROUP;
        avs_buffer[scp_avs_cmd_num].command_control = AVS_CMD_READ;

        scp_avs_cmd_num++; // three commands sent (voltage, current, temperature)

        status = avs_send_cmd_frame(device->avs_bus_num, scp_avs_cmd_num, avs_buffer);
        break;

    case AVS_REQUEST_READ_MULTI:
        device->outstanding_request = avs_request;
        scp_avs_cmd_num = avs_request->avs_params.cmd_count;

        for (uint8_t index = 0; index < scp_avs_cmd_num; index++)
        {
            avs_buffer[index].command_type = avs_request->avs_params.avs_cmd_array[index].rail_id;
            avs_buffer[index].command_data_type = avs_request->avs_params.avs_cmd_array[index].cmd_type;
            avs_buffer[index].command_group = AVS_CGROUP;
            avs_buffer[index].command_control = AVS_CMD_READ;
        }
        AVS_LOG_TRACE("Read multi, 0x%0x commands", (uint8_t)scp_avs_cmd_num);
        status = avs_send_cmd_frame(device->avs_bus_num, scp_avs_cmd_num, avs_buffer);
        break;

    case AVS_REQUEST_WRITE_MULTI:
        // AVS voltage write of both rails
        device->outstanding_request = avs_request;
        uint8_t index;
        uint8_t rail;
        for (index = 0, rail = AVS_RAIL0; rail < MAX_AVS_RAILS; index++, rail++)
        {
            avs_buffer[index].command_type = rail;
            avs_buffer[index].command_data_type = AVS_VOLTAGE_RW; // cmd_type
            avs_buffer[index].command_data = avs_request->avs_resp_multi.avs_response_multi[index].data;

            avs_buffer[index].command_group = AVS_CGROUP;
            avs_buffer[index].command_control = AVS_CMD_WRITE_COMMIT;
            AVS_LOG_TRACE("Write multi, %d data", (int16_t)avs_buffer[index].command_data);
        }
        status = avs_send_cmd_frame(device->avs_bus_num, MAX_AVS_RAILS, avs_buffer);
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
    pscp_avs_interface_t Interface = (pscp_avs_interface_t)original_request->Header.OwningInterface;
    pscp_avs_device device = Interface->Device; // Device associated with this request

    FPFW_UNUSED(Context);
    AVS_LOG_TRACE("scp_avs_isr_dispatch, RequestType = %x", (uint8_t)Request->RequestType);

    int status = SILIBS_SUCCESS;
    original_request->avs_response_status = SCP_AVS_STATUS_SUCCESS; // this will be sent to the client
    uint32_t scp_avs_resp_buf[AVS_CMD_BUFF_SIZE];
    uint32_t scp_avs_resp_idx = 0; // index to read from the command response buffer
    uint32_t scp_avs_resp_num = 0; // count to read from the command response buffer

    switch (original_request->Header.RequestType)
    {
    case AVS_REQUEST_READ_DATA:
        scp_avs_resp_idx = 0; // for single reads this will always be zero.
        scp_avs_resp_num = 1; // for single reads this will always be one, to indicate only one response is to be read
        status = avs_get_cmd_resp_data(device->avs_bus_num, scp_avs_resp_idx, scp_avs_resp_num, scp_avs_resp_buf);

        if (status != SILIBS_SUCCESS)
        {
            AVS_LOG_CRIT("avs_get_cmd_resp_data failure (Read)!");
            original_request->avs_response_status = SCP_AVS_STATUS_READ_FAIL;
        }
        else
        {
            // Populate the data to be sent to the client.
            original_request->avs_response_single_resp = (int16_t)(scp_avs_resp_buf[scp_avs_resp_idx] & 0xFFFF);
            AVS_LOG_TRACE("Raw data read: 0x%0x", (int16_t)scp_avs_resp_buf[scp_avs_resp_idx]);
        }
        break;

    case AVS_REQUEST_WRITE_DATA:
        scp_avs_resp_idx = 0;
        scp_avs_resp_num = 2; // setting to 2 since two commands (1st is the write command, 2nd is the read of the data to verify)
        status = avs_get_cmd_resp_data(device->avs_bus_num, scp_avs_resp_idx, scp_avs_resp_num, scp_avs_resp_buf);

        if (status != SILIBS_SUCCESS)
        {
            AVS_LOG_CRIT("avs_get_cmd_resp_data (Write) failure!");
            original_request->avs_response_status = SCP_AVS_STATUS_WRITE_FAIL;
        }
        else
        {
            // Populate the data to be sent to the client. scp_avs_resp_buf[0] contains the write status.
            // scp_avs_resp_buf[1] contains the data from the read which was executed immediately after the write.
            // So scp_avs_resp_buf[(scp_avs_resp_idx + 1)] is sent to the client, which should match the value of the voltage written.
            original_request->avs_response_single_resp = (int16_t)(scp_avs_resp_buf[(scp_avs_resp_idx + 1)] & 0xFFFF);
            AVS_LOG_TRACE("Raw data read after AVS write: 0x%0x", (int16_t)scp_avs_resp_buf[1]);
        }
        break;

    case AVS_REQUEST_READ_ALL_VCT:
        scp_avs_resp_idx = 0;
        scp_avs_resp_num = 3; // Indicates the number of values read: voltage, current, and temperature.
        status = avs_get_cmd_resp_data(device->avs_bus_num, scp_avs_resp_idx, scp_avs_resp_num, scp_avs_resp_buf);

        if (status != SILIBS_SUCCESS)
        {
            AVS_LOG_CRIT("avs_get_cmd_resp_data failure (Read VCT)!");
            original_request->avs_response_status = SCP_AVS_STATUS_READ_ALL_VCT_FAIL;
        }
        else
        {
            original_request->avs_response_vct.voltage_mV = (int16_t)(scp_avs_resp_buf[scp_avs_resp_idx++] & 0xFFFF);
            original_request->avs_response_vct.current_cA = (int16_t)(scp_avs_resp_buf[scp_avs_resp_idx++] & 0xFFFF);
            original_request->avs_response_vct.temperature_dC = (int16_t)(scp_avs_resp_buf[scp_avs_resp_idx] & 0xFFFF);
            AVS_LOG_TRACE("Read raw data VCT. \n  Volt: 0x%0x, Cur: 0x%0x, Temp: 0x%0x",
                          (int16_t)scp_avs_resp_buf[0],
                          (int16_t)scp_avs_resp_buf[1],
                          (int16_t)scp_avs_resp_buf[2]);
        }
        break;

    case AVS_REQUEST_READ_MULTI:
        scp_avs_resp_idx = 0;
        scp_avs_resp_num = original_request->avs_params.cmd_count;

        status = avs_get_cmd_resp_data(device->avs_bus_num, scp_avs_resp_idx, scp_avs_resp_num, scp_avs_resp_buf);

        if (status != SILIBS_SUCCESS)
        {
            AVS_LOG_CRIT("avs_get_cmd_resp_data failure (Read multi)!");
            original_request->avs_response_status = SCP_AVS_STATUS_READ_MULTI_FAIL;
        }
        else
        {
            AVS_LOG_TRACE("Read multi, AVSBus = %d", device->avs_bus_num);
            for (scp_avs_resp_idx = 0; scp_avs_resp_idx < scp_avs_resp_num; scp_avs_resp_idx++)
            {
                original_request->avs_resp_multi.avs_response_multi[scp_avs_resp_idx].data =
                    (int16_t)(scp_avs_resp_buf[scp_avs_resp_idx] & 0xFFFF);
                AVS_LOG_TRACE("data[%d]: 0x%0x",
                              (int16_t)scp_avs_resp_idx,
                              original_request->avs_resp_multi.avs_response_multi[scp_avs_resp_idx].data);
            }
        }
        break;

    case AVS_REQUEST_WRITE_MULTI:
        scp_avs_resp_idx = 0;
        scp_avs_resp_num = 1;

        status = avs_get_cmd_resp_data(device->avs_bus_num, scp_avs_resp_idx, scp_avs_resp_num, scp_avs_resp_buf);

        if (status != SILIBS_SUCCESS)
        {
            AVS_LOG_CRIT("avs_get_cmd_resp_data failure (Write multi)!");
            original_request->avs_response_status = SCP_AVS_STATUS_WRITE_MULTI_FAIL;
        }
        break;

    default:
        break;
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

    AVS_LOG_TRACE("scp_avs_dispatch_sync");
    return 0; //(DFWK_SUCCESS);
}

void scp_avs_driver_initialize(pscp_avs_device Device)
{

    // Set up the queue for each driver, based on the driver config. Any event that is put on the queue will call scp_avs_dispatch.
    DfwkQueueInitialize(&Device->avs_queue, &Device->Header, scp_avs_dispatch, &Device->Header, DfwkQueueType_SerializedDispatch);
    // Set up the queue for each driver to hold the AVS data read.
    DfwkQueueInitialize(&Device->avs_isr_resp_queue, &Device->Header, scp_avs_isr_dispatch, &Device->Header, DfwkQueueType_SerializedDispatch);

    avs_init((uint32_t)Device->avs_bus_num, NULL);

    AVS_LOG_INFO("AVS bus num =  %d, IRQ =  %d", Device->avs_bus_num, Device->config.avs_irq);

    // Configure the NVIC Handling
    nvic_status_t status = nvic_irq_set_isr_with_param(Device->config.avs_irq, scp_avs_isr, Device);
    FPFW_RUNTIME_ASSERT(status == NVIC_STATUS_SUCCESS);

    status = nvic_irq_clear_pending(Device->config.avs_irq);
    FPFW_RUNTIME_ASSERT(status == NVIC_STATUS_SUCCESS);

    status = nvic_irq_enable(Device->config.avs_irq);
    FPFW_RUNTIME_ASSERT(status == NVIC_STATUS_SUCCESS);

    // Configure the AVS HW to fire the ISR only on CMD ONE
    FPFW_RUNTIME_ASSERT(avs_enable_interrupt(Device->avs_bus_num, AVS_IRQ_CMD_DONE) == SILIBS_SUCCESS);
}

void scp_avs_interface_initialize(pscp_avs_device Device, pscp_avs_interface_t Interface)
{
    DfwkInterfaceInitialize(&Interface->Header, &Device->Header, &Device->avs_queue, scp_avs_dispatch_sync);
    Interface->Device = Device;
    Interface->Device->avs_response_errors = 0;
    DfwkAsyncRequestInititalize((PDFWK_ASYNC_REQUEST_HEADER)&Device->isr_request, sizeof(Device));
}

void scp_avs_init(pscp_avs_device avsDevice, DFWK_SCHEDULE* scheduler)
{
    DfwkDeviceInitialize(&avsDevice->Header, scheduler);
    scp_avs_driver_initialize(avsDevice);
}