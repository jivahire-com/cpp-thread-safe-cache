//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_variable_services.cpp
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, check_expected, CmockaWra...
#include <stddef.h>        // for offsetof
#include <string.h>        // for memcpy,
extern "C" {
#include <FpFwUtils.h>            // for FPFW_UNUSED
#include <fpfw_icc_base.h>        // for fpfw_icc_base_ctx_t, fpfw_icc_ba...
#include <hsp_firmware_headers.h> // for kng_hsp_mailbox_msg, HSP_MAILBOX_RSP...
#include <kng_error.h>
#include <silibs_common.h>
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

/* Use real UTF-16 names so variable_name_size is even (bytes) */
static const char16_t kGetName[] = u"TestGetVar";
static const char16_t kSetName[] = u"TestSetVar";

/* Controls for wrappers to hit additional (legal) branches */
static bool g_force_icc_fail = false;
static bool g_force_get_not_found = false;

/*------------- Mock Functions ----------------*/

/*------------- Local Helpers ----------------*/
static inline uint8_t* get_shared_mem_base(void* payload_buffer)
{
    /* payload_buffer points to &shared_mem->metadata.msg in prod code */
    return ((uint8_t*)payload_buffer) - offsetof(variable_service_shared_mem_format_t, metadata.msg);
}

static inline uint32_t get_rsp_data_offset_bytes(void* payload_buffer)
{
    volatile variable_service_shared_mem_format_t* shm =
        (volatile variable_service_shared_mem_format_t*)get_shared_mem_base(payload_buffer);
    /* variable_name_size in shared mem is number of uint16 chars */
    uint32_t name_bytes = (uint32_t)(shm->variable_name_size * sizeof(uint16_t));
    return (uint32_t)sizeof(variable_service_shared_mem_format_t) + name_bytes;
}

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

        uint8_t* base = get_shared_mem_base(payload_buffer);
        uint32_t shared_mem_data_offset = get_rsp_data_offset_bytes(payload_buffer);
        memcpy((void*)(base + shared_mem_data_offset), (void*)test_rsp_data, sizeof(test_rsp_data));
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

    /* Force ICC failure to cover the error-return path in async common handler */
    if (g_force_icc_fail)
    {
        return (fpfw_status_t)(FPFW_ICC_BASE_STATUS_SUCCESS + 1);
    }

    //! prepare the hsp mbox response
    kng_hsp_mailbox_msg* msg = (kng_hsp_mailbox_msg*)icc_req->payload_buffer;
    //! Update response status
    msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
    if (msg->header.cmd == HSP_MAILBOX_CMD_GET_VARIABLE_REQ)
    {
        //! Update resp cmd code
        msg->header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_RSP;

        /* Optionally force NOT_FOUND to cover that async-callback branch */
        if (g_force_get_not_found)
        {
            msg->rsp.status = HSP_MAILBOX_RSP_STATUS_NOT_FOUND;

            /* For NOT_FOUND, emulate no payload returned */
            volatile variable_service_shared_mem_format_t* shm =
                (volatile variable_service_shared_mem_format_t*)get_shared_mem_base(icc_req->payload_buffer);
            shm->data_size = 0;
        }
        else
        {
            //! Simulate HSP copying get var response data to shared memory region
            uint8_t* base = get_shared_mem_base(icc_req->payload_buffer);
            uint32_t shared_mem_data_offset = get_rsp_data_offset_bytes(icc_req->payload_buffer);
            memcpy((void*)(base + shared_mem_data_offset), (void*)test_rsp_data, sizeof(test_rsp_data));
        }
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

    /* Reset wrapper controls for each test */
    g_force_icc_fail = false;
    g_force_get_not_found = false;

    variable_service_initialize_ctx(&req_ctx, &shared_mem);
    return 0;
}

void test_variable_service_req_complete_notify_not_found(void* context,
                                                         struct _variable_service_req_ctx* var_serv_ctx,
                                                         uint8_t* data_start_ptr,
                                                         size_t data_size)
{
    FPFW_UNUSED(context);
    assert_non_null(var_serv_ctx);
    assert_non_null(data_start_ptr);

    /* GET NOT_FOUND: async layer should translate to KNG_E_NOT_FOUND and no payload */
    assert_int_equal(var_serv_ctx->operation_type, ASYNC_GET_VARIABLE);

    /*
     * NOTE: On Windows/MSVC + cmocka, KNG_* error codes with high bit set can
     * trigger sign-extension mismatches in assert_int_equal(). Cast explicitly.
     */
    assert_true((int32_t)var_serv_ctx->async_req_result == (int32_t)KNG_E_NOT_FOUND);

    /* NOT_FOUND implies no payload; pointer may be NULL or undefined, size must be 0 */
    assert_int_equal((int)data_size, 0);

    function_called();
}

/*------------- Test Cases ----------------*/
TEST_FUNCTION(test_variable_services_sync_get_variable, setup, teardown)
{
    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, dsize, shared_mem.max_payload_size, -1);

    //! client buffer to store the response data
    uint8_t client_buffer[5] = {0};

    //! prepare the get variable request
    var_service_req_params_t get_var_req = {
        .variable_name_ptr = (uint16_t*)kGetName,
        .variable_name_size = sizeof(kGetName),
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

/* ---------------------------------------------------------
 * Additional tests to drive async common handler coverage >90%
 * (legal paths only: no BUG_ASSERT expected)
 * --------------------------------------------------------- */

TEST_FUNCTION(test_variable_services_async_get_variable_not_found, setup, teardown)
{
    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);

    uintptr_t aligned_base = ALIGN_DOWN(shared_mem.payload_base, 32);
    int32_t alignment_offset = (int32_t)(shared_mem.payload_base - aligned_base);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr,
                       dsize,
                       (int32_t)(shared_mem.max_payload_size + (uint32_t)alignment_offset),
                       -1);

    uint8_t client_buffer[5] = {0};
    var_service_req_params_t get_var_req = {
        .variable_name_ptr = (uint16_t*)kGetName,
        .variable_name_size = sizeof(kGetName),
        .vendor_namespace_guid = TEST_GUID,
        .data_size = sizeof(client_buffer),
        .data = client_buffer,
    };

    /* Force wrapper to reply NOT_FOUND for GET */
    g_force_get_not_found = true;

    assert_int_equal(variable_service_async_get_variable(&req_ctx, &get_var_req, test_variable_service_req_complete_notify_not_found, NULL),
                     KNG_SUCCESS);

    expect_function_call(test_variable_service_req_complete_notify_not_found);
    req_ctx.icc_req.cb(req_ctx.icc_req.cb_ctx, sizeof(kng_hsp_mailbox_msg), FPFW_ICC_BASE_STATUS_SUCCESS);

    /* GET path does not auto-unlock; free for cleanliness */
    variable_service_unlock_get_var_ctx(&req_ctx);
}

TEST_FUNCTION(test_variable_services_sync_set_variable, setup, teardown)
{
    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, dsize, shared_mem.max_payload_size, -1);

    //! client's data to be written to the shared memory region
    uint8_t client_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    //! prepare the set variable request
    var_service_req_params_t set_var_req = {
        .variable_name_ptr = (uint16_t*)kSetName,
        .variable_name_size = sizeof(kSetName),
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
    uint32_t shared_mem_data_offset =
        (uint32_t)sizeof(variable_service_shared_mem_format_t) + (uint32_t)set_var_req.variable_name_size;
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
        /* GET: validate returned payload only (name size may vary in boundary tests) */
        assert_memory_equal(data_start_ptr, test_rsp_data, data_size);
    }
    else if (var_serv_ctx->operation_type == ASYNC_SET_VARIABLE)
    {
        //! for set variable, user can verify the data written to the shared memory region thru data_start_ptr & data_size
        //! immediately post the request completion, the shared memory is freed by variable services
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
    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);

    uintptr_t aligned_base = ALIGN_DOWN(shared_mem.payload_base, 32);
    int32_t alignment_offset = (int32_t)(shared_mem.payload_base - aligned_base);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr,
                       dsize,
                       (int32_t)(shared_mem.max_payload_size + (uint32_t)alignment_offset),
                       -1);

    //! client buffer to store the response data,
    uint8_t client_buffer[5] = {0};

    //! prepare the get variable request
    var_service_req_params_t get_var_req = {
        .variable_name_ptr = (uint16_t*)kGetName,
        .variable_name_size = sizeof(kGetName),
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

TEST_FUNCTION(test_variable_services_async_get_variable_icc_send_recv_fail, setup, teardown)
{
    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);

    uintptr_t aligned_base = ALIGN_DOWN(shared_mem.payload_base, 32);
    int32_t alignment_offset = (int32_t)(shared_mem.payload_base - aligned_base);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr,
                       dsize,
                       (int32_t)(shared_mem.max_payload_size + (uint32_t)alignment_offset),
                       -1);

    uint8_t client_buffer[5] = {0};
    var_service_req_params_t get_var_req = {
        .variable_name_ptr = (uint16_t*)kGetName,
        .variable_name_size = sizeof(kGetName),
        .vendor_namespace_guid = TEST_GUID,
        .data_size = sizeof(client_buffer),
        .data = client_buffer,
    };

    /* Force wrapper to fail send/recv so common handler returns KNG_E_FAIL */
    g_force_icc_fail = true;

    assert_int_equal(variable_service_async_get_variable(&req_ctx, &get_var_req, test_variable_service_req_complete_notify, NULL),
                     KNG_E_FAIL);

    /* No async cb should be invoked on send/recv failure */
    variable_service_unlock_get_var_ctx(&req_ctx);
}

TEST_FUNCTION(test_variable_services_async_set_variable, setup, teardown)
{
    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);

    intptr_t aligned_base = ALIGN_DOWN(shared_mem.payload_base, 32);
    int32_t alignment_offset = (int32_t)(shared_mem.payload_base - aligned_base);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr,
                       dsize,
                       (int32_t)(shared_mem.max_payload_size + (uint32_t)alignment_offset),
                       -1);

    //! client's data to be written to the shared memory region
    uint8_t client_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    //! prepare the set variable request
    var_service_req_params_t set_var_req = {
        .variable_name_ptr = (uint16_t*)kGetName,
        .variable_name_size = sizeof(kGetName),
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

/* ---------------------------------------------------------
 * Legal boundary tests (NO BUG_ASSERT / NO crash_dump)
 * --------------------------------------------------------- */

TEST_FUNCTION(test_variable_services_async_get_variable_name_size_at_max_payload, setup, teardown)
{

    /* metadata-aware legal boundary */
    const uint32_t data_size = 4;
    const uint32_t max_legal_name_size =
        shared_mem.max_payload_size - sizeof(variable_service_shared_mem_format_t) - data_size;

    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);

    uintptr_t aligned_base = ALIGN_DOWN(shared_mem.payload_base, 32);
    int32_t alignment_offset = (int32_t)(shared_mem.payload_base - aligned_base);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr,
                       dsize,
                       (int32_t)(shared_mem.max_payload_size + (uint32_t)alignment_offset),
                       -1);

    /* variable name exactly equals max_payload_size (legal boundary) */

    /* allocate big UTF-16 buffer, fill bytes */
    static uint16_t big_name[128];
    /* keep it even to match UTF-16 expectations */
    const uint32_t max_legal_name_size_even = (max_legal_name_size & ~1u);
    assert_true(sizeof(big_name) >= max_legal_name_size_even);
    memset(big_name, 0, max_legal_name_size_even);

    uint8_t client_buffer[4] = {0};

    var_service_req_params_t req = {
        .variable_name_ptr = big_name,
        .variable_name_size = max_legal_name_size_even,

        .vendor_namespace_guid = TEST_GUID,
        .data_size = data_size,
        .data = client_buffer,
    };

    assert_int_equal(variable_service_async_get_variable(&req_ctx, &req, test_variable_service_req_complete_notify, NULL),
                     KNG_SUCCESS);

    expect_function_call(test_variable_service_req_complete_notify);
    req_ctx.icc_req.cb(req_ctx.icc_req.cb_ctx, sizeof(kng_hsp_mailbox_msg), FPFW_ICC_BASE_STATUS_SUCCESS);

    variable_service_unlock_get_var_ctx(&req_ctx);
}

TEST_FUNCTION(test_variable_services_async_set_variable_data_size_at_max_payload, setup, teardown)
{

    const uint32_t name_size = sizeof(kSetName);
    ;
    const uint32_t max_legal_data_size =
        shared_mem.max_payload_size - sizeof(variable_service_shared_mem_format_t) - name_size;

    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);

    uintptr_t aligned_base = ALIGN_DOWN(shared_mem.payload_base, 32);
    int32_t alignment_offset = (int32_t)(shared_mem.payload_base - aligned_base);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr,
                       dsize,
                       (int32_t)(shared_mem.max_payload_size + (uint32_t)alignment_offset),
                       -1);

    static uint8_t big_data[128];
    assert_true(sizeof(big_data) >= max_legal_data_size);
    memset(big_data, 0xAB, max_legal_data_size);

    var_service_req_params_t req = {
        .variable_name_ptr = (uint16_t*)kGetName,
        .variable_name_size = name_size,
        .vendor_namespace_guid = TEST_GUID,
        .data_size = max_legal_data_size,
        .data = big_data,
        .attributes =
            {
                .as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
            },
    };

    assert_int_equal(variable_service_async_set_variable(&req_ctx, &req, test_variable_service_req_complete_notify, NULL),
                     KNG_SUCCESS);

    expect_function_call(test_variable_service_req_complete_notify);
    req_ctx.icc_req.cb(req_ctx.icc_req.cb_ctx, sizeof(kng_hsp_mailbox_msg), FPFW_ICC_BASE_STATUS_SUCCESS);
}
}