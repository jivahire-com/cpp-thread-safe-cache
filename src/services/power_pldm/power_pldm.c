//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_pldm.c
 * Power pldm request processing
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwAssert.h>
#include <fpfw_icc_base.h>
#include <fpfw_pldm_service.h>
#include <icc_mhu.h>
#include <idsw_kng.h>
#include <kng_icc_shared.h>
#include <pldm_common_power.h>
#include <pldm_pdr.h>
#include <power_pldm.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
//! Structure grouping common metrics for power requests
typedef struct
{
    fpfw_icc_base_send_req_t icc_send_req_params;
    fpfw_icc_base_recv_req_t icc_recv_req_params;
    FPFW_LOCK lock;
    bool is_active;
} common_request_ctx_t;

//! Structure for power throttling requests
typedef struct
{
    pldm_state_sensor_context_t sensor_ctx;
    icc_pwr_throt_state_request_t icc_req_payload;
    common_request_ctx_t params;
} power_throttling_request_t;

//! Structure for power cap requests
typedef struct
{
    pldm_numeric_effecter_context_t effecter_ctx;
    icc_pwr_cap_request_t icc_req_payload;
    common_request_ctx_t params;
} power_cap_request_t;

//! Top level context for the power PLDM service
typedef struct
{
    power_throttling_request_t pwr_throt;
    power_cap_request_t pwr_cap;
    fpfw_icc_base_ctx_t* icc_ctx;
} power_pldm_context_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static power_pldm_context_t pldm_ctx = {0};

/*-------- Common Static Function-----------*/
void common_send_complete_notify(void* context, fpfw_status_t status)
{
    FPFW_RUNTIME_ASSERT(context != NULL);
    FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
    fpfw_icc_base_send_req_t* req_params = (fpfw_icc_base_send_req_t*)context; // NOLINT
    icc_mhu_header_t* mhu_payload = (icc_mhu_header_t*)req_params->payload_buffer;
    FPFW_DBGPRINT_VERBOSE("[PWR PLDM] Send Complete: Cmd[0x%x]\n", mhu_payload->msg_header.command);
}

static void common_icc_base_recv_send(uint32_t cmd_code,
                                      common_request_ctx_t* params,
                                      void* payload,
                                      size_t payload_size,
                                      icc_base_recv_complete_notify cb,
                                      void* cb_ctx)
{
    FPFW_RUNTIME_ASSERT(params != NULL);
    FPFW_RUNTIME_ASSERT(payload != NULL);

    //! 1. Subscribe to the response 1st
    params->icc_recv_req_params.payload_buffer = payload;
    params->icc_recv_req_params.buffer_size = payload_size;
    params->icc_recv_req_params.recv_cmd_code = cmd_code;
    params->icc_recv_req_params.cb = cb;
    params->icc_recv_req_params.cb_ctx = cb_ctx;
    fpfw_status_t status = fpfw_icc_base_recv(pldm_ctx.icc_ctx, &params->icc_recv_req_params);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

    //! 2. Prepare to send, Populate mhu payload packet
    icc_mhu_header_t* mhu_payload = (icc_mhu_header_t*)payload;
    mhu_payload->msg_header.command = cmd_code;
    mhu_payload->msg_header.payload_size = payload_size - sizeof(icc_mhu_header_t);

    //! 3. Create an icc base send_recv request
    params->icc_send_req_params.payload_buffer = payload;
    params->icc_send_req_params.buffer_size = payload_size;
    params->icc_send_req_params.cb = common_send_complete_notify;
    params->icc_send_req_params.cb_ctx = &params->icc_send_req_params;

    //! 4. Send the payload/request
    status = fpfw_icc_base_send(pldm_ctx.icc_ctx, &params->icc_send_req_params);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
}

/*-------- ICC Callbacks -----------*/
static void pldm_set_cap_callback(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
    FPFW_RUNTIME_ASSERT(context != NULL);

    //! Get the payload from the request, payload now contains the response from SCP
    power_cap_request_t* p_request = (power_cap_request_t*)context;
    icc_pwr_cap_request_t recv_payload = p_request->icc_req_payload;
    FPFW_DBGPRINT_ALWAYS("[PWR PLDM] pldm_set_cap_callback: current - %dW,  previous %dW,  result - %d (%s)",
                         recv_payload.soc_output.current_power_cap,
                         recv_payload.soc_output.previous_power_cap,
                         recv_payload.soc_output.status,
                         (recv_payload.soc_output.status == 0 ? "success" : "fail"));

    //! Notify the PLDM service that the effecter set operation is complete
    fpfw_status_t pldm_status = fpfw_pldm_service_numeric_effecter_set_complete(&p_request->effecter_ctx);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(pldm_status), pldm_status, 0, 0, 0);

    //! Reset the request flag, dfwk request completion cb have reentrancy protection
    p_request->params.is_active = false;
}

static void pldm_get_perf_state(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
    FPFW_RUNTIME_ASSERT(context != NULL);

    static uint8_t prev_state = PLDM_PERFORMANCE_NORMAL; //! keep track of prev state
    //! Get the payload from the request, payload now contains the response from SCP
    power_throttling_request_t* p_request = (power_throttling_request_t*)context;
    icc_pwr_throt_state_request_t recv_payload = p_request->icc_req_payload;
    FPFW_DBGPRINT_VERBOSE("[PWR PLDM] pldm_get_perf_state: status - %d,  State:[%s, %s]",
                          recv_payload.soc_output.status,
                          recv_payload.soc_output.throttled == 0 ? "no throttle" : "throttling",
                          recv_payload.soc_output.degraded == 0 ? "no degradation" : "degradation");

    //! Update the power throttling state sensor with the received values
    fpfw_pldm_composite_value_t composite_value = {0};
    uint8_t state = PLDM_PERFORMANCE_NORMAL;
    if (recv_payload.soc_output.degraded)
    {
        state = PLDM_PERFORMANCE_DEGRADED;
    }
    else if (recv_payload.soc_output.throttled)
    {
        state = PLDM_PERFORMANCE_THROTTLED;
    }
    composite_value.state.states[PLDM_PERFORMANCE_COMP_STATE] = state;

    //! Notify the PLDM service of the state change
    fpfw_status_t pldm_status =
        fpfw_pldm_service_state_sensor_set(&p_request->sensor_ctx, composite_value, 1 << PLDM_PERFORMANCE_COMP_STATE);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(pldm_status), pldm_status, 0, 0, 0);

    //! Reset the request flag, dfwk request completion cb have reentrancy protection
    p_request->params.is_active = false;

    //! Set the previous state, always print if state changed
    if (prev_state != state)
    {
        FPFW_DBGPRINT_ALWAYS("[PWR PLDM] Power Throttle State Changed: Prev[%s] Curr[%s]",
                             prev_state == PLDM_PERFORMANCE_NORMAL     ? "NORMAL"
                             : prev_state == PLDM_PERFORMANCE_DEGRADED ? "DEGRADED"
                                                                       : "THROTTLED",
                             state == PLDM_PERFORMANCE_NORMAL     ? "NORMAL"
                             : state == PLDM_PERFORMANCE_DEGRADED ? "DEGRADED"
                                                                  : "THROTTLED");
        prev_state = state;
    }
}

/*-------- PLDM Callbacks -----------*/
static void power_cap_on_effecter_set(pldm_numeric_effecter_context_t* effecter, fpfw_pldm_composite_value_t value, void* p_context)
{
    static uint32_t power_cap_set_count = 0;
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    power_cap_request_t* p_request = (power_cap_request_t*)p_context;
    /**
     * @note: Check if the previous request is still active, highly unlikely scenario
     * BMC will raise pldm requests at 500ms cadence, soc fw should typically fulfill
     * the requests within 20 ms. If this condition is hit, it might indicate a bigger
     * issue. For now we log & drop the request.
     */
    if (p_request->params.is_active)
    {
        FPFW_DBGPRINT_ERROR("[PWR PLDM] Previous Pwr Cap Request is still active, dropping the request!");
        return;
    }
    //! Set the active flag
    FPFW_LOCK_STATE state = FpFwLockAcquire(&p_request->params.lock);
    p_request->params.is_active = true;
    FpFwLockRelease(&p_request->params.lock, state);
    power_cap_set_count++;
    FPFW_DBGPRINT_ALWAYS("[PWR PLDM] Pwr Cap effecter 0x%lx Value: %u Set Requested! Count: %d\n",
                         effecter->effecter_state.config.effecter_id,
                         value.numeric.u16,
                         power_cap_set_count);

    //! Set desired power cap in payload
    p_request->icc_req_payload.oob_input.power_cap = value.numeric.u16;
    //! Prepare to send the power cap to SCP, raise async request to send the power cap
    common_icc_base_recv_send(ICC_COMMAND_PLDM_PWR_CAP,
                              &p_request->params,
                              &p_request->icc_req_payload,
                              sizeof(p_request->icc_req_payload),
                              pldm_set_cap_callback,
                              p_request);
}

static void pldm_perf_state_query(pldm_state_sensor_context_t* p_sensor, void* p_context)
{
    static uint32_t throttle_query_count = 0;
    static bool active_msg_printed = false;
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    power_throttling_request_t* p_request = (power_throttling_request_t*)p_context;
    /**
     * @note: Check if the previous request is still active, highly unlikely scenario
     * BMC will raise pldm requests at 500ms cadence, soc fw should typically fulfill
     * the requests within 20 ms. If this condition is hit, it might indicate a bigger
     * issue. For now we log & drop the request.
     */
    if (p_request->params.is_active)
    {
        if (!active_msg_printed)
        {
            FPFW_DBGPRINT_ERROR(
                "[PWR PLDM] Previous Pwr Throt State Request is still active, dropping the request!");
            active_msg_printed = true;
        }
        return;
    }
    //! Set the active flag, reset print flag
    active_msg_printed = false;
    FPFW_LOCK_STATE state = FpFwLockAcquire(&p_request->params.lock);
    p_request->params.is_active = true;
    FpFwLockRelease(&p_request->params.lock, state);
    //! Keep count & reduce no of prints for query raised to only once every 500 times, current update interval is every 500ms
    if (throttle_query_count % 500 == 0)
    {
        FPFW_DBGPRINT_ALWAYS("[PWR PLDM] Pwr throt state sensor 0x%lx Query Raised! Count: %d\n",
                             p_sensor->sensor_state.config.sensor_id,
                             throttle_query_count);
    }
    throttle_query_count++;
    //! Prepare to send the performance state query to SCP, raise async request to send the performance state
    common_icc_base_recv_send(ICC_COMMAND_PLDM_PERF_STATE,
                              &p_request->params,
                              &p_request->icc_req_payload,
                              sizeof(p_request->icc_req_payload),
                              pldm_get_perf_state,
                              p_request);
}

/*-------- Public Functions -----------*/
void power_pldm_init(fpfw_icc_base_ctx_t* icc_ctx)
{
    FPFW_RUNTIME_ASSERT(icc_ctx != NULL);
    pldm_ctx.icc_ctx = icc_ctx;

    if ((idsw_get_cpu_type() == CPU_MCP) && (idsw_get_die_id() == DIE_0))
    {
        //! Initialize the power capping numeric effecter
        pldm_numeric_effecter_config_t pwr_cap_effecter_cfg = {.effecter_id = PLDM_EFFECTER_ID_POWER_CAP_NUM_EFFECTER,
                                                               .notifications.on_effecter_set = power_cap_on_effecter_set,
                                                               .notifications.context = &pldm_ctx.pwr_cap};

        fpfw_status_t status =
            fpfw_pldm_service_register_numeric_effecter(&pldm_ctx.pwr_cap.effecter_ctx, &pwr_cap_effecter_cfg);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

        //! Initialize the power throttling state sensor
        pldm_state_sensor_config_t pwr_throt_state_cfg = {.sensor_id = PLDM_SENSOR_ID_POWER_THROTTLING_STATE_SENSOR,
                                                          .notifications.on_sensor_get = pldm_perf_state_query,
                                                          .notifications.context = &pldm_ctx.pwr_throt};
        status = fpfw_pldm_service_register_state_sensor(&pldm_ctx.pwr_throt.sensor_ctx, &pwr_throt_state_cfg);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

        //! Initialize the locks
        FpFwLockInitialize(&pldm_ctx.pwr_throt.params.lock);
        FpFwLockInitialize(&pldm_ctx.pwr_cap.params.lock);

        FPFW_DBGPRINT_ALWAYS(
            "[PWR PLDM] Initialized, Pwr Throt State Sensor ID: %d, Pwr Cap Num Effecter ID: %d\n",
            pwr_throt_state_cfg.sensor_id,
            pwr_cap_effecter_cfg.effecter_id);
    }
}