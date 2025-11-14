//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_pldm_scp.c
 * This file contains the implementation of the Power PLDM SCP service.
 */

/*------------- Includes -----------------*/
#include "power_pldm_scp.h"

#include "power_i.h"
#include "power_runconfig.h"
#include "power_runconfig_i.h"

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <fpfw_icc_base.h>
#include <icc_mhu.h>
#include <idsw_kng.h>
#include <inttypes.h>
#include <kng_icc_shared.h>

/*-- Symbolic Constant Macros (defines) --*/
#define UINT16_TO_STR(val)                                   \
    ({                                                       \
        static char buf[8];                                  \
        snprintf(buf, sizeof(buf), "%u W", (uint16_t)(val)); \
        buf;                                                 \
    })
/*-------------- Typedefs ----------------*/
typedef enum
{
    PWR_PLDM_CAP_REQUEST,
    PWR_PLDM_THROT_STATE_REQUEST,
    PWR_PLDM_MAX_REQUEST
} power_pldm_request_type_t;

typedef struct _pwr_pldm_icc_req_ctx
{
    fpfw_icc_base_recv_req_t recv_param;
    fpfw_icc_base_send_rsp_t send_param;
} pwr_pldm_icc_req_ctx_t;

/*-- Declarations (Statics and globals) --*/
static icc_pwr_cap_request_t pwr_cap_payload = {
    .header = {.msg_header = {.command = ICC_COMMAND_PLDM_PWR_CAP,
                              .payload_size = sizeof(icc_pwr_cap_request_t) - sizeof(icc_mhu_header_t)}}};
static icc_pwr_throt_state_request_t pwr_throt_payload = {
    .header = {.msg_header = {.command = ICC_COMMAND_PLDM_PERF_STATE,
                              .payload_size = sizeof(icc_pwr_throt_state_request_t) - sizeof(icc_mhu_header_t)}}};

static fpfw_icc_base_ctx_t* p_icc_base_ctx = NULL;
static uint16_t pwr_cap_upper_limit = 0;

static void pwr_pldm_icc_send_cb(void* context, fpfw_status_t status);
static void pwr_pldm_icc_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status);

static pwr_pldm_icc_req_ctx_t req_ctx[PWR_PLDM_MAX_REQUEST] = {
    {
        .recv_param =
            {
                .payload_buffer = &pwr_cap_payload,
                .buffer_size = sizeof(pwr_cap_payload),
                .recv_cmd_code = ICC_COMMAND_PLDM_PWR_CAP,
                .cb = pwr_pldm_icc_recv_cb,
                .cb_ctx = &req_ctx[PWR_PLDM_CAP_REQUEST].recv_param,
            },
        .send_param =
            {
                .recv_payload_buffer = &pwr_cap_payload,
                .recv_buffer_size = sizeof(pwr_cap_payload),
                .send_payload_buffer = &pwr_cap_payload,
                .send_buffer_size = sizeof(pwr_cap_payload),
                .cb = pwr_pldm_icc_send_cb,
                .cb_ctx = &req_ctx[PWR_PLDM_CAP_REQUEST].send_param,
            },
    },
    {
        .recv_param =
            {
                .payload_buffer = &pwr_throt_payload,
                .buffer_size = sizeof(pwr_throt_payload),
                .recv_cmd_code = ICC_COMMAND_PLDM_PERF_STATE,
                .cb = pwr_pldm_icc_recv_cb,
                .cb_ctx = &req_ctx[PWR_PLDM_THROT_STATE_REQUEST].recv_param,
            },
        .send_param =
            {
                .recv_payload_buffer = &pwr_throt_payload,
                .recv_buffer_size = sizeof(pwr_throt_payload),
                .send_payload_buffer = &pwr_throt_payload,
                .send_buffer_size = sizeof(pwr_throt_payload),
                .cb = pwr_pldm_icc_send_cb,
                .cb_ctx = &req_ctx[PWR_PLDM_THROT_STATE_REQUEST].send_param,
            },
    },
};

/*--------- Function Prototypes ----------*/
static void power_cap_completed_callback(int result, uint16_t current_cap, uint16_t previous_cap_watts)
{
    uint16_t resolved_prev_cap = (previous_cap_watts == UINT16_MAX) ? pwr_cap_upper_limit : previous_cap_watts;

    POWER_LOG_INFO("[PLDM POWER CAP UPDATE] Result %d Current %s Previous %s\n",
                   result,
                   (current_cap == NO_POWER_CAP) ? "NO CAPPING" : UINT16_TO_STR(current_cap),
                   (resolved_prev_cap == NO_POWER_CAP) ? "NO CAPPING" : UINT16_TO_STR(resolved_prev_cap));
    //! At this point the power cap has been finalized, send mcp the response for power cap complete
    //! fill in the response
    pwr_cap_payload.soc_output.status = (MP_POWER_CAP_SUCCESS == result) ? 0 : 1; //! 0 ->success, 1 -> failure
    pwr_cap_payload.soc_output.current_power_cap = current_cap;
    pwr_cap_payload.soc_output.previous_power_cap =
        (previous_cap_watts == UINT16_MAX) ? pwr_cap_upper_limit : previous_cap_watts;
    //! Send response to mcp
    fpfw_status_t status = fpfw_icc_base_send_resp(p_icc_base_ctx, &req_ctx[PWR_PLDM_CAP_REQUEST].send_param);
    FPFW_RUNTIME_ASSERT_EXT(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0, 0, 0);
}

uint16_t handle_power_cap_request(icc_pwr_cap_request_t* p_cap_request)
{
    uint16_t requested_pwr_cap = p_cap_request->oob_input.power_cap;
    uint16_t current_power_cap = get_current_soc_power_cap();

    // Log the received power cap request using the UINT16_TO_STR macro for formatting
    POWER_LOG_INFO("[PWR PLDM] Received Power Cap Request: Requested %s, Current %s\n",
                   (requested_pwr_cap == NO_POWER_CAP) ? "NO_POWER_CAP" : UINT16_TO_STR(requested_pwr_cap),
                   (current_power_cap == NO_POWER_CAP) ? "NO_POWER_CAP" : UINT16_TO_STR(current_power_cap));

    //! Print the contents of p_cap_request
    POWER_LOG_TRACE("[PWR PLDM] p_cap_request contents:\n");
    POWER_LOG_TRACE("  header.msg_header.command: %lu\n", p_cap_request->header.msg_header.command);
    POWER_LOG_TRACE("  header.msg_header.status: %u\n", p_cap_request->header.msg_header.status);
    POWER_LOG_TRACE("  soc_output.current_power_cap: %u\n", p_cap_request->soc_output.current_power_cap);
    POWER_LOG_TRACE("  soc_output.previous_power_cap: %u\n", p_cap_request->soc_output.previous_power_cap);

    //! 1. Verify if the power cap is within limits, Correct if not, NO_POWER_CAP = 65535 is a special case to turn off power capping.
    if ((requested_pwr_cap > pwr_cap_upper_limit) && (requested_pwr_cap != NO_POWER_CAP))
    {
        POWER_LOG_ERR("[PWR PLDM] Power cap out of range, Expected <= %d W, Received: %d W!", pwr_cap_upper_limit, requested_pwr_cap);
        requested_pwr_cap = FPFW_MIN(pwr_cap_upper_limit, requested_pwr_cap);
    }

    //! 2. Verify if the current power cap value of die 0 differs from BMC requested value
    if (current_power_cap != requested_pwr_cap)
    {
        //! 2a. If Current power cap != Requested power cap, Prepare to update current power cap value of die 0, send resp in cb
        int result = power_cap_update(power_cap_completed_callback, requested_pwr_cap, false);
        if (result != MP_POWER_CAP_PENDING)
        {
            // if the power cap update failed, we need to set the power cap back to the previous value
            POWER_LOG_ERR("[PWR PLDM] Power cap update failed, status %d\n", result);
        }
        else
        {
            //! response will be sent to bmc post power cap finalize
            POWER_LOG_INFO("[PWR PLDM] Power cap update pending, new cap %s\n",
                           (requested_pwr_cap == NO_POWER_CAP) ? "NO_POWER_CAP" : UINT16_TO_STR(requested_pwr_cap));
        }
    }
    else
    {
        //! 2b. else Nothing to update, Send response back to BMC
        POWER_LOG_INFO("[PWR PLDM] Power cap update not required, current cap = requested cap\n");
        power_cap_completed_callback(MP_POWER_CAP_SUCCESS, requested_pwr_cap, current_power_cap);
    }
    return requested_pwr_cap;
}

pldm_performance_states_t handle_performance_state_request(icc_pwr_throt_state_request_t* p_throt_request)
{
    pldm_performance_states_t state = PLDM_PERFORMANCE_NORMAL; //! Initialize to normal
    //! Update the response payload
    p_throt_request->soc_output.status = 0; //! unused ?
    p_throt_request->soc_output.throttled = power_control_loop_throttled();
    p_throt_request->soc_output.degraded = power_control_loop_degraded();
    POWER_LOG_INFO("[PWR PLDM] Received Performance State Request: Throttled %d, Degraded %d\n",
                   p_throt_request->soc_output.throttled,
                   p_throt_request->soc_output.degraded);

    //! Print the contents of p_throt_request
    POWER_LOG_TRACE("[PWR PLDM] p_throt_request contents:\n");
    POWER_LOG_TRACE("  header.msg_header.command: %lu\n", p_throt_request->header.msg_header.command);
    POWER_LOG_TRACE("  header.msg_header.status: %u\n", p_throt_request->header.msg_header.status);
    POWER_LOG_TRACE("  soc_output.status: %lu\n", p_throt_request->soc_output.status);
    POWER_LOG_TRACE("  soc_output.throttled: %u\n", p_throt_request->soc_output.throttled);
    POWER_LOG_TRACE("  soc_output.degraded: %u\n", p_throt_request->soc_output.degraded);

    //! Send response to mcp
    fpfw_status_t status = fpfw_icc_base_send_resp(p_icc_base_ctx, &req_ctx[PWR_PLDM_THROT_STATE_REQUEST].send_param);
    FPFW_RUNTIME_ASSERT_EXT(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0, 0, 0);

    //! Determine and return the performance state
    if (p_throt_request->soc_output.throttled)
    {
        state = p_throt_request->soc_output.degraded ? PLDM_PERFORMANCE_THROTTLED_DEGRADED : PLDM_PERFORMANCE_THROTTLED;
    }
    else if (p_throt_request->soc_output.degraded)
    {
        state = PLDM_PERFORMANCE_DEGRADED;
    }
    return state;
}

static void pwr_pldm_icc_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
    FPFW_RUNTIME_ASSERT(context != NULL);

    //! Get the payload from the request, payload now contains the response from MCP
    fpfw_icc_base_recv_req_t* p_request = (fpfw_icc_base_recv_req_t*)context;
    icc_mhu_header_t* recv_payload = (icc_mhu_header_t*)p_request->payload_buffer;

    //! Process the received payload
    switch (recv_payload->msg_header.command)
    {
    case ICC_COMMAND_PLDM_PWR_CAP: {
        icc_pwr_cap_request_t* p_cap_request = (icc_pwr_cap_request_t*)recv_payload;
        handle_power_cap_request(p_cap_request);
    }
    break;
    case ICC_COMMAND_PLDM_PERF_STATE: {
        icc_pwr_throt_state_request_t* p_throt_request = (icc_pwr_throt_state_request_t*)recv_payload;
        handle_performance_state_request(p_throt_request);
    }
    break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

static void pwr_pldm_icc_send_cb(void* context, fpfw_status_t status)
{
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
    FPFW_RUNTIME_ASSERT(context != NULL);
    fpfw_status_t icc_status = FPFW_ICC_BASE_STATUS_SUCCESS;

    //! Cb for send response completion, resubscribe to recv next requests now
    fpfw_icc_base_send_rsp_t* p_request = (fpfw_icc_base_send_rsp_t*)context;
    icc_mhu_header_t* send_payload = (icc_mhu_header_t*)p_request->send_payload_buffer;

    switch (send_payload->msg_header.command)
    {
    case ICC_COMMAND_PLDM_PWR_CAP:
        POWER_LOG_INFO("[PWR PLDM] Power Cap Request Response Send Completion\n");
        icc_status = fpfw_icc_base_recv(p_icc_base_ctx, &req_ctx[PWR_PLDM_CAP_REQUEST].recv_param);
        FPFW_RUNTIME_ASSERT_EXT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_status, 0, 0, 0);
        break;
    case ICC_COMMAND_PLDM_PERF_STATE:
        POWER_LOG_TRACE("[PWR PLDM] Power Perf State Request Response Send Completion\n");
        icc_status = fpfw_icc_base_recv(p_icc_base_ctx, &req_ctx[PWR_PLDM_THROT_STATE_REQUEST].recv_param);
        FPFW_RUNTIME_ASSERT_EXT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_status, 0, 0, 0);
        break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

void power_pldm_service_init(power_runconfig_t* p_runconfig)
{
    if (idsw_get_die_id() == DIE_1)
    {
        //! Only relevant for Die 0, skip for die 1
        return;
    }
    //! subscribe to icc messages from MCP for power cap effectors & power state sensors
    fpfw_status_t icc_status = FPFW_ICC_BASE_STATUS_SUCCESS;
    p_icc_base_ctx = (fpfw_icc_base_ctx_t*)(p_runconfig->p_sconfig->icc_mscp_ctx);
    pwr_cap_upper_limit = p_runconfig->derived.soc_maximum_thermal_watts_limit;

    //! Register to recv pldm messages from mcp
    for (power_pldm_request_type_t i = 0; i < PWR_PLDM_MAX_REQUEST; i++)
    {
        icc_status = fpfw_icc_base_recv(p_icc_base_ctx, &req_ctx[i].recv_param);
        FPFW_RUNTIME_ASSERT_EXT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_status, 0, 0, 0);
        POWER_LOG_INFO("[PWR PLDM] Registered to receive PLDM messages, Cmd: %" PRIx32 "\n",
                       req_ctx[i].recv_param.recv_cmd_code);
    }
    POWER_LOG_INFO("[PWR PLDM] Power PLDM service initialized successfully\n");
}