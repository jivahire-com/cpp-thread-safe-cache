//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_variable_services.cpp
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, check_expected, CmockaWra...

extern "C" {
#include <FpFwUtils.h>            // for FPFW_UNUSED
#include <fpfw_icc_base.h>        // for fpfw_icc_base_ctx_t, fpfw_icc_ba...
#include <hsp_firmware_headers.h> // for kng_hsp_mailbox_msg, HSP_MAILBOX_RSP...
#include <kng_error.h>
#include <stdint.h>                   // for int32_t, uint32_t
#include <variable_services.h>        // for var_service_shared_mem_t, var_serv...
#include <variable_services_helper.h> // for variable_services_sync_get_vari...
#include <variable_services_mem.h>    // for variable_service_mem_metadata_t

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_GUID                                          \
    {                                                      \
        0x3363AE8A, 0xDAB5, 0x4DCA,                        \
        {                                                  \
            0xBF, 0x32, 0xDD, 0x0E, 0x65, 0x89, 0x95, 0xC5 \
        }                                                  \
    }

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t test_rsp_data[] = {0x11, 0x22, 0x33, 0x44, 0x55};
static uint8_t rmss_shared_ram_region[128] = {0};
static var_service_shared_mem_t shared_mem = {
    .payload_base = (uintptr_t)rmss_shared_ram_region,
    .max_payload_size = sizeof(rmss_shared_ram_region),
};
static var_service_req_ctx_t req_ctx = {};
static uint32_t dummy_icc_base_ctx = 0;

/*------------- Mock Functions ----------------*/
fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(buffer_size);

    //! prepare the hsp mbox response
    kng_hsp_mailbox_msg* msg = (kng_hsp_mailbox_msg*)payload_buffer;
    //! Update response status
    msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
    if (msg->header.cmd == HSP_MAILBOX_CMD_GET_VARIABLE_REQ)
    {
        //! Update resp cmd code
        msg->header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_RSP;
        //! Simulate HSP copying get var response data to shared memory region
        uint32_t shared_mem_data_offset = sizeof(variable_service_shared_mem_format_t) + sizeof("TestGetVar");
        memcpy((void*)(rmss_shared_ram_region + shared_mem_data_offset), (void*)test_rsp_data, sizeof(test_rsp_data));
    }
    else if (msg->header.cmd == HSP_MAILBOX_CMD_SET_VARIABLE_REQ)
    {
        //! Update resp cmd code
        msg->header.cmd = HSP_MAILBOX_CMD_SET_VARIABLE_RSP;
    }
    *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);

    return FPFW_ICC_BASE_STATUS_SUCCESS;
}

fpfw_status_t __wrap_fpfw_icc_base_send_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_recv_req_t* icc_req)
{
    FPFW_UNUSED(icc_ctx);
    assert_non_null(icc_req);

    //! prepare the hsp mbox response
    kng_hsp_mailbox_msg* msg = (kng_hsp_mailbox_msg*)icc_req->payload_buffer;
    //! Update response status
    msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
    if (msg->header.cmd == HSP_MAILBOX_CMD_GET_VARIABLE_REQ)
    {
        //! Update resp cmd code
        msg->header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_RSP;
        //! Simulate HSP copying get var response data to shared memory region
        uint32_t shared_mem_data_offset = sizeof(variable_service_shared_mem_format_t) + sizeof("TestGetVar");
        memcpy((void*)(rmss_shared_ram_region + shared_mem_data_offset), (void*)test_rsp_data, sizeof(test_rsp_data));
    }
    else if (msg->header.cmd == HSP_MAILBOX_CMD_SET_VARIABLE_REQ)
    {
        //! Update resp cmd code
        msg->header.cmd = HSP_MAILBOX_CMD_SET_VARIABLE_RSP;
    }
    else
    {
        assert_true(false);
    }
    return FPFW_ICC_BASE_STATUS_SUCCESS;
}

fpfw_icc_base_ctx_t* __wrap_get_icc_base_ctx(void)
{
    //! dummy icc base ctx for test
    return (fpfw_icc_base_ctx_t*)&dummy_icc_base_ctx;
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    FPFW_UNUSED(Lock);
    return ((FPFW_LOCK_STATE)0);
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    FPFW_UNUSED(Lock);
    FPFW_UNUSED(OldState);
}

/*------------- Test Setup/Teardown ----------------*/
static int teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    //! Reset the shared memory region
    variable_service_unlock_get_var_ctx(&req_ctx);
    return 0;
}

static int setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    variable_service_initialize_ctx(&req_ctx, &shared_mem);
    return 0;
}

/*------------- Test Cases ----------------*/
TEST_FUNCTION(test_variable_services_sync_get_variable, setup, teardown)
{
    //! client buffer to store the response data
    uint8_t client_buffer[5] = {0};

    //! prepare the get variable request
    var_service_req_params_t get_var_req = {
        .variable_name_ptr = (uint16_t*)"TestGetVar",
        .variable_name_size = sizeof("TestGetVar"),
        .vendor_namespace_guid = TEST_GUID,
        .data_size = sizeof(client_buffer),
        .data = client_buffer,
    };

    //! Invoke the FUT
    variable_service_sync_get_variable(&req_ctx, &get_var_req);

    //! verify the client buffer is updated with the test response data
    assert_memory_equal(client_buffer, test_rsp_data, sizeof(client_buffer));

    //! Invoke FUT for a 2nd time without freeing memory will result in assert
    //! shared memory is not freed by variable services for get variable request
    variable_service_unlock_get_var_ctx(&req_ctx);

    //! Invoke the FUT, must pass
    variable_service_sync_get_variable(&req_ctx, &get_var_req);
}

TEST_FUNCTION(test_variable_services_sync_set_variable, setup, teardown)
{
    //! client's data to be written to the shared memory region
    uint8_t client_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    //! prepare the set variable request
    var_service_req_params_t set_var_req = {
        .variable_name_ptr = (uint16_t*)"TestSetVar",
        .variable_name_size = sizeof("TestSetVar"),
        .vendor_namespace_guid = TEST_GUID,
        .data_size = sizeof(client_data),
        .data = client_data,
        .attributes =
            {
                .as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
            },
    };

    //! Invoke the FUT
    variable_service_sync_set_variable(&req_ctx, &set_var_req);

    //! verify the shared memory is updated with the client data
    uint32_t shared_mem_data_offset = sizeof(variable_service_shared_mem_format_t) + sizeof("TestSetVar");
    assert_memory_equal((void*)(rmss_shared_ram_region + shared_mem_data_offset), client_data, sizeof(client_data));

    //! Invoke FUT for a 2nd time, must pass as shared memory is freed by variable services post the 1st request completion
    variable_service_sync_set_variable(&req_ctx, &set_var_req);
}

TEST_FUNCTION(test_variable_service_initialize_ctx, nullptr, nullptr)
{
    //! mock shared memory region locally for test
    uint8_t shared_memory[100];
    var_service_shared_mem_t dummy_shared_mem = {
        .payload_base = (uintptr_t)shared_memory,
        .max_payload_size = sizeof(shared_memory),
    };
    var_service_req_ctx_t dummy_req_ctx = {};
    memset(shared_memory, 0xFFFFFFFF, sizeof(shared_memory));
    //! verify ctx is not initialized
    assert_false(dummy_req_ctx.is_initialized);

    //! Invoke the FUT, must be called once per unique shared memory region
    //! if different variables use different shared memory regions, then this API must be called for each variable
    assert_int_equal(variable_service_initialize_ctx(&dummy_req_ctx, &dummy_shared_mem), KNG_SUCCESS);
    assert_int_equal(dummy_req_ctx.shared_mem.max_payload_size, sizeof(shared_memory));
    assert_memory_equal((void*)shared_memory,
                        (void*)dummy_req_ctx.shared_mem.payload_base,
                        dummy_req_ctx.shared_mem.max_payload_size);
    variable_service_shared_mem_format_t* shared_mem =
        (variable_service_shared_mem_format_t*)dummy_req_ctx.shared_mem.payload_base;
    assert_true(shared_mem->metadata.sync.bitfield.is_in_use == 0);

    //! verify ctx is initialized
    assert_true(dummy_req_ctx.is_initialized);
}

TEST_FUNCTION(test_variable_service_unlock_get_var_ctx, nullptr, nullptr)
{
    //! mock shared memory region locally for test
    uint8_t shared_memory[100];
    //! emulate initialized ctx
    var_service_req_ctx_t dummy_req_ctx = {
        .shared_mem =
            {
                .payload_base = (uintptr_t)shared_memory,
                .max_payload_size = sizeof(shared_memory),
            },
        .is_initialized = true,
    };
    memset(shared_memory, 0xFFFFFFFF, sizeof(shared_memory));

    //! Invoke the FUT
    variable_service_unlock_get_var_ctx(&dummy_req_ctx);
    //! verify ctx is reset with exception of shared memory mapping
    assert_int_equal(dummy_req_ctx.shared_mem.max_payload_size, sizeof(shared_memory));
    assert_memory_equal((void*)shared_memory,
                        (void*)dummy_req_ctx.shared_mem.payload_base,
                        dummy_req_ctx.shared_mem.max_payload_size);
    variable_service_shared_mem_format_t* shared_mem =
        (variable_service_shared_mem_format_t*)dummy_req_ctx.shared_mem.payload_base;
    assert_true(shared_mem->metadata.sync.bitfield.is_in_use == 0);
    assert_true(dummy_req_ctx.is_initialized);
}

void test_variable_service_req_complete_notify(void* context,
                                               struct _variable_service_req_ctx* var_serv_ctx,
                                               uint8_t* data_start_ptr,
                                               size_t data_size)
{
    FPFW_UNUSED(context);
    assert_non_null(var_serv_ctx);
    //! cb output params is populated with the response data
    if (var_serv_ctx->operation_type == ASYNC_GET_VARIABLE)
    {
        //! for get variable, user gets the response data in the shared memory region thru data_start_ptr & data_size
        assert_true(var_serv_ctx->req_params.variable_name_size == sizeof("TestGetVar"));
        assert_memory_equal(data_start_ptr, test_rsp_data, data_size);
    }
    else if (var_serv_ctx->operation_type == ASYNC_SET_VARIABLE)
    {
        //! for set variable, user can verify the data written to the shared memory region thru data_start_ptr & data_size
        //! immediately post the request completion, the shared memory is freed by variable services
        assert_true(var_serv_ctx->req_params.variable_name_size == sizeof("TestSetVar"));
        assert_memory_equal(data_start_ptr, var_serv_ctx->req_params.data, data_size);
    }
    else
    {
        assert_true(false);
    }
    function_called();
}

TEST_FUNCTION(test_variable_services_async_get_variable, setup, teardown)
{
    //! client buffer to store the response data,
    uint8_t client_buffer[5] = {0};

    //! prepare the get variable request
    var_service_req_params_t get_var_req = {
        .variable_name_ptr = (uint16_t*)"TestGetVar",
        .variable_name_size = sizeof("TestGetVar"),
        .vendor_namespace_guid = TEST_GUID,
        .data_size = sizeof(client_buffer),
        .data = client_buffer,
    };

    //! Verify cb is initialized
    assert_true(req_ctx.is_initialized);

    //! Invoke the FUT with mocked icc base APIs, must pass
    assert_int_equal(variable_service_async_get_variable(&req_ctx, &get_var_req, test_variable_service_req_complete_notify, NULL),
                     KNG_SUCCESS);

    //! Expect client's cb to be invoked
    expect_function_call(test_variable_service_req_complete_notify);

    //! simulate icc internal cb invocation post dfwk request completion when HSP has responded
    req_ctx.icc_req.cb(req_ctx.icc_req.cb_ctx, sizeof(kng_hsp_mailbox_msg), FPFW_ICC_BASE_STATUS_SUCCESS);

    //! Invoking the FUT will result in assert, Post cb verification, the shared memory is still not free, any
    //! subsequent request will ASSERT for get variable request, shared memory should be freed by the client
    variable_service_unlock_get_var_ctx(&req_ctx);

    //! Invoke the FUT, must pass
    assert_int_equal(variable_service_async_get_variable(&req_ctx, &get_var_req, test_variable_service_req_complete_notify, NULL),
                     KNG_SUCCESS);
}

TEST_FUNCTION(test_variable_services_async_set_variable, setup, teardown)
{
    //! client's data to be written to the shared memory region
    uint8_t client_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    //! prepare the set variable request
    var_service_req_params_t set_var_req = {
        .variable_name_ptr = (uint16_t*)"TestSetVar",
        .variable_name_size = sizeof("TestSetVar"),
        .vendor_namespace_guid = TEST_GUID,
        .data_size = sizeof(client_data),
        .data = client_data,
        .attributes =
            {
                .as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
            },
    };

    //! Verify cb is initialized
    assert_true(req_ctx.is_initialized);

    //! Invoke the FUT with mocked icc base APIs, must pass
    assert_int_equal(variable_service_async_set_variable(&req_ctx, &set_var_req, test_variable_service_req_complete_notify, NULL),
                     KNG_SUCCESS);

    //! Expect client's cb to be invoked
    expect_function_call(test_variable_service_req_complete_notify);

    //! simulate icc internal cb invocation post dfwk request completion when HSP has responded & consumed from shared memory
    req_ctx.icc_req.cb(req_ctx.icc_req.cb_ctx, sizeof(kng_hsp_mailbox_msg), FPFW_ICC_BASE_STATUS_SUCCESS);

    //! Invoke the FUT, must pass, post cb, shared memory is freed by variable services, hence no need to free shared mem explicitly
    assert_int_equal(variable_service_async_set_variable(&req_ctx, &set_var_req, test_variable_service_req_complete_notify, NULL),
                     KNG_SUCCESS);
}
}