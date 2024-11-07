//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_helper.c
 * Implementation for common helper functions for variable services sync & async APIs
 */

/*------------- Includes -----------------*/
#include "variable_services_helper.h"

#include "variable_services_mem.h"

#include <FpFwLock.h>
#include <bug_check.h>
#include <kng_error.h> // for KNG_E_INVALIDARG, KNG_E_NOTIMPL
#include <string.h>    // for memcpy

/*-- Symbolic Constant Macros (defines) --*/
#define EFI_VARIABLE_ATTRIBUTES_MASK                                                             \
    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | \
     EFI_VARIABLE_HARDWARE_ERROR_RECORD | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | EFI_VARIABLE_APPEND_WRITE)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
//! Since variable services is always over hsp mbox, we can use a static variable to store the ICC context
//! instead of taking in ICC base crx as a param from user
static fpfw_icc_base_ctx_t* hsp_icc_ctx = NULL;

/*------------- Internal Helper Functions ----------------*/
int32_t variable_services_check_attribute(uint32_t attributes)
{
    //! Check for valid variable attribute.
    //! EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated and should not be used.
    if ((attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0)
    {
        return KNG_E_NOTIMPL;
    }

    //! Make sure the Attributes combination is supported by the platform.
    if ((attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == 0)
    {
        return KNG_E_INVALIDARG;
    }

    //! Only EFI_VARIABLE_NON_VOLATILE attribute is invalid
    if ((attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == EFI_VARIABLE_NON_VOLATILE)
    {
        if ((attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0)
        {
            return KNG_E_NOTIMPL;
        }
        return KNG_E_INVALIDARG;
    }

    //! Make sure if runtime bit is set, boot service bit is set also.
    if ((attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS)
    {

        if ((attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0)
        {
            return KNG_E_NOTIMPL;
        }
        return KNG_E_INVALIDARG;
    }

    //! Hardware error record is not supported
    if ((attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0)
    {
        return KNG_E_NOTIMPL;
    }

    //! EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS and EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
    //! attribute cannot be set both.
    if (((attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) &&
        ((attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS))
    {
        return KNG_E_NOTIMPL;
    }

    return KNG_SUCCESS;
}

fpfw_icc_base_ctx_t* get_icc_base_ctx(void)
{
    return hsp_icc_ctx;
}

bool variable_service_set_shared_mem_in_use(var_service_req_ctx_t* var_serv_ctx)
{
    bool status = false;
    //! Null checks for input parameters
    BUG_ASSERT(var_serv_ctx != NULL);
    //! get the shared memory object & check if it's free & set the in_use flag
    variable_service_shared_mem_format_t* shared_mem =
        (variable_service_shared_mem_format_t*)var_serv_ctx->shared_mem.payload_base;

    //! critical section start, set flag in use
    FPFW_LOCK_STATE oldState = FpFwLockAcquire(&var_serv_ctx->var_serv_lock);
    if (shared_mem->metadata.sync.bitfield.is_in_use == 0)
    {
        shared_mem->metadata.sync.bitfield.is_in_use = 1;
        status = true;
    }
    FpFwLockRelease(&var_serv_ctx->var_serv_lock, oldState);
    return status;
}

void debug_print_pre_mbox_send(var_service_req_ctx_t* var_serv_ctx)
{
#ifndef DEBUG_VAR_SERV
    FPFW_UNUSED(var_serv_ctx);
#else
    variable_service_shared_mem_format_t* shared_mem =
        (variable_service_shared_mem_format_t*)var_serv_ctx->shared_mem.payload_base;
    uint32_t total_size = sizeof(variable_service_shared_mem_format_t) +
                          var_serv_ctx->req_params.variable_name_size + var_serv_ctx->req_params.data_size;

    printf("Metadata:          InUse[0x%" PRIx32 "]\n", shared_mem->metadata.sync.as_uint32);
    printf("Variable:          Name[%s] Size[%d]\n",
           (char*)var_serv_ctx->req_params.variable_name_ptr,
           var_serv_ctx->req_params.variable_name_size);
    printf("Shared Mem:        Addr[0x%x] Size[%d] bytes\n",
           var_serv_ctx->shared_mem.payload_base,
           var_serv_ctx->shared_mem.max_payload_size);
    printf("Vendor GUID:       GUID1[0x%" PRIx32
           "] GUID2[0x%x] GUID3[0x%x] GUID4[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]\n",
           var_serv_ctx->req_params.vendor_namespace_guid.guid1,
           var_serv_ctx->req_params.vendor_namespace_guid.guid2,
           var_serv_ctx->req_params.vendor_namespace_guid.guid3,
           var_serv_ctx->req_params.vendor_namespace_guid.guid4[0],
           var_serv_ctx->req_params.vendor_namespace_guid.guid4[1],
           var_serv_ctx->req_params.vendor_namespace_guid.guid4[2],
           var_serv_ctx->req_params.vendor_namespace_guid.guid4[3],
           var_serv_ctx->req_params.vendor_namespace_guid.guid4[4],
           var_serv_ctx->req_params.vendor_namespace_guid.guid4[5],
           var_serv_ctx->req_params.vendor_namespace_guid.guid4[6],
           var_serv_ctx->req_params.vendor_namespace_guid.guid4[7]);
    if ((var_serv_ctx->operation_type == ASYNC_SET_VARIABLE) || (var_serv_ctx->operation_type == SYNC_SET_VARIABLE))
    {
        printf("Attributes:        [0x%" PRIx32 "]\n", var_serv_ctx->req_params.attributes.as_uint32);
    }
    else if ((var_serv_ctx->operation_type == ASYNC_GET_VARIABLE) || (var_serv_ctx->operation_type == SYNC_GET_VARIABLE))
    {
        printf("Attributes Size:   [%" PRId32 "] bytes\n", var_serv_ctx->req_params.attributes_size);
    }
    printf("Data Size:         [%d] bytes\n", var_serv_ctx->req_params.data_size);
    printf("Projected Total Size:        [%" PRId32 "] bytes\n", total_size);
    printf("Shared Memory Offsets: get/set var[0x%" PRIx32 "]   variable_name_ptr[0x%" PRIx32
           "]    data[0x%" PRIx32 "]\n",
           (uint32_t)sizeof(variable_service_mem_metadata_t),
           (uint32_t)sizeof(variable_service_shared_mem_format_t),
           (uint32_t)(sizeof(variable_service_shared_mem_format_t) + var_serv_ctx->req_params.variable_name_size));
    printf("Shared Memory Req Dump\n");
    for (size_t i = 0; i < total_size; i++)
    {
        printf("Byte Index [%" PRId32 "]: [0x%02x]\n",
               (uint32_t)i,
               ((unsigned char*)var_serv_ctx->shared_mem.payload_base)[i]);
    }
    printf("Preparing to send get/set variable request to HSP\n");
    printf("HSP Mbox Req Mesg: Cmd[0x%x] Addr[0x%" PRIx32 "]\n",
           shared_mem->metadata.msg.header.cmd,
           shared_mem->metadata.msg.as_uint32[1]);
    for (size_t i = 0; i < HSP_MBOX_FIFO_DEPTH; i++)
    {
        printf("as_uint32[%" PRId32 "]: [0x%" PRIx32 "]\n", (uint32_t)i, shared_mem->metadata.msg.as_uint32[i]);
    }
#endif
}

void debug_print_post_mbox_recv(var_service_req_ctx_t* var_serv_ctx, size_t output_size_bytes, fpfw_status_t status)
{
#ifndef DEBUG_VAR_SERV
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);
#else
    variable_service_shared_mem_format_t* shared_mem =
        (variable_service_shared_mem_format_t*)var_serv_ctx->shared_mem.payload_base;
    uint32_t total_size = sizeof(variable_service_shared_mem_format_t) +
                          var_serv_ctx->req_params.variable_name_size + var_serv_ctx->req_params.data_size;

    printf("Metadata:          InUse[0x%" PRIx32 "]\n", shared_mem->metadata.sync.as_uint32);
    printf("Response for variable get/set received, status [%" PRId32 "]\n", status);
    printf("Response message size [%" PRId32 "] bytes\n", (uint32_t)output_size_bytes);

    printf("HSP Mbox Rsp Mesg: Cmd[0x%x] Status[0x%" PRIx32 "] Status_ex[0x%" PRIx32 "]\n",
           shared_mem->metadata.msg.rsp.header.cmd,
           shared_mem->metadata.msg.rsp.status,
           shared_mem->metadata.msg.rsp.status_ex);

    if (shared_mem->metadata.msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS)
    {
        for (size_t i = 0; i < HSP_MBOX_FIFO_DEPTH; i++)
        {
            printf("as_uint32[%" PRId32 "]: [0x%" PRIx32 "]\n", (uint32_t)i, shared_mem->metadata.msg.as_uint32[i]);
        }

        //! The response data at this point is written over to the shared memory region supplied by the caller
        printf("Shared Memory Response Dump\n");
        for (size_t i = 0; i < total_size; i++)
        {
            printf("Byte Index [%" PRId32 "]: [0x%02x]\n",
                   (uint32_t)i,
                   ((unsigned char*)var_serv_ctx->shared_mem.payload_base)[i]);
        }
    }
#endif
}

/*------------- Common Public Functions ----------------*/
void variable_service_init(fpfw_icc_base_ctx_t* icc_ctx)
{
    //! Initialize the ICC context
    BUG_ASSERT(icc_ctx != NULL);
    hsp_icc_ctx = icc_ctx;
    //! What else needs to be done here?
}

void variable_service_unlock_get_var_ctx(var_service_req_ctx_t* var_serv_ctx)
{
    //! Null checks for input parameters
    BUG_ASSERT(var_serv_ctx != NULL);
    BUG_ASSERT(var_serv_ctx->is_initialized != false);

    //! get the shared memory object & reset the in_use flag
    variable_service_shared_mem_format_t* shared_mem =
        (variable_service_shared_mem_format_t*)var_serv_ctx->shared_mem.payload_base;

    //! critical section start, reset the in_use flag
    FPFW_LOCK_STATE oldState = FpFwLockAcquire(&var_serv_ctx->var_serv_lock);
    if (shared_mem->metadata.sync.bitfield.is_in_use == true)
    {
        //! Reset shared memory region
        memset((void*)var_serv_ctx->shared_mem.payload_base, 0, sizeof(var_serv_ctx->shared_mem.max_payload_size));
        //! reset the variable ctx object preserving the original shared memory mapping & initialized flag
        memset(&var_serv_ctx->icc_req, 0, sizeof(fpfw_icc_base_send_recv_req_t));
        memset(&var_serv_ctx->req_params, 0, sizeof(var_service_req_params_t));
        var_serv_ctx->callback = NULL;
        var_serv_ctx->context = NULL;
        var_serv_ctx->operation_type = OPERATION_TYPE_MAX;
    }
    FpFwLockRelease(&var_serv_ctx->var_serv_lock, oldState);
}

int32_t variable_service_initialize_ctx(var_service_req_ctx_t* var_serv_ctx, var_service_shared_mem_t* mem_ctx)
{
    BUG_ASSERT(var_serv_ctx != NULL);
    if (var_serv_ctx->is_initialized)
    {
        DEBUG_PRINT("Shared memory is already Initialized\n");
        return KNG_SUCCESS;
    }

    //! Null checks for input parameters
    BUG_ASSERT(mem_ctx != NULL);
    BUG_ASSERT(mem_ctx->payload_base != 0);
    BUG_ASSERT(mem_ctx->max_payload_size != 0);
    //! Verify the shared memory is atleast large enough to hold metadata & actual payload data
    BUG_ASSERT(mem_ctx->max_payload_size > sizeof(variable_service_shared_mem_format_t));

    //! Initialize the var service ctx
    var_serv_ctx->shared_mem.payload_base = mem_ctx->payload_base;
    var_serv_ctx->shared_mem.max_payload_size = mem_ctx->max_payload_size;
    memset(&var_serv_ctx->icc_req, 0, sizeof(fpfw_icc_base_send_recv_req_t));
    memset(&var_serv_ctx->req_params, 0, sizeof(var_service_req_params_t));
    var_serv_ctx->callback = NULL;                     //! to be filled in later
    var_serv_ctx->context = NULL;                      //! to be filled in later
    var_serv_ctx->operation_type = OPERATION_TYPE_MAX; //! to be filled in later

    //! Reset shared memory region
    memset((void*)var_serv_ctx->shared_mem.payload_base, 0, var_serv_ctx->shared_mem.max_payload_size);

    //! Initialize the lock to protect the share memory ctx
    FpFwLockInitialize(&var_serv_ctx->var_serv_lock);

    var_serv_ctx->is_initialized = true;
    return KNG_SUCCESS;
}
