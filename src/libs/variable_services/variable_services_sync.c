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
static void variable_service_sync_common_handler(variable_service_operation_type_t type,
                                                 var_service_req_ctx_t* var_serv_ctx,
                                                 var_service_req_params_t* req_params);

/*------------------------------ Public Functions -------------------------*/
void variable_service_sync_set_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    DEBUG_PRINT("----Sync Set Variable----\n");
    variable_service_sync_common_handler(SYNC_SET_VARIABLE, var_serv_ctx, req_params);
    DEBUG_PRINT("----End of Sync Set Variable----\n");
}

void variable_service_sync_get_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    DEBUG_PRINT("----Sync Get Variable----\n");
    variable_service_sync_common_handler(SYNC_GET_VARIABLE, var_serv_ctx, req_params);
    DEBUG_PRINT("----End of Sync Get Variable----\n");
}

/*----------------------------Static Functions------------------------------*/
static void variable_service_sync_common_handler(variable_service_operation_type_t type,
                                                 var_service_req_ctx_t* var_serv_ctx,
                                                 var_service_req_params_t* req_params)
{
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

    //! Calculate the total size of the payload to be copied into shared memory & verify it is within the max payload size
    uint32_t total_size =
        sizeof(variable_service_shared_mem_format_t) + req_params->variable_name_size + req_params->data_size;
    uint32_t total_payload_size = total_size - sizeof(variable_service_mem_metadata_t);
    BUG_ASSERT(total_size <= var_serv_ctx->shared_mem.max_payload_size);

    //! Verify if the shared memory is free to use, must be free!
    variable_service_shared_mem_format_t* shared_mem =
        (variable_service_shared_mem_format_t*)var_serv_ctx->shared_mem.payload_base;

    //! populate the metadata including the hsp mbox cmd request struct & the mbox get/set variable struct
    //! to send the request to HSP
    if (type == SYNC_GET_VARIABLE)
    {
        shared_mem->metadata.msg.header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_REQ;
        shared_mem->attributes_size = req_params->attributes_size;
    }
    else if (type == SYNC_SET_VARIABLE)
    {
        shared_mem->metadata.msg.header.cmd = HSP_MAILBOX_CMD_SET_VARIABLE_REQ;
        BUG_ASSERT(variable_services_check_attribute(req_params->attributes.as_uint32) ==
                   KNG_SUCCESS); //! check for valid combination of attributes
        shared_mem->attributes = req_params->attributes.as_uint32;
    }
    shared_mem->metadata.msg.as_uint32[1] =
        var_serv_ctx->shared_mem.payload_base + sizeof(variable_service_mem_metadata_t); //! get/set_variable_address field
    shared_mem->metadata.actual_payload_size = total_payload_size;
    shared_mem->variable_name_size = req_params->variable_name_size / sizeof(uint16_t); //! No of 16-bit characters
    memcpy((void*)&shared_mem->vendor_namespace_guid, (void*)&req_params->vendor_namespace_guid, sizeof(guid_t));
    shared_mem->data_size = req_params->data_size;

    //! copy over the variable name into the shared memory
    memcpy((void*)var_serv_ctx->shared_mem.payload_base + sizeof(variable_service_shared_mem_format_t),
           (void*)req_params->variable_name_ptr,
           req_params->variable_name_size);
    if (type == SYNC_SET_VARIABLE)
    {
        //! copy over the data into the shared memory
        memcpy((void*)var_serv_ctx->shared_mem.payload_base + sizeof(variable_service_shared_mem_format_t) +
                   req_params->variable_name_size,
               (void*)req_params->data,
               req_params->data_size);
    }

    //! Debug prints before mailbox communication testing
    debug_print_pre_mbox_send(var_serv_ctx);

    size_t recv_msg_size_bytes = 0;
    //! Send the get variable sync request to the HSP & get response, blocking call
    fpfw_status_t icc_status =
        fpfw_icc_base_send_recv_sync(hsp_icc_ctx, &shared_mem->metadata.msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);

    //! Debug prints post mailbox communication testing
    debug_print_post_mbox_recv(var_serv_ctx, recv_msg_size_bytes, icc_status);

    //! Verify sync return status & response message
    BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
    BUG_ASSERT(recv_msg_size_bytes > 0);
    BUG_ASSERT(shared_mem->metadata.msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);
    if (type == SYNC_SET_VARIABLE)
    {
        BUG_ASSERT(shared_mem->metadata.msg.rsp.header.cmd == HSP_MAILBOX_CMD_SET_VARIABLE_RSP);
        //! HSP has consumed the data, free up the ctx & shared memory for SET
        variable_service_unlock_get_var_ctx(var_serv_ctx);
    }
    else if (type == SYNC_GET_VARIABLE)
    {
        BUG_ASSERT(shared_mem->metadata.msg.rsp.header.cmd == HSP_MAILBOX_CMD_GET_VARIABLE_RSP);
        //! Reset data buffer & copy data over from shared memory into the data buffer supplied in request params
        memset(req_params->data, 0, req_params->data_size);
        memcpy((void*)req_params->data,
               (void*)var_serv_ctx->shared_mem.payload_base + sizeof(variable_service_shared_mem_format_t) +
                   req_params->variable_name_size,
               shared_mem->data_size);
    }
}
