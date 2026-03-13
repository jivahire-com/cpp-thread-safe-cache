//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_cli.c
 * Implementation for variable services cli commands
 * These commands are used to demonstrate the usage of variable services async APIs
 * A more comprehensive usage of variable services via cli will be added via config knob cli
 */

/*----------- Nested includes ------------*/
#include <FpFwCli.h> // for FPFW_CLI_COMMAND, FPFW_CLI_STATUS, FpFwCliRegisterTable
#include <FpFwUtils.h>
#include <bug_check.h>
#include <kng_error.h> // for KNG_E_INVALIDARG, KNG_E_NOTIMPL
#include <stdlib.h>    // for atoi
#include <string.h>
#include <utils.h>
#include <variable_services.h>     // for var_service_shared_mem_t, var_serv...
#include <variable_services_cli.h> // for variable_services_sync_get_vari...

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_GUID                                          \
    {                                                      \
        0x3363AE8A, 0xDAB5, 0x4DCA,                        \
        {                                                  \
            0xBF, 0x32, 0xDD, 0x0E, 0x65, 0x89, 0x95, 0xC5 \
        }                                                  \
    }
#define TEST_VARIABLE_ASYNC_DATA_SIZE 64
/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static var_service_req_ctx_t s_req_ctx = {};
static const guid_t vendor_guid[] = {TEST_GUID};
static uint8_t set_var_data_buffer[TEST_VARIABLE_ASYNC_DATA_SIZE] = {0};
static uint8_t set_var_test_counter = 0;
static const uint16_t test_variable_name[] = {'T', 'e', 's', 't', 'V', 'a', 'r', 'A', 's', 'y', 'n', 'c', 0x00};
/*-- Static function declarations --*/
static FPFW_CLI_STATUS var_serv_async_get_var(int argc, const char** argv);
static FPFW_CLI_STATUS var_serv_async_set_var(int argc, const char** argv);

static FPFW_CLI_COMMAND s_variable_serv_cmd_list[] = {
    {NULL_LIST_ENTRY, "variable_serv", "get_var", var_serv_async_get_var, "get variable value from HSP", "syntax: get_var\n"},
    {NULL_LIST_ENTRY, "variable_serv", "set_var", var_serv_async_set_var, "set new variable value", "syntax: set_var\n"},
};

/*--------- Function Prototypes ----------*/
PLACED_CODE void variable_services_cli_init(var_service_shared_mem_t* mem_ctx)
{
    if (mem_ctx == NULL)
    {
        FpFwCliPrint("[var_serv] Error: mem_ctx is NULL\n");
        return;
    }
    //! reset shared memory region
    memset((void*)mem_ctx->payload_base, 0, mem_ctx->max_payload_size);
    //! initialize variable services ctx for the region
    variable_service_initialize_ctx(&s_req_ctx, mem_ctx);
    //! register the cli commands
    FpFwCliRegisterTable(s_variable_serv_cmd_list, FPFW_ARRAY_SIZE(s_variable_serv_cmd_list));
}

PLACED_CODE void test_variable_service_req_complete_notify(void* context,
                                                           struct _variable_service_req_ctx* var_serv_ctx,
                                                           uint8_t* data_start_ptr,
                                                           size_t data_size)
{
    FPFW_UNUSED(context);
    BUG_ASSERT(var_serv_ctx != NULL);   // NOLINT
    BUG_ASSERT(data_start_ptr != NULL); // NOLINT
    BUG_ASSERT(data_size > 0);

    const char* operation_str = (var_serv_ctx->operation_type == ASYNC_GET_VARIABLE) ? "Get" : "Set";
    FpFwCliPrint("[var_serv] Async %s Variable Complete Notify\n", operation_str);
    BUG_ASSERT(var_serv_ctx->req_params.variable_name_size == sizeof(test_variable_name));
    FpFwCliPrint("[var_serv] Variable Name: ");
    for (size_t i = 0; i < var_serv_ctx->req_params.variable_name_size / sizeof(uint16_t); ++i)
    {
        FpFwCliPrint("%c", var_serv_ctx->req_params.variable_name_ptr[i]);
    }
    FpFwCliPrint("\n");

    if (var_serv_ctx->operation_type == ASYNC_GET_VARIABLE)
    {
        BUG_ASSERT(data_size == TEST_VARIABLE_ASYNC_DATA_SIZE);
        FpFwCliPrint("[var_serv] Data received (%ld bytes):\n", data_size); // NOLINT

        if (KNG_SUCCEEDED(var_serv_ctx->async_req_result))
        {
            for (size_t i = 0; i < data_size; ++i)
            {
                FpFwCliPrint("%02X ", data_start_ptr[i]); // NOLINT
            }
            FpFwCliPrint("\n");
        }
        else
        {
            FpFwCliPrint("[var_serv] Err! Variable Name not found\n");
        }
        variable_service_unlock_get_var_ctx(var_serv_ctx);
        FpFwCliPrint("[var_serv] Get Variable Ctx Freed!\n");
    }
    FpFwCliPrint("[var_serv] Async %s Variable Done\n", operation_str);
}

static PLACED_CODE FPFW_CLI_STATUS var_serv_async_get_var(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);
    FpFwCliPrint("[var_serv] Async Get Variable Initiate Test, Set Var Count[%d]\n", set_var_test_counter);

    //! Populate the get variable request params,
    //! variable_name_ptr, variable_name_size & vendor guid are mandatory
    //! data_size & attributes_size are optional
    //! data is an optional output parameter for get variable, ignored
    var_service_req_params_t s_get_var_req = {0};
    s_get_var_req.variable_name_ptr = (uint16_t*)test_variable_name;
    s_get_var_req.variable_name_size = sizeof(test_variable_name);
    memcpy(&s_get_var_req.vendor_namespace_guid, vendor_guid, sizeof(s_get_var_req.vendor_namespace_guid));
    s_get_var_req.data_size = TEST_VARIABLE_ASYNC_DATA_SIZE;
    s_get_var_req.attributes_size = 0;

    FpFwCliPrint("[var_serv] Using default params:\n");
    FpFwCliPrint("[var_serv] Variable Name: ");
    for (size_t i = 0; i < s_get_var_req.variable_name_size / sizeof(uint16_t); ++i)
    {
        FpFwCliPrint("%c", s_get_var_req.variable_name_ptr[i]);
    }
    FpFwCliPrint(" Size: %u\n", s_get_var_req.variable_name_size);
    FpFwCliPrint("[var_serv] Vendor Guid: ");
    for (size_t i = 0; i < sizeof(s_get_var_req.vendor_namespace_guid); ++i)
    {
        FpFwCliPrint("%02X ", ((uint8_t*)&s_get_var_req.vendor_namespace_guid)[i]);
    }
    FpFwCliPrint("\n");
    FpFwCliPrint("[var_serv] data_size: %u\n", s_get_var_req.data_size);
    FpFwCliPrint("[var_serv] attributes_size: %u\n", s_get_var_req.attributes_size);

    // Call the function to get the variable
    int32_t status =
        variable_service_async_get_variable(&s_req_ctx, &s_get_var_req, test_variable_service_req_complete_notify, NULL);
    if (status != KNG_SUCCESS)
    {
        FpFwCliPrint("[var_serv] Get Variable Async Request Failed\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS var_serv_async_set_var(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);
    FpFwCliPrint("[var_serv] Async Set Variable Initiate Test, Count[%d]\n", set_var_test_counter);

    //! Populate the set variable request params,
    //! variable_name_ptr, variable_name_size, vendor guid, attributes & data size are mandatory
    var_service_req_params_t s_set_var_req = {0};
    s_set_var_req.variable_name_ptr = (uint16_t*)test_variable_name;
    s_set_var_req.variable_name_size = sizeof(test_variable_name);
    memcpy(&s_set_var_req.vendor_namespace_guid, vendor_guid, sizeof(s_set_var_req.vendor_namespace_guid));
    //! populate set_var_data_buffer with fake data
    for (size_t i = 0; i < sizeof(set_var_data_buffer); ++i)
    {
        set_var_data_buffer[i] = (uint8_t)(i % TEST_VARIABLE_ASYNC_DATA_SIZE) + set_var_test_counter;
    }
    s_set_var_req.data = set_var_data_buffer;
    s_set_var_req.data_size = sizeof(set_var_data_buffer);
    s_set_var_req.attributes.as_uint32 =
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    FpFwCliPrint("[var_serv] Using default params:\n");
    FpFwCliPrint("[var_serv] Variable Name: ");
    for (size_t i = 0; i < s_set_var_req.variable_name_size / sizeof(uint16_t); ++i)
    {
        FpFwCliPrint("%c", s_set_var_req.variable_name_ptr[i]);
    }
    FpFwCliPrint(" Size: %u\n", s_set_var_req.variable_name_size);
    FpFwCliPrint("[var_serv] Vendor Guid: ");
    for (size_t i = 0; i < sizeof(s_set_var_req.vendor_namespace_guid); ++i)
    {
        FpFwCliPrint("%02X ", ((uint8_t*)&s_set_var_req.vendor_namespace_guid)[i]);
    }
    FpFwCliPrint("\n");
    FpFwCliPrint("[var_serv] Data: ");
    for (size_t i = 0; i < s_set_var_req.data_size; ++i)
    {
        FpFwCliPrint("%02X ", s_set_var_req.data[i]);
    }
    FpFwCliPrint("\n");
    FpFwCliPrint("[var_serv] Data Size: %u\n", s_set_var_req.data_size);
    FpFwCliPrint("[var_serv] Attributes: %u\n", s_set_var_req.attributes.as_uint32);

    // Call the function to set the variable
    int32_t status =
        variable_service_async_set_variable(&s_req_ctx, &s_set_var_req, test_variable_service_req_complete_notify, NULL);
    if (status != KNG_SUCCESS)
    {
        FpFwCliPrint("[var_serv] Set Variable Async Request Failed\n");
        return CLI_ERROR;
    }
    set_var_test_counter++;
    return CLI_SUCCESS;
}