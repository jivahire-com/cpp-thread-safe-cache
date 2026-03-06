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
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <arm_intrinsic.h>
#include <bug_check.h>
#include <fpfw_init.h>
#include <nvic.h>
#include <padring_southeast_regs.h>
#include <power_runconfig.h>
#include <scp_avs.h>
#include <scp_avs_cli.h>
#include <scp_avs_driver.h>
#include <scp_avs_events.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/
scp_avs_error_count_t avs_error_count = {0};

avs_error_t scp_avs_status_error(uint32_t resp_data)
{
    // bits 21 and 22 = target ACK
    // bits 16 - 20 = status response. (VDone bit 20, StatusAlert bit 19, AVS_Control bit 18, MfrSpec 17 and
    // 16) bits 0 - 15 = read data

    avs_error_t scp_avs_error = {.as_uint8 = AVS_ERROR_NONE};

    // Check for VDone status. 0 when the rail is off, powering up, or ramping to target.  1 when when voltage has reached target.
    if ((resp_data >> AVS_VDONE_SHIFT) & AVS_STATUS_MASK)
    {
        scp_avs_error.v_done = 1;
    }
    if ((resp_data >> AVS_ERR_STAT_ALERT_SHIFT) & AVS_STATUS_MASK)
    { // Check for Status Alert
        scp_avs_error.status_alert = 1;
        avs_error_count.status_alert_error_count++;
    }

    if (((resp_data >> AVS_NO_CONTROL_SHIFT) & AVS_STATUS_MASK) == 0x00)
    { // Check for AVS_Control - bit set to 0 when not controlling AVS output
        scp_avs_error.no_control = 1;
    }

    // Check the target Ack
    if (((resp_data >> AVS_ERR_ACK_SHIFT) & AVS_ERR_ACK_MASK) == 0x01)
    { // Check for Good CRC, but no action taken due to busy
        scp_avs_error.no_action_busy = 1;
        avs_error_count.ack_no_action_busy_error_count++;
    }

    if (((resp_data >> AVS_ERR_ACK_SHIFT) & AVS_ERR_ACK_MASK) == 0x02)
    { // Check for bad CRC, no action taken
        scp_avs_error.bad_crc_no_action = 1;
        avs_error_count.ack_bad_crc_no_action_error_count++;
    }

    if (((resp_data >> AVS_ERR_ACK_SHIFT) & AVS_ERR_ACK_MASK) == 0x03)
    { // Check for 0x11 Good CRC, but invalid
        scp_avs_error.invalid_no_action = 1;
        avs_error_count.ack_invalid_no_action_error_count++;
    }

    AVS_LOG_TRACE("Status Error: 0x%0x", error);
    return (scp_avs_error);
}

void avs_get_error_counts(scp_avs_error_count_t* error_count_resp)
{
    BUG_ASSERT(error_count_resp != NULL);

    // for CLI - grab the error count
    *error_count_resp = avs_error_count;
}

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
     */

    uint32_t intr_status = 0;
    int status = avs_get_interrupt_status(device->avs_bus_num, &intr_status);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

    // Note - the AVS_IRQ_CMD_DONE is also set with the AVS_IRQ_SLV_STATUS_CRC_ERR.
    if ((intr_status & (AVS_IRQ_SLV_STATUS_CRC_ERR)))
    {
        AVS_LOG_WARN("CRC error detected on AVS bus %d", device->avs_bus_num);
        device->isr_request.avs_crc_error = true;
        status = avs_clear_interrupt_status(device->avs_bus_num, AVS_IRQ_SLV_STATUS_CRC_ERR);
        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);
    }

    if ((intr_status & (AVS_IRQ_CMD_DONE)))
    {
        device->isr_request.outstanding_client_request = device->outstanding_request;
        DfwkQueueEnqueueRequest(&device->avs_isr_resp_queue, (PDFWK_ASYNC_REQUEST_HEADER)&device->isr_request);
    }

    status = avs_clear_interrupt_status(device->avs_bus_num, AVS_IRQ_CMD_DONE);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

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
        BUG_ASSERT(false);
        break;
    }

    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);
}

void scp_avs_isr_dispatch(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    pscp_avs_request original_request = ((pscp_avs_isr_request)Request)->outstanding_client_request;
    pscp_avs_isr_request avs_isr_request = (pscp_avs_isr_request)Request;
    pscp_avs_interface_t Interface = (pscp_avs_interface_t)original_request->Header.OwningInterface;
    pscp_avs_device device = Interface->Device; // Device associated with this request

    FPFW_UNUSED(Context);
    AVS_LOG_TRACE("scp_avs_isr_dispatch, RequestType = %x", (uint8_t)Request->RequestType);

    int status = SILIBS_SUCCESS;

    original_request->avs_response_status = SCP_AVS_STATUS_SUCCESS; // this will be sent to the client
    uint32_t scp_avs_resp_buf[AVS_CMD_BUFF_SIZE];
    uint32_t scp_avs_resp_idx = 0; // index to read from the command response buffer
    uint32_t scp_avs_resp_num = 0; // count to read from the command response buffer

    if (avs_isr_request->avs_crc_error)
    {
        // don't modify any data, return error
        AVS_LOG_CRIT("AVS CRC error!");
        FPFW_ET_LOG(ScpAvsCrcError, __LINE__);
        avs_error_count.crc_error_count++;
        original_request->avs_response_status = SCP_AVS_STATUS_CRC_ERROR;
        avs_isr_request->avs_crc_error = false;
        DfwkAsyncRequestComplete(&original_request->Header);
        return;
    }

    switch (original_request->Header.RequestType)
    {
    case AVS_REQUEST_READ_DATA:
        scp_avs_resp_idx = 0; // for single reads this will always be zero.
        scp_avs_resp_num = 1; // for single reads this will always be one, to indicate only one response is to be read
        status = avs_get_cmd_resp_data(device->avs_bus_num, scp_avs_resp_idx, scp_avs_resp_num, scp_avs_resp_buf);

        if (status != SILIBS_SUCCESS)
        {
            AVS_LOG_CRIT("avs_get_cmd_resp_data failure (Read)!");
            FPFW_ET_LOG(ScpAvsDriGetCmdRespReadError, __LINE__);
            original_request->avs_response_status = SCP_AVS_STATUS_READ_FAIL;
        }
        else
        {
            avs_error_t status_error = scp_avs_status_error(scp_avs_resp_buf[scp_avs_resp_idx]);
            // Populate the data to be sent to the client.
            original_request->avs_response_single_resp.error = status_error;
            original_request->avs_response_single_resp.data = (int16_t)(scp_avs_resp_buf[scp_avs_resp_idx] & 0xFFFF);
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
            FPFW_ET_LOG(ScpAvsDriGetCmdRespWriteError, __LINE__);
            original_request->avs_response_status = SCP_AVS_STATUS_WRITE_FAIL;
        }
        else
        {
            // scp_avs_resp_idx = 0 for write, 1 for read, so need to check for both write and read errors.
            // update local status (all bits but AVS_DONE--we'll want the latest from the next response)
            avs_error_t status_error_write = scp_avs_status_error(scp_avs_resp_buf[scp_avs_resp_idx]);
            status_error_write.v_done = 0;

            avs_error_t status_error_read = (scp_avs_status_error(scp_avs_resp_buf[scp_avs_resp_idx + 1]));
            status_error_write.as_uint8 |= status_error_read.as_uint8;

            // Populate the data to be sent to the client. scp_avs_resp_buf[0] contains the write status.
            // scp_avs_resp_buf[1] contains the data from the read which was executed immediately after the write.
            // So scp_avs_resp_buf[(scp_avs_resp_idx + 1)] is sent to the client, which should match the value of the voltage written.
            original_request->avs_response_single_resp.error = status_error_write;
            original_request->avs_response_single_resp.data =
                (int16_t)(scp_avs_resp_buf[(scp_avs_resp_idx + 1)] & 0xFFFF);
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
            FPFW_ET_LOG(ScpAvsDriGetCmdRespReadError, __LINE__);
            original_request->avs_response_status = SCP_AVS_STATUS_READ_ALL_VCT_FAIL;
        }
        else
        {
            avs_error_t status_error = scp_avs_status_error(scp_avs_resp_buf[scp_avs_resp_idx]);
            // Populate the data to be sent to the client.
            original_request->avs_response_vct.error_voltage = status_error;
            original_request->avs_response_vct.voltage_mV = (int16_t)(scp_avs_resp_buf[scp_avs_resp_idx++] & 0xFFFF);

            status_error = scp_avs_status_error(scp_avs_resp_buf[scp_avs_resp_idx]);
            original_request->avs_response_vct.error_current = status_error;
            original_request->avs_response_vct.current_cA = (int16_t)(scp_avs_resp_buf[scp_avs_resp_idx++] & 0xFFFF);

            status_error = scp_avs_status_error(scp_avs_resp_buf[scp_avs_resp_idx]);
            original_request->avs_response_vct.error_temperature = status_error;
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
            FPFW_ET_LOG(ScpAvsDriGetCmdRespReadError, __LINE__);
            original_request->avs_response_status = SCP_AVS_STATUS_READ_MULTI_FAIL;
        }
        else
        {
            AVS_LOG_TRACE("Read multi, AVSBus = %d", device->avs_bus_num);
            for (scp_avs_resp_idx = 0; scp_avs_resp_idx < scp_avs_resp_num; scp_avs_resp_idx++)
            {
                avs_error_t status_error = scp_avs_status_error(scp_avs_resp_buf[scp_avs_resp_idx]);
                original_request->avs_resp_multi.avs_response_multi[scp_avs_resp_idx].error = status_error;

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
            FPFW_ET_LOG(ScpAvsDriGetCmdRespWriteError, __LINE__);
            original_request->avs_response_status = SCP_AVS_STATUS_WRITE_MULTI_FAIL;
        }
        else
        {
            uint8_t rail;
            avs_error_t status_error;
            for (scp_avs_resp_idx = 0, rail = AVS_RAIL0; rail < MAX_AVS_RAILS; scp_avs_resp_idx++, rail++)
            {
                status_error = scp_avs_status_error(scp_avs_resp_buf[scp_avs_resp_idx]);
                original_request->avs_resp_multi.avs_response_multi[scp_avs_resp_idx].error = status_error;
            }
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
        BUG_ASSERT(false);
        break;
    }

    AVS_LOG_TRACE("scp_avs_dispatch_sync");
    return 0; //(DFWK_SUCCESS);
}

void scp_avs_driver_initialize(pscp_avs_device Device)
{
/*
Per AVS SPEC Sec 5.6, need 34 1's to sync slave
*/
#define AVS_RESYNC_LEN (34)

/* Add some delay between commands and resync */
#define AVS_RESYNC_GAP (16)

    // Master Prescaler
    // With i_clk = 200 and n = 3 (n = avs_cfg_default->clk_prescaler)
    // avs_clk = (i_clk/((n+1)*2)) = 25Mhz
    const avs_master_cfg_t avs_cfg_default = {.periodic_rsync_en = true,
                                              .periodic_rsync_len = AVS_RESYNC_LEN,
                                              .periodic_rsync_gap = AVS_RESYNC_GAP,
                                              .clk_prescaler = 3};

    // Set up the queue for each driver, based on the driver config. Any event that is put on the queue will call scp_avs_dispatch.
    DfwkQueueInitialize(&Device->avs_queue, &Device->Header, scp_avs_dispatch, &Device->Header, DfwkQueueType_SerializedDispatch);
    // Set up the queue for each driver to hold the AVS data read.
    DfwkQueueInitialize(&Device->avs_isr_resp_queue, &Device->Header, scp_avs_isr_dispatch, &Device->Header, DfwkQueueType_SerializedDispatch);

    // Set the clk and mdata drive strength based off of the knob configuration.
    uintptr_t avs_afm_csr_clk_address = Device->config.afm_csr_avs_clk_addr;
    uintptr_t avs_afm_csr_mdata_address = Device->config.afm_csr_mdata_addr;

    switch (Device->avs_bus_num)
    {
    case AVS_BUS0:
        volatile padring_southeast_southeast_afm_csr_avs0_clk* afm_clk_addr0 =
            (volatile padring_southeast_southeast_afm_csr_avs0_clk*)avs_afm_csr_clk_address;
        afm_clk_addr0->ds = config_get_power_avs_ds().array[Device->avs_bus_num].clk;
        AVS_LOG_TRACE(" AVS0 Device clk ds = %d \n", afm_clk_addr0->ds);

        volatile padring_southeast_southeast_afm_csr_avs0_mdata* afm_mdata_addr0 =
            (volatile padring_southeast_southeast_afm_csr_avs0_mdata*)avs_afm_csr_mdata_address;
        afm_mdata_addr0->ds = config_get_power_avs_ds().array[Device->avs_bus_num].mdata;
        AVS_LOG_TRACE(" AVS0 Device mdata ds = %d \n", afm_mdata_addr0->ds);
        break;

    case AVS_BUS1:
        volatile padring_southeast_southeast_afm_csr_avs1_clk* afm_clk_addr1 =
            (volatile padring_southeast_southeast_afm_csr_avs1_clk*)avs_afm_csr_clk_address;
        afm_clk_addr1->ds = config_get_power_avs_ds().array[Device->avs_bus_num].clk;
        AVS_LOG_TRACE(" AVS1 Device clk ds = %d \n", afm_clk_addr1->ds);

        volatile padring_southeast_southeast_afm_csr_avs1_mdata* afm_mdata_addr1 =
            (volatile padring_southeast_southeast_afm_csr_avs1_mdata*)avs_afm_csr_mdata_address;
        afm_mdata_addr1->ds = config_get_power_avs_ds().array[Device->avs_bus_num].mdata;
        AVS_LOG_TRACE(" AVS1 Device mdata ds = %d \n", afm_mdata_addr1->ds);
        break;

    case AVS_BUS2:
        volatile padring_southeast_southeast_afm_csr_avs2_clk* afm_clk_addr2 =
            (volatile padring_southeast_southeast_afm_csr_avs2_clk*)avs_afm_csr_clk_address;
        afm_clk_addr2->ds = config_get_power_avs_ds().array[Device->avs_bus_num].clk;
        AVS_LOG_TRACE(" AVS2 Device clk ds = %d \n", afm_clk_addr2->ds);

        volatile padring_southeast_southeast_afm_csr_avs2_mdata* afm_mdata_addr2 =
            (volatile padring_southeast_southeast_afm_csr_avs2_mdata*)avs_afm_csr_mdata_address;
        afm_mdata_addr2->ds = config_get_power_avs_ds().array[Device->avs_bus_num].mdata;
        AVS_LOG_TRACE(" AVS2 Device mdata ds = %d \n", afm_mdata_addr2->ds);
        break;

    case AVS_BUS3:
        volatile padring_southeast_southeast_afm_csr_avs3_clk* afm_clk_addr3 =
            (volatile padring_southeast_southeast_afm_csr_avs3_clk*)avs_afm_csr_clk_address;
        afm_clk_addr3->ds = config_get_power_avs_ds().array[Device->avs_bus_num].clk;
        AVS_LOG_TRACE(" AVS3 Device clk ds = %d \n", afm_clk_addr3->ds);

        volatile padring_southeast_southeast_afm_csr_avs2_mdata* afm_mdata_addr3 =
            (volatile padring_southeast_southeast_afm_csr_avs2_mdata*)avs_afm_csr_mdata_address;
        afm_mdata_addr3->ds = config_get_power_avs_ds().array[Device->avs_bus_num].mdata;
        AVS_LOG_TRACE(" AVS3 Device mdata ds = %d \n", afm_mdata_addr3->ds);
        break;

    default:
        BUG_ASSERT(false);
        break;
    }

    avs_init((uint32_t)Device->avs_bus_num, &avs_cfg_default);

    AVS_LOG_INFO("AVS bus num =  %d, IRQ =  %d", Device->avs_bus_num, Device->config.avs_irq);

    // Configure the NVIC Handling
    nvic_status_t status = nvic_irq_set_isr_with_param(Device->config.avs_irq, scp_avs_isr, Device);
    BUG_ASSERT_PARAM(status == NVIC_STATUS_SUCCESS, status, 0);

    status = nvic_irq_clear_pending(Device->config.avs_irq);
    BUG_ASSERT_PARAM(status == NVIC_STATUS_SUCCESS, status, 0);

    status = nvic_irq_enable(Device->config.avs_irq);
    BUG_ASSERT_PARAM(status == NVIC_STATUS_SUCCESS, status, 0);

    // Configure the AVS HW to fire the ISR only on CMD DONE
    BUG_ASSERT_PARAM(avs_enable_interrupt(Device->avs_bus_num, AVS_IRQ_CMD_DONE) == SILIBS_SUCCESS, status, 0);
}

void scp_avs_interface_initialize(pscp_avs_device Device, pscp_avs_interface_t Interface)
{
    DfwkInterfaceInitialize(&Interface->Header, &Device->Header, &Device->avs_queue, scp_avs_dispatch_sync);
    Interface->Device = Device;
    Interface->Device->avs_response_errors = 0;
    DfwkAsyncRequestInitialize((PDFWK_ASYNC_REQUEST_HEADER)&Device->isr_request, sizeof(Device));
}

void scp_avs_init(pscp_avs_device avsDevice, DFWK_SCHEDULE* scheduler)
{
    DfwkDeviceInitialize(&avsDevice->Header, scheduler);
    scp_avs_driver_initialize(avsDevice);
}
