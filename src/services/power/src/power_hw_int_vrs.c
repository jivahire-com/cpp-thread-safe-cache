//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_vrs.c
 * Implements the voltage rail interface of the power service
 */

/*------------- Includes -----------------*/
#include "power_events.h"
#include "power_hw_int_i.h"
#include "power_i.h"
#include "power_loops_i.h"
#include "power_stub_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <debug.h>
#include <idsw.h> // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <inttypes.h>
#include <sensor_fifo_service.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

#define PWR_MW 1000

#define MAX_VR_PER_DIE 8

static_assert(MAX_VR_PER_DIE == MAX_NUM_OF_VR_RAILS, "MAX_VR_PER_DIE != MAX_NUM_OF_VR_RAILS");

#define DO_ONLY_ONCE(...)                    \
    do                                       \
    {                                        \
        static bool has_not_occurred = true; \
        if (has_not_occurred)                \
        {                                    \
            __VA_ARGS__;                     \
            has_not_occurred = false;        \
        }                                    \
    } while (0)

/*------------- Typedefs -----------------*/
// context structure for VRs
typedef struct _power_vrs_context
{
    // latest current/temp/voltage here
    power_vrs_avs_latest_t vr_inputs[MAX_VR_PER_DIE];
    power_latest_calcs_t latest_power;

    // store recent power calculations
    float soc_power[SOC_POWER_AVG_COUNT]; /* most recent x soc power measurements,
                                             most recent in 0 index */
    float vcpu_power_log[SOC_POWER_AVG_COUNT];
    float vcpu_current_log[SOC_POWER_AVG_COUNT];
} power_vrs_context_t;

/*-------- Function Prototypes -----------*/
static void calculate_soc_power();

/*-- Declarations (Statics and globals) --*/
static power_vrs_context_t s_power_vrs_ctx = {0};
avs_pwr_request_context_t pwr_avs_request[MAX_AVS_INST] = {0};
static pscp_avs_interface_t pwr_avs_interfaces[MAX_AVS_INST] = {0};

/*------------- Functions ----------------*/
void AVSPwrWriteRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(Request);
    int pwr_avs_bus = (int)CompletionContext;

    pwr_avs_request[pwr_avs_bus].in_use = false;

    if (pwr_avs_request[pwr_avs_bus].request.avs_response_status != SCP_AVS_STATUS_SUCCESS)
    {
        POWER_LOG_ERR("\n AVS PWR status error = %x\n", pwr_avs_request[pwr_avs_bus].request.avs_response_status);
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL, NULL);
        return;
    }

    switch (pwr_avs_request[pwr_avs_bus].request.Header.RequestType)
    {
    case AVS_REQUEST_WRITE_DATA:
        if (pwr_avs_request[pwr_avs_bus].request.avs_response_single_resp.error.v_done == 1)
        {
            power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
            POWER_LOG_TRACE("\n AVS PWR Write complete\n");
        }
        else
        {
            power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING, NULL);
        }
        break;

    default:
        // unexpected to receive any other event in this state,
        POWER_LOG_ERR("\n AVSPwrWriteRequestCompletion, unexpected RequestType\n");
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL, NULL);
        break;
    }
}

void AVSPwrReadRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(Request);
    int pwr_avs_bus = (int)CompletionContext;

    pwr_avs_request[pwr_avs_bus].in_use = false;

    if (pwr_avs_request[pwr_avs_bus].request.avs_response_status != SCP_AVS_STATUS_SUCCESS)
    {
        POWER_LOG_ERR("\n AVS PWR status error = %x\n", pwr_avs_request[pwr_avs_bus].request.avs_response_status);
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL, NULL);
        return;
    }

    switch (pwr_avs_request[pwr_avs_bus].request.Header.RequestType)
    {
    case AVS_REQUEST_READ_DATA:
        if (pwr_avs_request[pwr_avs_bus].request.avs_response_single_resp.error.v_done == 1)
        {
            POWER_LOG_TRACE("\n AVS voltage read.\n AVSBus = %d\n rail = %d\n AVS volt. = %d.%03d\n",
                            pwr_avs_bus,
                            pwr_avs_request[pwr_avs_bus].request.avs_params.avs_cmd_info.rail_id,
                            ((pwr_avs_request[pwr_avs_bus].request.avs_response_single_resp.data) / 1000),
                            ((pwr_avs_request[pwr_avs_bus].request.avs_response_single_resp.data) % 1000));
            power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
        }
        else
        {
            power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING, NULL);
        }
        break;
    // TODO: Add AVS_REQUEST_READ_MULTI case https://azurecsi.visualstudio.com/Woodinville/_workitems/edit/2137515/?triage=true
    default:
        // unexpected to receive any other event in this state,
        POWER_LOG_ERR("\n AVSPwrReadRequestCompletion, unexpected RequestType\n");
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL, NULL);
        break;
    }
}

// TODO: Below is temp implementation ahead of AVS integration (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491023/)
void power_vrs_initiate_vr_reads()
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    uint8_t vr_vcpu_idx;
    vr_vcpu_idx = (p_config->vr_idx_info.vcpu_idx - p_config->vr_idx_info.flattened_vr_start_idx);
    BUG_ASSERT(vr_vcpu_idx <= MAX_VR_PER_DIE);

    // temp, somewhat valid-looking data
    s_power_vrs_ctx.vr_inputs[vr_vcpu_idx].voltage = 1000;
    s_power_vrs_ctx.vr_inputs[vr_vcpu_idx].current = 20000;

    calculate_soc_power();

    // store for control loop
    s_power_vrs_ctx.latest_power.soc_power = s_power_vrs_ctx.soc_power[0]; // most recent soc power measurement
    s_power_vrs_ctx.latest_power.vcpu_power = s_power_vrs_ctx.vcpu_power_log[0]; // most recent vcpu power measurement
    s_power_vrs_ctx.latest_power.vcpu_avs_voltage = s_power_vrs_ctx.vr_inputs[vr_vcpu_idx].voltage;
    s_power_vrs_ctx.latest_power.vcpu_avs_current = s_power_vrs_ctx.vr_inputs[vr_vcpu_idx].current;

    power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VR_READ, &s_power_vrs_ctx.latest_power);
    power_loops_vr_telem_handle_event(POWER_VR_TELEM_SIGNAL_VR_CURRENT, s_power_vrs_ctx.vr_inputs);
}

void power_vrs_write_vcpu_voltage(uint16_t voltage_mv)
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    // any boot VR request changes should have completed

    // TODO: update cpu_idx, since forced index will be difference on die1
    // (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/) skip updating VCPU if forced VCPU was set

    uint8_t vcpu_index;
    vcpu_index = p_config->vr_idx_info.vcpu_idx;

    assert(vcpu_index < DIMOF(p_runconfig->knobs.forced_vrs.vr));
    if (p_runconfig->knobs.forced_vrs.vr[vcpu_index] != 0)
    {
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
        return;
    }

    // want to warn/adjust if calculated voltage is over a maximum, under minimum
    if (voltage_mv > VR_VCPU_MAX_VOLTAGE_MV)
    {
        DO_ONLY_ONCE(POWER_ET_ERROR(POWER_ET_TYPE_VCPU_CALCULATED_OVER_MAX, voltage_mv));
        DO_ONLY_ONCE(POWER_LOG_WARN(" vcpu %dmV > %dmV", voltage_mv, VR_VCPU_MAX_VOLTAGE_MV));
        voltage_mv = VR_VCPU_MAX_VOLTAGE_MV;
    }
    else if (voltage_mv < VR_VCPU_MIN_VOLTAGE_MV)
    {
        DO_ONLY_ONCE(POWER_ET_ERROR(POWER_ET_TYPE_VCPU_CALCULATED_UNDER_MIN, voltage_mv));
        DO_ONLY_ONCE(POWER_LOG_WARN(" vcpu %dmV < %dmV", voltage_mv, VR_VCPU_MIN_VOLTAGE_MV));
        voltage_mv = VR_VCPU_MIN_VOLTAGE_MV;
    }

    if (!all_requests_completed(pwr_avs_request, p_config->avs_details[vcpu_index].bus_id))
    {
        // can't send another request until the previous one is complete, just signal failure and wait for retry
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL, NULL);
        return;
    }

    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.avs_response_single_resp.error.as_uint8 = AVS_ERROR_NONE;
    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].in_use = true;
    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.avs_params.avs_cmd_info.rail_id =
        p_config->avs_details[vcpu_index].rail_id;
    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.avs_params.avs_cmd_info.cmd_type = AVS_VOLTAGE_RW;
    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.avs_params.avs_data = (uint32_t)voltage_mv;

    // Temporary work around here to prevent SVP 12 from crashing: Task 2194174: Unblock IFWI by working around SVP 12 AVS bug
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        scp_avs_client_write(&pwr_avs_interfaces[p_config->avs_details[vcpu_index].bus_id]->Header,
                             &pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.Header,
                             AVSPwrWriteRequestCompletion,
                             (void*)(uintptr_t)p_config->avs_details[vcpu_index].bus_id);
    }
    else
    {
        pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].in_use = false;
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
    }

    POWER_LOG_TRACE(" PWR AVS write compl. \n");
}

void power_vrs_read_vcpu_voltage()
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    uint8_t vcpu_index;
    vcpu_index = p_config->vr_idx_info.vcpu_idx;

    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.avs_response_single_resp.error.as_uint8 = AVS_ERROR_NONE;
    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].in_use = true;
    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.avs_params.avs_cmd_info.rail_id =
        p_config->avs_details->rail_id;
    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.avs_params.avs_cmd_info.cmd_type = AVS_VOLTAGE_RW;

    // Temporary work around here to prevent SVP 12 from crashing: Task 2194174: Unblock IFWI by working around SVP 12 AVS bug
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        scp_avs_client_read(&pwr_avs_interfaces[p_config->avs_details[vcpu_index].bus_id]->Header,
                            &pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.Header,
                            AVSPwrReadRequestCompletion,
                            (void*)(uintptr_t)p_config->avs_details->bus_id);
    }
    else
    {
        pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].in_use = false;
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
    }

    POWER_LOG_TRACE(" PWR AVS read compl. \n");
}

static void calculate_soc_power()
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    // initialize with static rail power
    // TODO: Only include static rail on die 0 (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/)
    float soc_power = p_runconfig->knobs.static_rail_power_watts;
    float vcpu_power = 0;
    // TODO: pick right loadline knob for die (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/)
    float r_loadline_ohms = 0.000001F * p_runconfig->knobs.r_loadline_vcpu0_uohm; // loadline knobs in uOhms
    float vsys_r_loadline_ohms = 0.000001F * p_runconfig->knobs.vsys_r_loadline_uohm;
    float rail_vcpu_current = 0;

    // sum the power of all the VRs; save off VCPU separately
    for (int vr_idx = 0; vr_idx < p_config->num_vr; ++vr_idx)
    {
        const float rail_current = AVS_CURRENT_FLOAT(s_power_vrs_ctx.vr_inputs[vr_idx].current);
        float rail_power = rail_current * AVS_VOLTAGE_FLOAT(s_power_vrs_ctx.vr_inputs[vr_idx].voltage);

        // Adjust for loadline need rail/die info
        if ((vr_idx + p_config->vr_idx_info.flattened_vr_start_idx) == p_config->vr_idx_info.vsys_idx)
        { // VSYS = Die0, AVSBus1 rail 0, so index 2.
            rail_power -= (rail_current * rail_current * vsys_r_loadline_ohms); // subtract i^2 * r for
        }
        else if ((vr_idx + p_config->vr_idx_info.flattened_vr_start_idx) == p_config->vr_idx_info.vcpu_idx)
        {                                                                  // VCPU
            rail_power -= (rail_current * rail_current * r_loadline_ohms); // subtract i^2 * r for loadline
            rail_vcpu_current = rail_current;
            vcpu_power = rail_power;
        }
        soc_power += rail_power;
    }
    // move 0-(x-1) to 1-x  -- leaving index 0 open for new measurement
    memmove(&s_power_vrs_ctx.soc_power[1],
            &s_power_vrs_ctx.soc_power[0],
            sizeof(s_power_vrs_ctx.soc_power[0]) * (SOC_POWER_AVG_COUNT - 1));
    // save most recent measurement to index 0
    s_power_vrs_ctx.soc_power[0] = soc_power;

    // move 0-(x-1) to 1-x  -- leaving index 0 open for new measurement
    memmove(&s_power_vrs_ctx.vcpu_power_log[1],
            &s_power_vrs_ctx.vcpu_power_log[0],
            sizeof(s_power_vrs_ctx.vcpu_power_log[0]) * (SOC_POWER_AVG_COUNT - 1));
    // save most recent measurement to index 0
    s_power_vrs_ctx.vcpu_power_log[0] = vcpu_power;

    memmove(&s_power_vrs_ctx.vcpu_current_log[1],
            &s_power_vrs_ctx.vcpu_current_log[0],
            sizeof(s_power_vrs_ctx.vcpu_current_log[0]) * (SOC_POWER_AVG_COUNT - 1));
    // save most recent measurement to index 0
    s_power_vrs_ctx.vcpu_current_log[0] = rail_vcpu_current;
}

uint32_t power_vrs_get_recent_power_mw()
{
    // TODO: updates for remote die (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/)
    // likely want an API that can be used to pass in the remote power to also log and include in this calculation
    float total = 0.0F;
    for (int meas_idx = 0; meas_idx < SOC_POWER_AVG_COUNT; meas_idx++)
    {
        total += s_power_vrs_ctx.soc_power[meas_idx];
    }
    return (uint32_t)((total / SOC_POWER_AVG_COUNT) * PWR_MW);
}

void pwr_avs_initialize(pscp_avs_interface_t avs_array[])
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    // Die 0 has 8 VRs, Die 1 has 2 VRs, so 4 AVSBus for Die 0 and 1 AVSBus for Die 1
    for (uint8_t i = 0; i < (p_config->num_vr / MAX_AVS_RAILS); i++)
    {
        pwr_avs_interfaces[i] = avs_array[i];
        DfwkAsyncRequestInitialize((PDFWK_ASYNC_REQUEST_HEADER)&pwr_avs_request[i].request.Header,
                                   sizeof(pwr_avs_request[i].request));
        pwr_avs_request[i].in_use = false;
    }
}
