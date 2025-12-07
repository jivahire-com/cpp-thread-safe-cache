//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include <FpFwLock.h>
#include <fpfw_icc_base.h>
#include <fpfw_pldm_service.h>
#include <pldm_common_power.h>
#include <pldm_pdr.h>

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

void common_send_complete_notify(void* context, fpfw_status_t status);
void common_icc_base_recv_send(uint32_t cmd_code,
                               common_request_ctx_t* params,
                               void* payload,
                               size_t payload_size,
                               icc_base_recv_complete_notify cb,
                               void* cb_ctx);
void pldm_perf_state_query(pldm_state_sensor_context_t* p_sensor, void* p_context);

#ifdef __cplusplus
}
#endif
