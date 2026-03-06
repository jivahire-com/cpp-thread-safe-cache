//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_async.c
 * Implementation for variable services async APIs
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
#include <mpu.h>
#include <silibs_common.h>
#include <stddef.h> // for NULL
#include <string.h> // for memcpy
#include <variable_services_events.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/**
 * @brief Common API to for get/set async variable service
 *
 * @param type operation type
 * @param var_serv_ctx variable service context
 * @param req_params request parameters
 * @param callback caller provided callback function
 * @param context caller provided context
 * @return int32_t KNG_SUCCESS on success, else KNG_E_FAIL
 */
static int32_t variable_service_async_common_handler(variable_service_operation_type_t type,
                                                     var_service_req_ctx_t* var_serv_ctx,
                                                     var_service_req_params_t* req_params,
                                                     variable_service_req_complete_notify callback,
                                                     void* context);

/**
 * @brief Common callback function for async get/set variable service
 *
 * @param context caller context
 * @param output_size_bytes size of the output received
 * @param status status of the ICC operation
 */
static void variable_service_common_async_cb(void* context, size_t output_size_bytes, fpfw_status_t status);

/**
 * @brief  Helper function to check if the variable serv ctx is in use
 *
 * @param var_serv_ctx  Variable service context
 * @return true
 * @return false
 */
static bool is_var_serv_ctx_in_use(var_service_req_ctx_t* var_serv_ctx);

/*------------------------------ Public Functions -------------------------*/
int32_t variable_service_async_get_variable(var_service_req_ctx_t* var_serv_ctx,
                                            var_service_req_params_t* req_params,
                                            variable_service_req_complete_notify callback,
                                            void* context)
{
    DEBUG_PRINT("----Async Get Variable----\n");
    return variable_service_async_common_handler(ASYNC_GET_VARIABLE, var_serv_ctx, req_params, callback, context);
}

int32_t variable_service_async_set_variable(var_service_req_ctx_t* var_serv_ctx,
                                            var_service_req_params_t* req_params,
                                            variable_service_req_complete_notify callback,
                                            void* context)
{
    DEBUG_PRINT("----Async Set Variable----\n");
    BUG_ASSERT(req_params != NULL);
    BUG_ASSERT(req_params->data != NULL);
    return variable_service_async_common_handler(ASYNC_SET_VARIABLE, var_serv_ctx, req_params, callback, context);
}

/*----------------------------Static Functions------------------------------*/
static int32_t variable_service_async_common_handler(variable_service_operation_type_t type,
                                                     var_service_req_ctx_t* var_serv_ctx,
                                                     var_service_req_params_t* req_params,
                                                     variable_service_req_complete_notify callback,
                                                     void* context)
{
    BUG_ASSERT(var_serv_ctx != NULL);
    BUG_ASSERT(req_params != NULL);
    BUG_ASSERT(req_params->variable_name_ptr != NULL);
    BUG_ASSERT(req_params->variable_name_size != 0);
    BUG_ASSERT(req_params->data_size != 0);
    //! Verify cb function is provided
    BUG_ASSERT(callback != NULL);

    //! get the icc base context object
    fpfw_icc_base_ctx_t* hsp_icc_ctx = get_icc_base_ctx();
    BUG_ASSERT(hsp_icc_ctx != NULL);

    //! Verify the ctx is initialized
    BUG_ASSERT(var_serv_ctx->is_initialized != false);
    BUG_ASSERT(variable_service_set_shared_mem_in_use(var_serv_ctx) != false) //! Verify if the shared memory is free to use, must be free!

    //! Calculate the total size of the payload to be copied into shared memory & verify it is within the max payload size
    uint32_t total_size =
        sizeof(variable_service_shared_mem_format_t) + req_params->variable_name_size + req_params->data_size;
    uint32_t total_payload_size = total_size - sizeof(variable_service_mem_metadata_t);
    BUG_ASSERT_PARAM(total_size <= var_serv_ctx->shared_mem.max_payload_size,
                     total_size,
                     var_serv_ctx->shared_mem.max_payload_size);

    //! Assumption, this shared memory address is accessible by MSCP
    volatile variable_service_shared_mem_format_t* shared_mem =
        (volatile variable_service_shared_mem_format_t*)var_serv_ctx->shared_mem.payload_base;

    //! Fetch the address where the variable serv payload (starting with `variable_name_size`field) is stored in shared memory.
    uint64_t variable_address = get_variable_serv_payload_address(var_serv_ctx->shared_mem.payload_base);

    //! populate the var service ctx
    memcpy((void*)&var_serv_ctx->req_params, (void*)req_params, sizeof(var_service_req_params_t));
    var_serv_ctx->callback = callback;   //! caller provided cb function
    var_serv_ctx->context = context;     //! caller provided ctx
    var_serv_ctx->operation_type = type; //! get/set variable type
    var_serv_ctx->icc_req.payload_buffer =
        (void*)&shared_mem->metadata.msg; //! only holds the inband payload from the mailbox fifo
    var_serv_ctx->icc_req.buffer_size = sizeof(kng_hsp_mailbox_msg);
    var_serv_ctx->icc_req.cb = variable_service_common_async_cb;
    var_serv_ctx->icc_req.cb_ctx = var_serv_ctx;
    var_serv_ctx->async_req_result = KNG_SUCCESS;

    //! populate the metadata including the hsp mbox cmd request struct & the mbox get/set variable struct
    //! to send the request to HSP
    if (type == ASYNC_GET_VARIABLE)
    {
        shared_mem->metadata.msg.header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_REQ;
        //! MSCP doesn't require attributes size to be populated, we will explicitly set this to 0 to avoid any confusion
        shared_mem->attributes_size = 0UL;
    }
    else if (type == ASYNC_SET_VARIABLE)
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
    shared_mem->metadata.actual_payload_size = total_payload_size;
    shared_mem->variable_name_size = req_params->variable_name_size / sizeof(uint16_t); //! No of 16-bit characters
    //! copy over the vendor namespace guid into the shared memory
    for (uint32_t i = 0; i < sizeof(guid_t) / sizeof(uint32_t); i++)
    {
        ((volatile uint32_t*)&shared_mem->vendor_namespace_guid)[i] =
            ((uint32_t*)&req_params->vendor_namespace_guid)[i];
    }

    shared_mem->data_size = req_params->data_size;

    //! copy over the variable name into the shared memory
    for (uint32_t i = 0; i < req_params->variable_name_size; i++)
    {
        ((volatile uint8_t*)shared_mem->variable_name_and_data)[i] = ((uint8_t*)req_params->variable_name_ptr)[i];
    }

    if (type == ASYNC_SET_VARIABLE)
    {
        //! copy over the data into the shared memory for set variable
        for (uint32_t i = 0; i < req_params->data_size; i++)
        {
            ((volatile uint8_t*)shared_mem->variable_name_and_data)[req_params->variable_name_size + i] =
                req_params->data[i];
        }
    }

    //! Debug prints pre mailbox communication testing
    debug_print_pre_mbox_send(var_serv_ctx);

    //! Flush the cache for the shared memory region to ensure the data is visible to HSP
    SCB_CleanInvalidateDCache_by_Addr((void*)ALIGN_DOWN(var_serv_ctx->shared_mem.payload_base, 32),
                                      var_serv_ctx->shared_mem.max_payload_size);

    //! Send the payload & wait for response
    fpfw_status_t icc_status = fpfw_icc_base_send_recv(hsp_icc_ctx, &var_serv_ctx->icc_req);
    if (icc_status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        DEBUG_PRINT("Error!! ICC send recv failed\n");
        FPFW_ET_LOG(VariableServicesAsyncIccSendError, icc_status, __LINE__);
        return KNG_E_FAIL;
    }
    DEBUG_PRINT("----End of Async %s Variable----\n", var_serv_ctx->operation_type == ASYNC_SET_VARIABLE ? "Set" : "Get");
    return KNG_SUCCESS;
}

static void variable_service_common_async_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    var_service_req_ctx_t* var_serv_ctx = (var_service_req_ctx_t*)context; // NOLINT
    volatile variable_service_shared_mem_format_t* shared_mem =
        (volatile variable_service_shared_mem_format_t*)var_serv_ctx->shared_mem.payload_base;

    DEBUG_PRINT("----Async %s Variable Callback Raised----\n",
                var_serv_ctx->operation_type == ASYNC_SET_VARIABLE ? "Set" : "Get");
    debug_print_post_mbox_recv(var_serv_ctx, output_size_bytes, status);

    //! these are non recoverable errors, indicating memory corruption or protocol mismatch, must assert
    //! perform sanity check on expected icc status
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, FPFW_ICC_BASE_STATUS_SUCCESS); //! always success
    //! output size must be atleast the size hsp mailbox header i.e 4 bytes
    BUG_ASSERT_PARAM(output_size_bytes > sizeof(uint32_t), output_size_bytes, 0);
    //! perform sanity check on shared memory & var service ctx, assert indicates memory corruption
    BUG_ASSERT(is_var_serv_ctx_in_use(var_serv_ctx) == true);

    //! Verify expected response cmd
    if (var_serv_ctx->operation_type == ASYNC_SET_VARIABLE)
    {
        BUG_ASSERT_PARAM(shared_mem->metadata.msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS,
                         shared_mem->metadata.msg.rsp.status,
                         HSP_MAILBOX_RSP_STATUS_SUCCESS);
        BUG_ASSERT_PARAM(shared_mem->metadata.msg.rsp.header.cmd == HSP_MAILBOX_CMD_SET_VARIABLE_RSP,
                         shared_mem->metadata.msg.rsp.header.cmd,
                         HSP_MAILBOX_CMD_SET_VARIABLE_RSP);
    }
    else if (var_serv_ctx->operation_type == ASYNC_GET_VARIABLE)
    {
        BUG_ASSERT_PARAM(shared_mem->metadata.msg.rsp.header.cmd == HSP_MAILBOX_CMD_GET_VARIABLE_RSP,
                         shared_mem->metadata.msg.rsp.header.cmd,
                         HSP_MAILBOX_CMD_GET_VARIABLE_RSP);

        if (shared_mem->metadata.msg.rsp.status != HSP_MAILBOX_RSP_STATUS_NOT_FOUND)
        {
            BUG_ASSERT_PARAM(shared_mem->metadata.msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS,
                             shared_mem->metadata.msg.rsp.status,
                             HSP_MAILBOX_RSP_STATUS_SUCCESS);
        }
        else
        {
            var_serv_ctx->async_req_result = KNG_E_NOT_FOUND;
        }
    }

    //! Call the caller provided callback function
    var_serv_ctx->callback(var_serv_ctx->context,
                           var_serv_ctx,
                           (uint8_t*)(shared_mem->variable_name_and_data + var_serv_ctx->req_params.variable_name_size),
                           shared_mem->data_size);
    if (var_serv_ctx->operation_type == ASYNC_SET_VARIABLE)
    {
        DEBUG_PRINT("----Async Set Variable Request Complete----\n");
        //! HSP has consumed the data, free up the ctx & shared memory for SET
        variable_service_unlock_get_var_ctx(var_serv_ctx);
    }
    else
    {
        DEBUG_PRINT("----Async Get Variable Request Complete----\n");
    }
}

static bool is_var_serv_ctx_in_use(var_service_req_ctx_t* var_serv_ctx)
{
    bool flag = false;
    //! Null checks for input parameters
    BUG_ASSERT(var_serv_ctx != NULL);
    volatile variable_service_shared_mem_format_t* shared_mem =
        (volatile variable_service_shared_mem_format_t*)var_serv_ctx->shared_mem.payload_base;
    //! critical section start, get the shared memory object & check if it's free
    FPFW_LOCK_STATE oldState = FpFwLockAcquire(&var_serv_ctx->var_serv_lock);
    flag = shared_mem->metadata.sync.bitfield.is_in_use;
    FpFwLockRelease(&var_serv_ctx->var_serv_lock, oldState);
    //! critical section end
    return flag;
}
