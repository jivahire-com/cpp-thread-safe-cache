//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_common.c
 * Contains functionality that is common to all fifo id's.
 */

/*------------- Includes -----------------*/
#include "sensor_fifo_driver_interface.h" // for psensor_fifo_device_proper...
#include "sensor_fifo_events_i.h"
#include "sensor_fifo_service.h" // for SENSOR_FIFO_ID, SENSOR_FIF...
#include "sensor_fifo_service_i.h"

#include <DfwkClient.h>                  // for DfwkClientInterfaceOpen
#include <FpFwAssert.h>                  // for FPFW_RUNTIME_ASSERT
#include <FpFwLock.h>                    // for FPFW_LOCK, FpFwLockInitialize
#include <IFpFwEventTracingGeneration.h> // for FPFW_ET_LOG
#include <fpfw_icc_base.h>
#include <fpfw_status.h> // for FPFW_STATUS_FAILED, fpfw_s...
#include <icc_mhu.h>
#include <kng_icc_shared.h>
#include <mscp_uefi_shared_ddrss.h>
#include <stdbool.h> // for bool
#include <stddef.h>  // for NULL
#include <stdint.h>  // for int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

psensor_fifo_driver_interface_t pSensor_fifo_driver_inf;
psensor_fifo_svc_config_t snsr_fifo_svc_config;
fpfw_icc_base_recv_req_t mcp_icc_async_recv_req;
uint8_t mcp_icc_rx_buffer[ICC_MHU_DDR_PAYLOAD_SIZE];

FPFW_LOCK snsr_fifo_seq_lock;
uint16_t snsr_fifo_seq_num = 0x9000;

static_assert(sizeof(icc_mhu_snsr_fifo_msg_t) <= ICC_MHU_DDR_PAYLOAD_SIZE, "MHU payload size too small");

/*------------- Functions ----------------*/
void sensor_fifo_svc_initialize(psensor_fifo_driver_interface_t driver_interface, psensor_fifo_svc_config_t svc_config)
{
    pSensor_fifo_driver_inf = driver_interface;
    snsr_fifo_svc_config = svc_config;

    int32_t status = DfwkClientInterfaceOpen(&driver_interface->base_interface);

    FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);

    if (!snsr_fifo_svc_config->is_scp)
    {
        memset(&mcp_icc_async_recv_req, 0, sizeof(fpfw_icc_base_recv_req_t));
        mcp_icc_async_recv_req.payload_buffer = mcp_icc_rx_buffer;
        mcp_icc_async_recv_req.buffer_size = sizeof(mcp_icc_rx_buffer);
        mcp_icc_async_recv_req.recv_cmd_code = ICC_COMMAND_SNSR_FIFO_MSG;
        mcp_icc_async_recv_req.cb = sensor_fifo_svc_recv_complete_notify_from_drv_frmwk;
        mcp_icc_async_recv_req.cb_ctx = &mcp_icc_async_recv_req;

        fpfw_status_t status = fpfw_icc_base_recv(snsr_fifo_svc_config->mscp_icc_base_ctx, &mcp_icc_async_recv_req);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
    }

    FpFwLockInitialize(&snsr_fifo_seq_lock);
}

void sensor_fifo_svc_get_properties(SENSOR_FIFO_ID fifo, psensor_fifo_properties_t properties)
{
    FPFW_RUNTIME_ASSERT(fifo < SENSOR_FIFO_MAX_ID);
    FPFW_RUNTIME_ASSERT(properties != NULL);

    psensor_fifo_device_properties_t device_properties = &pSensor_fifo_driver_inf->device->fifo_property_table[fifo];

    properties->entry_size_bytes = device_properties->entry_size_bytes;
    properties->stride_size_bytes = device_properties->stride_size_bytes;
    properties->start_address_incl = device_properties->start_address_incl;
    properties->end_address_excl = device_properties->end_address_excl;
    properties->num_entries_or_strides = device_properties->entry_count;
    properties->name = device_properties->name;
}

void sensor_fifo_svc_set_global_hw_enable(bool enable)
{
    fpfw_status_t status = sensor_fifo_driver_inf_set_global_hw_enable(pSensor_fifo_driver_inf, enable);
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(GlobalEnableFail, status);
    }
}

void sensor_fifo_svc_enable_fifo(SENSOR_FIFO_ID fifo)
{
    bool fifo_enabled_status[SENSOR_FIFO_MAX_ID] = {0};

    fpfw_status_t status =
        sensor_fifo_driver_inf_set_fifo_enable(pSensor_fifo_driver_inf,
                                               pSensor_fifo_driver_inf->device->fifo_property_table[fifo].device_fifo_id,
                                               true,
                                               &fifo_enabled_status);

    if (FPFW_STATUS_SUCCEEDED(status) && snsr_fifo_svc_config->is_scp)
    {
        sensor_fifo_svc_notify_mcp(&fifo_enabled_status);
    }
    else
    {
        FPFW_ET_LOG(FifoEnableFail, status);
    }
}

void sensor_fifo_svc_disable_fifo(SENSOR_FIFO_ID fifo)
{
    bool fifo_enabled_status[SENSOR_FIFO_MAX_ID] = {0};

    fpfw_status_t status =
        sensor_fifo_driver_inf_set_fifo_enable(pSensor_fifo_driver_inf,
                                               pSensor_fifo_driver_inf->device->fifo_property_table[fifo].device_fifo_id,
                                               false,
                                               &fifo_enabled_status);

    if (FPFW_STATUS_SUCCEEDED(status) && snsr_fifo_svc_config->is_scp)
    {
        sensor_fifo_svc_notify_mcp(&fifo_enabled_status);
    }
    else
    {
        FPFW_ET_LOG(FifoDisableFail, status);
    }
}

void sensor_fifo_svc_is_empty(bool (*is_empty)[SENSOR_FIFO_MAX_ID])
{
    fpfw_status_t status = sensor_fifo_driver_inf_is_empty(pSensor_fifo_driver_inf, is_empty);

    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(FifoEmptyFail, status);
        memset(*is_empty, 0, sizeof(bool) * SENSOR_FIFO_MAX_ID);
    }
}

fpfw_status_t sensor_fifo_svc_notify_mcp(bool (*is_enabled)[SENSOR_FIFO_MAX_ID])
{
    icc_mhu_snsr_fifo_msg_t icc_msg;

    icc_msg.sf_msg.hdr.msg_id = SNSR_FIFO_MSG_ID_SYNC_ENABLES;
    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&snsr_fifo_seq_lock);
    icc_msg.sf_msg.hdr.seq_num = snsr_fifo_seq_num++;
    FpFwLockRelease(&snsr_fifo_seq_lock, lock_state);
    icc_msg.sf_msg.hdr.payload_size = sizeof(snsr_fifo_msg_sync_enables_t);

    memcpy(icc_msg.sf_msg.payload.sync_enables.fifo_enabled,
           *is_enabled,
           sizeof(icc_msg.sf_msg.payload.sync_enables.fifo_enabled));

    icc_msg.header.msg_header.command = ICC_COMMAND_SNSR_FIFO_MSG;
    icc_msg.header.msg_header.payload_size = icc_msg.sf_msg.hdr.payload_size + sizeof(snsr_fifo_msg_hdr_t);

    fpfw_status_t status = fpfw_icc_base_send_sync(snsr_fifo_svc_config->mscp_icc_base_ctx,
                                                   &icc_msg,
                                                   icc_msg.header.msg_header.payload_size + sizeof(icc_mhu_header_t));
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(FifoIccSendFail, status, (uint32_t)snsr_fifo_svc_config->mscp_icc_base_ctx);
    }

    return status;
}

// called from the driver framework thread
void sensor_fifo_svc_recv_complete_notify_from_drv_frmwk(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    fpfw_status_t api_status;

    if (FPFW_STATUS_SUCCEEDED(status))
    {
        fpfw_icc_base_recv_req_t* recv_context = (fpfw_icc_base_recv_req_t*)context;
        icc_mhu_snsr_fifo_msg_t* icc_mhu_snsr_fifo_msg = (icc_mhu_snsr_fifo_msg_t*)(recv_context->payload_buffer);

        // validate
        if ((icc_mhu_snsr_fifo_msg->header.msg_header.command == ICC_COMMAND_SNSR_FIFO_MSG) &&
            (icc_mhu_snsr_fifo_msg->sf_msg.hdr.msg_id == SNSR_FIFO_MSG_ID_SYNC_ENABLES))
        {
            api_status =
                sensor_fifo_driver_inf_sync_fifo_enables(pSensor_fifo_driver_inf,
                                                         &icc_mhu_snsr_fifo_msg->sf_msg.payload.sync_enables.fifo_enabled);

            if (FPFW_STATUS_FAILED(api_status))
            {
                FPFW_ET_LOG(FifoIccSyncEnableFail, api_status, (uint32_t)snsr_fifo_svc_config->mscp_icc_base_ctx);
            }
        }
        else
        {
            FPFW_ET_LOG(FifoIccRecvValidationFail,
                        icc_mhu_snsr_fifo_msg->header.msg_header.command,
                        icc_mhu_snsr_fifo_msg->sf_msg.hdr.msg_id);
        }
    }
    else
    {
        FPFW_ET_LOG(FifoIccRecvFail, status, (uint32_t)context);
    }

    api_status = fpfw_icc_base_recv(snsr_fifo_svc_config->mscp_icc_base_ctx, &mcp_icc_async_recv_req);
    if (FPFW_STATUS_FAILED(api_status))
    {
        FPFW_ET_LOG(FifoIccReceiveRegisterFail, api_status, (uint32_t)snsr_fifo_svc_config->mscp_icc_base_ctx);
    }
}