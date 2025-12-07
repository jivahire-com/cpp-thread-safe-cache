//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_power_pldm.cpp
 * Power PLDM tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>
#include <cstring>

extern "C" {
#include <FpFwUtils.h>
#include <icc_mhu.h>
#include <kng_icc_shared.h>
#include <power_pldm.h>
#include <power_pldm_private.h>
}

/*-- Declarations (Statics and globals) --*/
struct IccMockState
{
    size_t send_call_count;
    size_t recv_call_count;
    fpfw_icc_base_ctx_t* last_send_ctx;
    fpfw_icc_base_ctx_t* last_recv_ctx;
    fpfw_icc_base_send_req_t* last_send_req;
    fpfw_icc_base_recv_req_t* last_recv_req;
    fpfw_status_t send_status;
    fpfw_status_t recv_status;
};

struct LockMockState
{
    size_t acquire_calls;
    size_t release_calls;
    PFPFW_LOCK last_acquire_lock;
    PFPFW_LOCK last_release_lock;
    FPFW_LOCK_STATE last_release_state;
    FPFW_LOCK_STATE next_state;
};

static IccMockState g_icc_mock = {};
static LockMockState g_lock_mock = {};

static void reset_mocks(void)
{
    std::memset(&g_icc_mock, 0, sizeof(g_icc_mock));
    g_icc_mock.send_status = FPFW_STATUS_SUCCESS;
    g_icc_mock.recv_status = FPFW_STATUS_SUCCESS;
    std::memset(&g_lock_mock, 0, sizeof(g_lock_mock));
}

static int setup(void** state)
{
    FPFW_UNUSED(state);
    reset_mocks();
    return 0;
}

static void test_recv_complete(void* context, size_t bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(bytes);
    FPFW_UNUSED(status);
}

extern "C" {

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    assert_non_null(params);
    function_called();

    g_icc_mock.recv_call_count++;
    g_icc_mock.last_recv_ctx = icc_ctx;
    g_icc_mock.last_recv_req = params;

    return g_icc_mock.recv_status;
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    assert_non_null(params);
    function_called();

    g_icc_mock.send_call_count++;
    g_icc_mock.last_send_ctx = icc_ctx;
    g_icc_mock.last_send_req = params;

    return g_icc_mock.send_status;
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);
    function_called();

    g_lock_mock.acquire_calls++;
    g_lock_mock.last_acquire_lock = Lock;

    return g_lock_mock.next_state;
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    assert_non_null(Lock);
    FPFW_UNUSED(OldState);
    function_called();

    g_lock_mock.release_calls++;
    g_lock_mock.last_release_lock = Lock;
    g_lock_mock.last_release_state = OldState;
}

} // extern "C"

/*------------- Tests ----------------*/
TEST_FUNCTION(test_common_send_complete_notify_retries_on_busy, setup, nullptr)
{
    uint8_t payload[sizeof(icc_mhu_header_t)] = {};
    fpfw_icc_base_send_req_t send_req = {};
    send_req.payload_buffer = payload;
    send_req.buffer_size = sizeof(payload);

    expect_function_call(__wrap_fpfw_icc_base_send);

    common_send_complete_notify(&send_req, FPFW_ICC_TRANSPORT_STATUS_BUSY);

    assert_int_equal(g_icc_mock.send_call_count, 1);
    assert_ptr_equal(g_icc_mock.last_send_req, &send_req);
}

TEST_FUNCTION(test_common_icc_base_recv_send_programs_requests, setup, nullptr)
{
    common_request_ctx_t params = {};
    uint8_t payload[sizeof(icc_mhu_header_t) + 8] = {};
    const uint32_t cmd_code = 0x55AA33u;
    uint32_t cb_context = 0x12345678u;

    expect_function_call(__wrap_fpfw_icc_base_recv);
    expect_function_call(__wrap_fpfw_icc_base_send);

    common_icc_base_recv_send(cmd_code, &params, payload, sizeof(payload), test_recv_complete, &cb_context);

    assert_int_equal(g_icc_mock.recv_call_count, 1);
    assert_ptr_equal(g_icc_mock.last_recv_req, &params.icc_recv_req_params);
    assert_ptr_equal(params.icc_recv_req_params.payload_buffer, payload);
    assert_int_equal(params.icc_recv_req_params.buffer_size, sizeof(payload));
    assert_int_equal(params.icc_recv_req_params.recv_cmd_code, cmd_code);
    assert_ptr_equal(params.icc_recv_req_params.cb, test_recv_complete);
    assert_ptr_equal(params.icc_recv_req_params.cb_ctx, &cb_context);

    auto* header = reinterpret_cast<icc_mhu_header_t*>(payload);
    assert_int_equal(header->msg_header.command, cmd_code);
    assert_int_equal(header->msg_header.payload_size, sizeof(payload) - sizeof(icc_mhu_header_t));

    assert_int_equal(g_icc_mock.send_call_count, 1);
    assert_ptr_equal(g_icc_mock.last_send_req, &params.icc_send_req_params);
    assert_ptr_equal(params.icc_send_req_params.payload_buffer, payload);
    assert_int_equal(params.icc_send_req_params.buffer_size, sizeof(payload));
    assert_ptr_equal(params.icc_send_req_params.cb, common_send_complete_notify);
    assert_ptr_equal(params.icc_send_req_params.cb_ctx, &params.icc_send_req_params);
}

TEST_FUNCTION(test_pldm_perf_state_query_dispatches_when_not_busy, setup, nullptr)
{
    power_throttling_request_t request = {};
    pldm_state_sensor_context_t sensor = {};
    sensor.sensor_state.config.sensor_id = PLDM_SENSOR_ID_POWER_THROTTLING_STATE_SENSOR;

    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);
    expect_function_call(__wrap_fpfw_icc_base_recv);
    expect_function_call(__wrap_fpfw_icc_base_send);

    pldm_perf_state_query(&sensor, &request);

    assert_true(request.params.is_active);
    assert_int_equal(g_lock_mock.acquire_calls, 1);
    assert_int_equal(g_lock_mock.release_calls, 1);
    assert_int_equal(g_icc_mock.recv_call_count, 1);
    assert_int_equal(g_icc_mock.send_call_count, 1);
}

TEST_FUNCTION(test_pldm_perf_state_query_skips_when_active, setup, nullptr)
{
    power_throttling_request_t request = {};
    request.params.is_active = true;
    pldm_state_sensor_context_t sensor = {};

    pldm_perf_state_query(&sensor, &request);

    assert_int_equal(g_lock_mock.acquire_calls, 0);
    assert_int_equal(g_lock_mock.release_calls, 0);
    assert_int_equal(g_icc_mock.recv_call_count, 0);
    assert_int_equal(g_icc_mock.send_call_count, 0);
}