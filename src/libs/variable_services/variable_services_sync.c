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

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <bug_check.h>
#include <fpfw_icc_base.h> // for FPFW_ICC_BASE
#include <hsp_firmware_headers.h>
#include <kng_error.h>   // for KNG_E_INVALIDARG, KNG_E_NOTIMPL
#include <stddef.h>      // for NULL
#include <string.h>      // for memcpy
#include <system_info.h> // for system_info_is_hsp_present()

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void variable_service_sync_set_variable(var_service_shared_mem_t* mem_ctx, var_service_req_params_t* req_params)
{
    if (!system_info_is_hsp_present())
    {
        DEBUG_PRINT("HSP not present, skipping set variable\n");
        return;
    }

    DEBUG_PRINT("----Sync Set Variable----\n");
    //! Null checks for input parameters
    BUG_ASSERT(mem_ctx != NULL);
    BUG_ASSERT(mem_ctx->payload_base != 0);
    BUG_ASSERT(mem_ctx->max_payload_size != 0);
    BUG_ASSERT(req_params != NULL);
    BUG_ASSERT(req_params->variable_name_ptr != NULL);
    BUG_ASSERT(req_params->variable_name_size != 0);
    BUG_ASSERT(req_params->data != NULL);
    BUG_ASSERT(req_params->data_size != 0);

    //! get the icc base context object
    fpfw_icc_base_ctx_t* hsp_icc_ctx = get_icc_base_ctx();
    BUG_ASSERT(hsp_icc_ctx != NULL);

    //! populate the mbox set variable structure
    struct hsp_mbox_set_variable set_var = {};
    set_var.variable_name_size = req_params->variable_name_size / sizeof(uint16_t); //! No of 16-bit characters
    set_var.vendor_guid.guid.data1 = req_params->vendor_namespace_guid.guid1;
    set_var.vendor_guid.guid.data2 = req_params->vendor_namespace_guid.guid2;
    set_var.vendor_guid.guid.data3 = req_params->vendor_namespace_guid.guid3;
    set_var.vendor_guid.guid.data4[0] = req_params->vendor_namespace_guid.guid4[0];
    set_var.vendor_guid.guid.data4[1] = req_params->vendor_namespace_guid.guid4[1];
    set_var.vendor_guid.guid.data4[2] = req_params->vendor_namespace_guid.guid4[2];
    set_var.vendor_guid.guid.data4[3] = req_params->vendor_namespace_guid.guid4[3];
    set_var.vendor_guid.guid.data4[4] = req_params->vendor_namespace_guid.guid4[4];
    set_var.vendor_guid.guid.data4[5] = req_params->vendor_namespace_guid.guid4[5];
    set_var.vendor_guid.guid.data4[6] = req_params->vendor_namespace_guid.guid4[6];
    set_var.vendor_guid.guid.data4[7] = req_params->vendor_namespace_guid.guid4[7];

    //! check for valid combination of attributes
    BUG_ASSERT(variable_services_check_attribute(req_params->attributes) == KNG_SUCCESS);
    set_var.attributes = req_params->attributes;
    set_var.data_size = req_params->data_size;

    //! Debug prints for testing
    DEBUG_PRINT("Variable:          Name[%s] Size[%d]\n", (char*)req_params->variable_name_ptr, req_params->variable_name_size);
    DEBUG_PRINT("Shared Mem:        Addr[0x%x] Size[%d] bytes\n", mem_ctx->payload_base, mem_ctx->max_payload_size);
    DEBUG_PRINT("Vendor GUID:       GUID1[0x%" PRIx32
                "] GUID2[0x%x] GUID3[0x%x] GUID4[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]\n",
                set_var.vendor_guid.guid.data1,
                set_var.vendor_guid.guid.data2,
                set_var.vendor_guid.guid.data3,
                set_var.vendor_guid.guid.data4[0],
                set_var.vendor_guid.guid.data4[1],
                set_var.vendor_guid.guid.data4[2],
                set_var.vendor_guid.guid.data4[3],
                set_var.vendor_guid.guid.data4[4],
                set_var.vendor_guid.guid.data4[5],
                set_var.vendor_guid.guid.data4[6],
                set_var.vendor_guid.guid.data4[7]);
    DEBUG_PRINT("Attributes:        [0x%" PRIx32 "]\n", set_var.attributes);
    DEBUG_PRINT("Data Size:         [%" PRId32 "] bytes\n", set_var.data_size);
    for (size_t i = 0; i < set_var.data_size; i++)
    {
        DEBUG_PRINT("Data byte [%zu]:   [0x%02x]\n", i, ((unsigned char*)req_params->data)[i]);
    }
    uint32_t total_size = sizeof(set_var) + req_params->variable_name_size + set_var.data_size;
    DEBUG_PRINT("Total Size:        [%" PRId32 "] bytes\n", total_size);

    //! Check if the total size of data to be copied is within the max payload size allocated in shared memory
    BUG_ASSERT(total_size <= mem_ctx->max_payload_size);

    //! copy over the set variable structure, variable name and data into the shared memory
    memcpy((void*)mem_ctx->payload_base, (void*)&set_var, sizeof(set_var));
    memcpy((void*)mem_ctx->payload_base + sizeof(set_var), (void*)req_params->variable_name_ptr, req_params->variable_name_size);
    memcpy((void*)mem_ctx->payload_base + sizeof(set_var) + req_params->variable_name_size,
           (void*)req_params->data,
           set_var.data_size);

    //! Debug prints for testing
    DEBUG_PRINT("Shared Memory Offsets: set_var[0x0]   variable_name_ptr[0x%x]    data[0x%x]\n",
                sizeof(set_var),
                sizeof(set_var) + req_params->variable_name_size);
    DEBUG_PRINT("Shared Memory Req Dump\n");
    for (size_t i = 0; i < total_size; i++)
    {
        DEBUG_PRINT("Byte Index [%zu]: [0x%02x]\n", i, ((unsigned char*)mem_ctx->payload_base)[i]);
    }

    //! populate mbox request struct to send the set variable request to the HSP
    kng_hsp_mailbox_msg msg = {};
    msg.header.cmd = HSP_MAILBOX_CMD_SET_VARIABLE_REQ;
    msg.as_uint32[1] = mem_ctx->payload_base; //! set_variable_address field
    size_t recv_msg_size_bytes = 0;

    //! Debug prints for testing
    DEBUG_PRINT("Preparing to send set variable request to HSP\n");
    DEBUG_PRINT("HSP Mbox Req Mesg: Cmd[0x%x] Addr[0x%" PRIx32 "]\n", msg.header.cmd, msg.as_uint32[1]);
    for (size_t i = 0; i < sizeof(msg.as_uint32) / sizeof(msg.as_uint32[0]); i++)
    {
        DEBUG_PRINT("as_uint32[%zu]: [0x%" PRIx32 "]\n", i, msg.as_uint32[i]);
    }

    //! Send the set variable sync request to the HSP & get response, blocking call
    fpfw_status_t icc_status =
        fpfw_icc_base_send_recv_sync(hsp_icc_ctx, &msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);

    //! Debug prints for testing
    DEBUG_PRINT("Response for variable set received, status [%" PRId32 "]\n", icc_status);
    DEBUG_PRINT("Response message size [%d] bytes\n", recv_msg_size_bytes);
    DEBUG_PRINT("HSP Mbox Rsp Mesg: Cmd[0x%x] Status[0x%" PRIx32 "] Status_ex[0x%" PRIx32 "]\n",
                msg.rsp.header.cmd,
                msg.as_uint32[1],
                msg.as_uint32[2]);
    for (size_t i = 0; i < sizeof(msg.as_uint32) / sizeof(msg.as_uint32[0]); i++)
    {
        DEBUG_PRINT("as_uint32[%zu]: [0x%" PRIx32 "]\n", i, msg.as_uint32[i]);
    }

    //! Verify sync return status & response message
    BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
    BUG_ASSERT(recv_msg_size_bytes > 0);
    BUG_ASSERT(msg.rsp.header.cmd == HSP_MAILBOX_CMD_SET_VARIABLE_RSP);
    BUG_ASSERT(msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);

    //! The response data at this point is written over to the shared memory region supplied by the caller
    DEBUG_PRINT("Shared Memory Response Dump\n");
    for (size_t i = 0; i < total_size; i++)
    {
        DEBUG_PRINT("Byte Index [%zu]: [0x%02x]\n", i, ((unsigned char*)mem_ctx->payload_base)[i]);
    }
    DEBUG_PRINT("----End of Sync Set Variable----\n");
}

void variable_service_sync_get_variable(var_service_shared_mem_t* mem_ctx, var_service_req_params_t* req_params)
{
    if (!system_info_is_hsp_present())
    {
        DEBUG_PRINT("HSP not present, skipping set variable\n");
        return;
    }

    DEBUG_PRINT("----Sync Get Variable----\n");
    //! Null checks for input parameters
    BUG_ASSERT(mem_ctx != NULL);
    BUG_ASSERT(mem_ctx->payload_base != 0);
    BUG_ASSERT(mem_ctx->max_payload_size != 0);
    BUG_ASSERT(req_params != NULL);
    BUG_ASSERT(req_params->variable_name_ptr != NULL);
    BUG_ASSERT(req_params->variable_name_size != 0);
    BUG_ASSERT(req_params->data != NULL);
    BUG_ASSERT(req_params->data_size != 0);

    //! get the icc base context object
    fpfw_icc_base_ctx_t* hsp_icc_ctx = get_icc_base_ctx();
    BUG_ASSERT(hsp_icc_ctx != NULL);

    //! populate the mbox get variable structure
    struct hsp_mbox_get_variable get_var = {};
    get_var.variable_name_size = req_params->variable_name_size / sizeof(uint16_t); //! No of 16-bit characters
    get_var.vendor_guid.guid.data1 = req_params->vendor_namespace_guid.guid1;
    get_var.vendor_guid.guid.data2 = req_params->vendor_namespace_guid.guid2;
    get_var.vendor_guid.guid.data3 = req_params->vendor_namespace_guid.guid3;
    get_var.vendor_guid.guid.data4[0] = req_params->vendor_namespace_guid.guid4[0];
    get_var.vendor_guid.guid.data4[1] = req_params->vendor_namespace_guid.guid4[1];
    get_var.vendor_guid.guid.data4[2] = req_params->vendor_namespace_guid.guid4[2];
    get_var.vendor_guid.guid.data4[3] = req_params->vendor_namespace_guid.guid4[3];
    get_var.vendor_guid.guid.data4[4] = req_params->vendor_namespace_guid.guid4[4];
    get_var.vendor_guid.guid.data4[5] = req_params->vendor_namespace_guid.guid4[5];
    get_var.vendor_guid.guid.data4[6] = req_params->vendor_namespace_guid.guid4[6];
    get_var.vendor_guid.guid.data4[7] = req_params->vendor_namespace_guid.guid4[7];
    get_var.attributes_size = req_params->attributes_size;
    get_var.data_size = req_params->data_size;

    //! Debug prints for testing
    DEBUG_PRINT("Variable:          Name[%s] Size[%d]\n", (char*)req_params->variable_name_ptr, req_params->variable_name_size);
    DEBUG_PRINT("Shared Mem:        Addr[0x%x] Size[%d] bytes\n", mem_ctx->payload_base, mem_ctx->max_payload_size);
    DEBUG_PRINT("Vendor GUID:       GUID1[0x%" PRIx32
                "] GUID2[0x%x] GUID3[0x%x] GUID4[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]\n",
                get_var.vendor_guid.guid.data1,
                get_var.vendor_guid.guid.data2,
                get_var.vendor_guid.guid.data3,
                get_var.vendor_guid.guid.data4[0],
                get_var.vendor_guid.guid.data4[1],
                get_var.vendor_guid.guid.data4[2],
                get_var.vendor_guid.guid.data4[3],
                get_var.vendor_guid.guid.data4[4],
                get_var.vendor_guid.guid.data4[5],
                get_var.vendor_guid.guid.data4[6],
                get_var.vendor_guid.guid.data4[7]);
    DEBUG_PRINT("Attributes Size:   [%" PRId32 "] bytes\n", get_var.attributes_size);
    DEBUG_PRINT("Data Size:         [%" PRId32 "] bytes\n", get_var.data_size);
    uint32_t total_size = sizeof(get_var) + req_params->variable_name_size + get_var.data_size;
    DEBUG_PRINT("Projected Total Size:        [%" PRId32 "] bytes\n", total_size);

    //! Check if the total size of data to be copied is within the max payload size allocated in shared memory
    BUG_ASSERT(total_size <= mem_ctx->max_payload_size);

    //! copy over the get variable structure, variable name into the shared memory
    memcpy((void*)mem_ctx->payload_base, (void*)&get_var, sizeof(get_var));
    memcpy((void*)mem_ctx->payload_base + sizeof(get_var), (void*)req_params->variable_name_ptr, req_params->variable_name_size);
    uint32_t data_offset = sizeof(get_var) + req_params->variable_name_size;

    //! Debug prints for testing
    DEBUG_PRINT("Shared Memory Offsets: get_var[0x0]   variable_name_ptr[0x%x]    data[0x%" PRIx32 "]\n",
                sizeof(get_var),
                data_offset);
    DEBUG_PRINT("Shared Memory Req Dump\n");
    for (size_t i = 0; i < total_size; i++)
    {
        DEBUG_PRINT("Byte Index [%zu]: [0x%02x]\n", i, ((unsigned char*)mem_ctx->payload_base)[i]);
    }

    //! populate mbox request struct to send the get variable request to the HSP
    kng_hsp_mailbox_msg msg = {};
    msg.header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_REQ;
    msg.as_uint32[1] = mem_ctx->payload_base; //! get_variable_address field
    size_t recv_msg_size_bytes = 0;

    //! Debug prints for testing
    DEBUG_PRINT("Preparing to send get variable request to HSP\n");
    DEBUG_PRINT("HSP Mbox Req Mesg: Cmd[0x%x] Addr[0x%" PRIx32 "]\n", msg.header.cmd, msg.as_uint32[1]);
    for (size_t i = 0; i < sizeof(msg.as_uint32) / sizeof(msg.as_uint32[0]); i++)
    {
        DEBUG_PRINT("as_uint32[%zu]: [0x%" PRIx32 "]\n", i, msg.as_uint32[i]);
    }

    //! Send the get variable sync request to the HSP & get response, blocking call
    fpfw_status_t icc_status =
        fpfw_icc_base_send_recv_sync(hsp_icc_ctx, &msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);

    //! Debug prints for testing
    DEBUG_PRINT("Response for variable get received, status [%" PRId32 "]\n", icc_status);
    DEBUG_PRINT("Response message size [%d] bytes\n", recv_msg_size_bytes);
    DEBUG_PRINT("HSP Mbox Rsp Mesg: Cmd[0x%x] Status[0x%" PRIx32 "] Status_ex[0x%" PRIx32 "]\n",
                msg.rsp.header.cmd,
                msg.as_uint32[1],
                msg.as_uint32[2]);
    for (size_t i = 0; i < sizeof(msg.as_uint32) / sizeof(msg.as_uint32[0]); i++)
    {
        DEBUG_PRINT("as_uint32[%zu]: [0x%" PRIx32 "]\n", i, msg.as_uint32[i]);
    }

    //! Verify sync return status & response message
    BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
    BUG_ASSERT(recv_msg_size_bytes > 0);
    BUG_ASSERT(msg.rsp.header.cmd == HSP_MAILBOX_CMD_GET_VARIABLE_RSP);
    BUG_ASSERT(msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);

    //! The response data at this point is written over to the shared memory region supplied by the caller
    DEBUG_PRINT("Shared Memory Response Dump\n");
    for (size_t i = 0; i < total_size; i++)
    {
        DEBUG_PRINT("Byte Index [%zu]: [0x%02x]\n", i, ((unsigned char*)mem_ctx->payload_base)[i]);
    }
    DEBUG_PRINT("----End of Sync Get Variable----\n");

    //! Reset data buffer & opy data over from shared memory into the data buffer supplied in request params
    memset(req_params->data, 0, req_params->data_size);
    memcpy((void*)req_params->data, (void*)mem_ctx->payload_base + data_offset, get_var.data_size);
}
