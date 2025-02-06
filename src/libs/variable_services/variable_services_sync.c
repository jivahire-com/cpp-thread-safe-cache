//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_sync.h
 * Implementation for variable services sync APIs
 */

/*------------- Includes -----------------*/
#include "variable_services.h"
#include "variable_services_helper.h"
#include "variable_services_mem.h"

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <bug_check.h>
#include <fpfw_icc_base.h> // for FPFW_ICC_BASE
#include <hsp_firmware_headers.h>
#include <kng_error.h> // for KNG_E_INVALIDARG, KNG_E_NOTIMPL
#include <stddef.h>    // for NULL
#include <string.h>    // for memcpy

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/**
 * @brief  Common Handler for setting/getting variable synchronously
 *
 * @param type  type of operation to be performed
 * @param var_serv_ctx  common var serv ctx
 * @param req_params  request parameters
 */
static int32_t variable_service_sync_common_handler(variable_service_operation_type_t type,
                                                    var_service_req_ctx_t* var_serv_ctx,
                                                    var_service_req_params_t* req_params);

/*------------------------------ Public Functions -------------------------*/
void variable_service_sync_set_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    DEBUG_PRINT("----Sync Set Variable----\n");
    variable_service_sync_common_handler(SYNC_SET_VARIABLE, var_serv_ctx, req_params);
    DEBUG_PRINT("----End of Sync Set Variable----\n");
}

int32_t variable_service_sync_get_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    int32_t result = KNG_SUCCESS;
    DEBUG_PRINT("----Sync Get Variable----\n");
    result = variable_service_sync_common_handler(SYNC_GET_VARIABLE, var_serv_ctx, req_params);
    DEBUG_PRINT("----End of Sync Get Variable----\n");

    return result;
}

/*----------------------------Static Functions------------------------------*/
static int32_t variable_service_sync_common_handler(variable_service_operation_type_t type,
                                                    var_service_req_ctx_t* var_serv_ctx,
                                                    var_service_req_params_t* req_params)
{
    int32_t result = KNG_SUCCESS;
    //! Null checks for input parameters
    BUG_ASSERT(var_serv_ctx != NULL);
    BUG_ASSERT(req_params != NULL);
    BUG_ASSERT(req_params->variable_name_ptr != NULL);
    BUG_ASSERT(req_params->variable_name_size != 0);
    BUG_ASSERT(req_params->data != NULL);
    BUG_ASSERT(req_params->data_size != 0);

    //! get the icc base context object
    fpfw_icc_base_ctx_t* hsp_icc_ctx = get_icc_base_ctx();
    BUG_ASSERT(hsp_icc_ctx != NULL);

    BUG_ASSERT(var_serv_ctx->is_initialized != false); //! Verify the ctx is initialized
    BUG_ASSERT(variable_service_set_shared_mem_in_use(var_serv_ctx) != false) //! Verify if the shared memory is free to use, must be free!
    memcpy((void*)&var_serv_ctx->req_params, (void*)req_params, sizeof(var_service_req_params_t));

    //! Calculate the total size of the payload to be copied into shared memory & verify it is within the max payload size
    uint32_t total_size =
        sizeof(variable_service_shared_mem_format_t) + req_params->variable_name_size + req_params->data_size;
    uint32_t total_payload_size = total_size - sizeof(variable_service_mem_metadata_t);
    BUG_ASSERT(total_size <= var_serv_ctx->shared_mem.max_payload_size);

    //! Assumption, this shared memory address is accessible by MSCP
    volatile variable_service_shared_mem_format_t* shared_mem =
        (volatile variable_service_shared_mem_format_t*)var_serv_ctx->shared_mem.payload_base;

    //! Fetch the address where the variable serv payload (starting with `variable_name_size` field) is stored in shared memory.
    uint64_t variable_address = get_variable_serv_payload_address(var_serv_ctx->shared_mem.payload_base);

    //! populate the metadata including the hsp mbox cmd request struct & the mbox get/set variable struct
    //! to send the request to HSP
    if (type == SYNC_GET_VARIABLE)
    {
        shared_mem->metadata.msg.header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_REQ;
        //! MSCP doesn't require attributes size to be populated, we will explicitly set this to 0 to avoid any confusion
        shared_mem->attributes_size = 0UL;
    }
    else if (type == SYNC_SET_VARIABLE)
    {
        shared_mem->metadata.msg.header.cmd = HSP_MAILBOX_CMD_SET_VARIABLE_REQ;
        BUG_ASSERT(variable_services_check_attribute(req_params->attributes.as_uint32) ==
                   KNG_SUCCESS); //! check for valid combination of attributes
        shared_mem->attributes = req_params->attributes.as_uint32;
    }
    //! populate lower 32 bits of the variable payload address in the 1st word of the hsp mbox message struct, get/set_variable_address field
    shared_mem->metadata.msg.as_uint32[1] = (uint32_t)(variable_address & 0xFFFFFFFFUL);
    //! populate upper 32 bits of the variable payload address in the 2nd word of the hsp mbox message struct, get/set_variable_address_high field
    shared_mem->metadata.msg.as_uint32[2] = (uint32_t)((variable_address >> 32U) & 0xFFFFFFFFUL);
    //! populate the rest of the metadata
    shared_mem->metadata.actual_payload_size = total_payload_size; //! this is the size of the payload excluding the metadata
    shared_mem->variable_name_size = req_params->variable_name_size / sizeof(uint16_t); //! No of 16-bit characters
    //! copy over the vendor namespace guid into the shared memory
    for (uint32_t i = 0; i < sizeof(guid_t) / sizeof(uint32_t); i++)
    {
        ((volatile uint32_t*)&shared_mem->vendor_namespace_guid)[i] =
            ((volatile uint32_t*)&req_params->vendor_namespace_guid)[i];
    }
    shared_mem->data_size = req_params->data_size;

    //! copy over the variable name into the shared memory byte by byte
    for (uint32_t i = 0; i < req_params->variable_name_size; i++)
    {
        shared_mem->variable_name_and_data[i] = ((volatile uint8_t*)req_params->variable_name_ptr)[i];
    }

    //! copy over the data into the shared memory for set variable byte by byte
    if (type == SYNC_SET_VARIABLE)
    {
        volatile uint8_t data_byte = 0;
        for (uint32_t i = 0; i < req_params->data_size; i++)
        {
            data_byte = ((volatile uint8_t*)req_params->data)[i];
            do
            {
                //! Copy & Verify data has been copied correctly
                shared_mem->variable_name_and_data[req_params->variable_name_size + i] = data_byte;
            } while (shared_mem->variable_name_and_data[req_params->variable_name_size + i] != data_byte);
        }
    }

    //! Debug prints before mailbox communication testing
    debug_print_pre_mbox_send(var_serv_ctx);

    size_t recv_msg_size_bytes = 0;
    //! Send the get variable sync request to the HSP & get response, blocking call
    fpfw_status_t icc_status =
        fpfw_icc_base_send_recv_sync(hsp_icc_ctx, (void*)&shared_mem->metadata.msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);

    //! Debug prints post mailbox communication testing
    debug_print_post_mbox_recv(var_serv_ctx, recv_msg_size_bytes, icc_status);

    //! Verify sync return status & response message
    BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
    BUG_ASSERT(recv_msg_size_bytes > 0);

    if (type == SYNC_SET_VARIABLE)
    {
        BUG_ASSERT(shared_mem->metadata.msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);
        BUG_ASSERT(shared_mem->metadata.msg.rsp.header.cmd == HSP_MAILBOX_CMD_SET_VARIABLE_RSP);
        //! HSP has consumed the data, free up the ctx & shared memory for SET
        variable_service_unlock_get_var_ctx(var_serv_ctx);
    }
    else if (type == SYNC_GET_VARIABLE)
    {
        BUG_ASSERT(shared_mem->metadata.msg.rsp.header.cmd == HSP_MAILBOX_CMD_GET_VARIABLE_RSP);

        // ! bugcheck all failures  except record not found
        if (shared_mem->metadata.msg.rsp.status == HSP_MAILBOX_RSP_STATUS_NOT_FOUND)
        {
            result = KNG_E_NOT_FOUND;
        }
        else
        {
            BUG_ASSERT(shared_mem->metadata.msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);
            //! Reset data buffer & copy data over from shared memory into the data buffer supplied in request params
            memset(req_params->data, 0, req_params->data_size);
            memcpy((void*)req_params->data,
                   (void*)var_serv_ctx->shared_mem.payload_base +
                       sizeof(variable_service_shared_mem_format_t) + req_params->variable_name_size,
                   shared_mem->data_size);
        }
    }

    return result;
}
