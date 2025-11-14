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
#include "power_runconfig.h"

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

/*-------- Function Prototypes -----------*/
static void calculate_soc_power();

/*-- Declarations (Statics and globals) --*/
static power_vrs_context_t s_power_vrs_ctx = {0};
avs_pwr_request_context_t pwr_avs_request[MAX_AVS_INST] = {0};
static pscp_avs_interface_t pwr_avs_interfaces[MAX_AVS_INST] = {0};

/*------------- Functions ----------------*/
bool power_hw_has_avs_devices()
{
    KNG_PLAT_ID platform_id;
    platform_id = idsw_get_platform_sdv();

    // Return true for PLATFORM_FPGA_LARGE_RVP, PLATFORM_RVP_EVT_SILICON and SVP platforms (AVS devices or simulated)
    if ((platform_id == PLATFORM_FPGA_LARGE_RVP) || (platform_id == PLATFORM_RVP_EVT_SILICON) ||
        (platform_id == PLATFORM_SVP_SIM) || (platform_id == PLATFORM_SVP_MIN_CONFIG_SIM))
    {
        return true;
    }
    // Return false for any other platform
    return false;
}

void AVSPwrWriteRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(Request);
    int pwr_avs_bus = (int)CompletionContext;

    pwr_avs_request[pwr_avs_bus].in_use = false;

    if (!power_hw_has_avs_devices())
    {
        // if no VR's then complete and return
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
        return;
    }

    if (pwr_avs_request[pwr_avs_bus].request.avs_response_status != SCP_AVS_STATUS_SUCCESS)
    {
        POWER_LOG_ERR("\n AVS PWR status error = %x\n", pwr_avs_request[pwr_avs_bus].request.avs_response_status);
        pwr_avs_request[pwr_avs_bus].error = true;
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL, NULL);
        return;
    }

    switch (pwr_avs_request[pwr_avs_bus].request.Header.RequestType)
    {
    case AVS_REQUEST_WRITE_DATA:
        if (pwr_avs_request[pwr_avs_bus].request.avs_response_single_resp.error.v_done == 1)
        {
            //! Send the current voltage set to the power control loop
            uint16_t current_vcpu = pwr_avs_request[pwr_avs_bus].request.avs_params.avs_data;
            power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, (void*)(uintptr_t)current_vcpu);
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

void AVSPwrKnobWriteRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(Request);
    int pwr_avs_bus = (int)CompletionContext;

    pwr_avs_request[pwr_avs_bus].in_use = false;

    BUG_ASSERT(pwr_avs_request[pwr_avs_bus].request.avs_response_status == SCP_AVS_STATUS_SUCCESS);

    switch (pwr_avs_request[pwr_avs_bus].request.Header.RequestType)
    {
    case AVS_REQUEST_WRITE_DATA:
    case AVS_REQUEST_WRITE_MULTI:
        if (pwr_avs_request[pwr_avs_bus].request.avs_response_single_resp.error.v_done == 1)
        {
            POWER_LOG_TRACE("\n AVS PWR knob init Write complete\n");
        }
        break;

    default:
        // unexpected to receive any other event in this state,
        POWER_LOG_ERR("\n AVSPwrKnobWriteRequestCompletion, unexpected RequestType\n");
        break;
    }
}

void AVSPwrReadRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(Request);
    int pwr_avs_bus = (int)CompletionContext;

    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    pwr_avs_request[pwr_avs_bus].in_use = false;

    if (!power_hw_has_avs_devices()) // If no VRs then complete and return.
    {
        if (pwr_avs_request[pwr_avs_bus].request.Header.RequestType == AVS_REQUEST_READ_DATA)
        {
            power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
        }
        else // //AVS_REQUEST_READ_MULTI
        {
            power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VR_READ, &s_power_vrs_ctx.latest_power);
            power_loops_vr_telem_handle_event(POWER_VR_TELEM_SIGNAL_VR_CURRENT, s_power_vrs_ctx.vr_inputs);
        }
        return;
    }

    if (pwr_avs_request[pwr_avs_bus].request.avs_response_status != SCP_AVS_STATUS_SUCCESS)
    {
        POWER_LOG_ERR("\n AVS PWR status error = %x\n", pwr_avs_request[pwr_avs_bus].request.avs_response_status);
        pwr_avs_request[pwr_avs_bus].error = true;
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

    case AVS_REQUEST_READ_MULTI:
        int temp_vr_index;
        int resp_idx = 0;
        temp_vr_index = (pwr_avs_bus * MAX_AVS_RAILS); // index of the first VR in the AVS bus, since vr_inputs is a flattened array.
        //  voltage is the first, current is the 2nd, and temperature the 3rd in the multi-read
        BUG_ASSERT(temp_vr_index < MAX_VR_PER_DIE);

        for (int i = 0; i < MAX_AVS_RAILS; i++, temp_vr_index++)
        {
            s_power_vrs_ctx.vr_inputs[temp_vr_index].voltage =
                pwr_avs_request[pwr_avs_bus].request.avs_resp_multi.avs_response_multi[resp_idx++].data;
            s_power_vrs_ctx.vr_inputs[temp_vr_index].current =
                pwr_avs_request[pwr_avs_bus].request.avs_resp_multi.avs_response_multi[resp_idx++].data;
            s_power_vrs_ctx.vr_inputs[temp_vr_index].temperature =
                pwr_avs_request[pwr_avs_bus].request.avs_resp_multi.avs_response_multi[resp_idx++].data;
            POWER_LOG_TRACE("voltage[%d] = %d\n", temp_vr_index, s_power_vrs_ctx.vr_inputs[temp_vr_index].voltage);
            POWER_LOG_TRACE("current[%d] = %d\n", temp_vr_index, s_power_vrs_ctx.vr_inputs[temp_vr_index].current);
            POWER_LOG_TRACE("temperature[%d] = %d\n", temp_vr_index, s_power_vrs_ctx.vr_inputs[temp_vr_index].temperature);
        }

        if ((all_requests_completed(pwr_avs_request, (p_config->num_vr / MAX_AVS_RAILS))) &&
            (no_errors(pwr_avs_request, (p_config->num_vr / MAX_AVS_RAILS))))
        {
            // calculate local soc power and send response
            calculate_soc_power();

            // // store for control loop
            s_power_vrs_ctx.latest_power.soc_power = s_power_vrs_ctx.soc_power[0]; // most recent soc power measurement
            s_power_vrs_ctx.latest_power.vcpu_power = s_power_vrs_ctx.vcpu_power_log[0]; // most recent vcpu power measurement
            // voltage is the first ,current is the 2nd response, and temperature the 3rd in the multi-read;
            // 0 = voltage rail 0, 1 = current rail 0, 3 = temperature rail 0
            s_power_vrs_ctx.latest_power.vcpu_avs_voltage =
                s_power_vrs_ctx
                    .vr_inputs[p_config->vr_idx_info.vcpu_idx - p_config->vr_idx_info.flattened_vr_start_idx]
                    .voltage;

            s_power_vrs_ctx.latest_power.vcpu_avs_current =
                s_power_vrs_ctx
                    .vr_inputs[p_config->vr_idx_info.vcpu_idx - p_config->vr_idx_info.flattened_vr_start_idx]
                    .current;

            power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VR_READ, &s_power_vrs_ctx.latest_power);
            power_loops_vr_telem_handle_event(POWER_VR_TELEM_SIGNAL_VR_CURRENT, s_power_vrs_ctx.vr_inputs);
        }
        else if (all_requests_completed(pwr_avs_request, (p_config->num_vr / MAX_AVS_RAILS))) // if all requests are completed but there are errors
        {
            printf("AVS PWR read error, some VRs failed to read\n");
            power_loops_vr_telem_handle_event(POWER_VR_TELEM_SIGNAL_VR_CURRENT_FAIL, NULL);
        }
        break;

    default:
        // unexpected to receive any other event in this state,
        POWER_LOG_ERR("\n AVSPwrReadRequestCompletion, unexpected RequestType\n");
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL, NULL);
        break;
    }
}

void pwr_hw_vrs_init(void)
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    uint8_t vr_start_index;
    vr_start_index = p_config->vr_idx_info.flattened_vr_start_idx;

    BUG_ASSERT(vr_start_index < DIMOF(p_runconfig->knobs.forced_vrs.vr));

    int avs_bus;
    uint8_t vr_index;

    //! Check if we need to override VSYS default (vsys vboot = 921 mV) voltage programmed at boot
    //! Check if override is enabled and forced_vrs for VSYS is zero
    //! If forced_vrs for VSYS is non-zero then it takes precedence over fuse value
    if ((p_runconfig->knobs.enable_vsys_vboot_override) && (p_runconfig->knobs.forced_vrs.vr[MPCL_VR_VSYS] == 0))
    {
        POWER_LOG_INFO(" PWR AVS VSYS Vboot override enabled, setting VSYS VR to fuse VID %d mV \n",
                       p_runconfig->fuses.vsys_vid_mv);
        //! Update forced_vrs for VSYS to the fuse value, this will be used to program the VR in the next steps
        p_runconfig->knobs.forced_vrs.vr[MPCL_VR_VSYS] = p_runconfig->fuses.vsys_vid_mv;
    }

    //! Iterate through all AVS buses and check if any VRs are forced to a value
    for (avs_bus = 0, vr_index = vr_start_index; avs_bus < (p_config->num_vr / MAX_AVS_RAILS); avs_bus++, vr_index++)
    {
        // Check to see if both VRs are forced to a value.  If so then write both VRs on the same AVSBus (scp_avs_client_write_multi)
        if ((p_runconfig->knobs.forced_vrs.vr[vr_index] != 0) && (p_runconfig->knobs.forced_vrs.vr[vr_index + 1] != 0))
        {
            POWER_LOG_TRACE(" Both PWR AVS knobs not zero, vr_index = %d, knob = %d, knob 2 = %d \n",
                            vr_index,
                            p_runconfig->knobs.forced_vrs.vr[vr_index],
                            p_runconfig->knobs.forced_vrs.vr[vr_index + 1]);

            pwr_avs_request[p_config->avs_details[vr_index].bus_id].in_use = true;
            pwr_avs_request[p_config->avs_details[vr_index].bus_id]
                .request.avs_resp_multi.avs_response_multi[AVS_RAIL0]
                .data = p_runconfig->knobs.forced_vrs.vr[vr_index];
            pwr_avs_request[p_config->avs_details[vr_index].bus_id]
                .request.avs_resp_multi.avs_response_multi[AVS_RAIL0]
                .error.as_uint8 = AVS_ERROR_NONE;

            vr_index++; // write second VR on the same AVSBus
            pwr_avs_request[p_config->avs_details[vr_index].bus_id].in_use = true;
            pwr_avs_request[p_config->avs_details[vr_index].bus_id]
                .request.avs_resp_multi.avs_response_multi[AVS_RAIL1]
                .data = p_runconfig->knobs.forced_vrs.vr[vr_index];
            pwr_avs_request[p_config->avs_details[vr_index].bus_id]
                .request.avs_resp_multi.avs_response_multi[AVS_RAIL1]
                .error.as_uint8 = AVS_ERROR_NONE;

            scp_avs_client_write_multi(&pwr_avs_interfaces[avs_bus]->Header,
                                       &pwr_avs_request[avs_bus].request.Header,
                                       AVSPwrKnobWriteRequestCompletion,
                                       (void*)avs_bus);
        }
        else if ((p_runconfig->knobs.forced_vrs.vr[vr_index] != 0) ||
                 (p_runconfig->knobs.forced_vrs.vr[vr_index + 1] != 0))
        {
            // Only one VR is forced to a value, write the single VR on the AVSBus (scp_avs_client_write)
            bool index_incremented = false;
            if (p_runconfig->knobs.forced_vrs.vr[vr_index + 1] != 0)
            {
                vr_index++; // pre-incremented to write the second VR on the same AVSBus
                index_incremented = true;
            }
            POWER_LOG_INFO(" PWR AVS one knob not zero, vr_index = %d, knob = %d, avs_bus = %d \n",
                           vr_index,
                           p_runconfig->knobs.forced_vrs.vr[vr_index],
                           avs_bus);

            pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_response_single_resp.error.as_uint8 =
                AVS_ERROR_NONE;
            pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_info.rail_id =
                p_config->avs_details[vr_index].rail_id;
            pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_data =
                p_runconfig->knobs.forced_vrs.vr[vr_index];
            pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_info.cmd_type = AVS_VOLTAGE_RW;
            pwr_avs_request[p_config->avs_details[vr_index].bus_id].in_use = true;

            scp_avs_client_write(&pwr_avs_interfaces[p_config->avs_details[vr_index].bus_id]->Header,
                                 &pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.Header,
                                 AVSPwrKnobWriteRequestCompletion,
                                 (void*)(uintptr_t)p_config->avs_details[vr_index].bus_id);

            if (!index_incremented)
            {
                vr_index++; // post increment to skip over the second VR
            }
        }
    }

    POWER_LOG_TRACE(" PWR AVS init VRs (knobs) complete \n");
}

void power_vrs_initiate_vr_reads()
{
    // setup read requests based on AVS config
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    uint8_t vr_vcpu_idx;
    vr_vcpu_idx = (p_config->vr_idx_info.vcpu_idx - p_config->vr_idx_info.flattened_vr_start_idx);
    BUG_ASSERT(vr_vcpu_idx <= MAX_VR_PER_DIE);

    if (!all_requests_completed(pwr_avs_request, (p_config->num_vr / MAX_AVS_RAILS)))
    {
        // can't send another request if current ones not completed, just signal failure and wait for retry
        power_loops_vr_telem_handle_event(POWER_VR_TELEM_SIGNAL_VR_CURRENT_FAIL, NULL);
        return;
    }

    reset_errors(pwr_avs_request, (p_config->num_vr / MAX_AVS_RAILS));
    memset(pwr_avs_request[0].request.avs_resp_multi.avs_response_multi,
           0,
           sizeof(pwr_avs_request[0].request.avs_resp_multi.avs_response_multi[0]) * MAX_AVS_MULTI_READ_CMDS);
    for (int i = 0; i < MAX_AVS_INST; i++)
    {
        pwr_avs_request[i].in_use =
            true; // need to set all to 'true', since these won't be processed in the callback until all AVSBus reads are completed
    }

    int avs_bus;
    uint8_t index;
    uint8_t vr_index;
    for (avs_bus = 0, vr_index = vr_vcpu_idx; avs_bus < (p_config->num_vr / MAX_AVS_RAILS); avs_bus++, vr_index++)
    {
        index = 0;
        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].rail_id =
            p_config->avs_details[vr_index].rail_id;
        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].cmd_type =
            AVS_VOLTAGE_RW;
        index++;
        BUG_ASSERT(index < AVS_CMD_BUFF_SIZE);

        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].rail_id =
            p_config->avs_details[vr_index].rail_id;
        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].cmd_type =
            AVS_CURRENT_READ;
        index++;
        BUG_ASSERT(index < AVS_CMD_BUFF_SIZE);

        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].rail_id =
            p_config->avs_details[vr_index].rail_id;
        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].cmd_type =
            AVS_TEMPERATURE_READ;
        index++;
        BUG_ASSERT(index < AVS_CMD_BUFF_SIZE);

        vr_index++; // read next VR on the same AVSBus
        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].rail_id =
            p_config->avs_details[vr_index].rail_id;
        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].cmd_type =
            AVS_VOLTAGE_RW;
        index++;
        BUG_ASSERT(index < AVS_CMD_BUFF_SIZE);

        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].rail_id =
            p_config->avs_details[vr_index].rail_id;
        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].cmd_type =
            AVS_CURRENT_READ;
        index++;
        BUG_ASSERT(index < AVS_CMD_BUFF_SIZE);

        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].rail_id =
            p_config->avs_details[vr_index].rail_id;
        pwr_avs_request[p_config->avs_details[vr_index].bus_id].request.avs_params.avs_cmd_array[index].cmd_type =
            AVS_TEMPERATURE_READ;
        index++;
        BUG_ASSERT(index < AVS_CMD_BUFF_SIZE);

        scp_avs_client_read_multi(&pwr_avs_interfaces[avs_bus]->Header,
                                  &pwr_avs_request[avs_bus].request.Header,
                                  AVSPwrReadRequestCompletion,
                                  (void*)avs_bus,
                                  (uint8_t)MAX_AVS_MULTI_READ_CMDS);
    }
}

void power_vrs_write_vcpu_voltage(uint16_t voltage_mv)
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    // any boot VR request changes should have completed

    uint8_t vcpu_index;
    vcpu_index = p_config->vr_idx_info.vcpu_idx;

    BUG_ASSERT(vcpu_index < DIMOF(p_runconfig->knobs.forced_vrs.vr));
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
    if (!(all_requests_completed(pwr_avs_request, (p_config->num_vr / MAX_AVS_RAILS))))
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

    reset_errors(pwr_avs_request, (p_config->num_vr / MAX_AVS_RAILS));

    scp_avs_client_write(&pwr_avs_interfaces[p_config->avs_details[vcpu_index].bus_id]->Header,
                         &pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.Header,
                         AVSPwrWriteRequestCompletion,
                         (void*)(uintptr_t)p_config->avs_details[vcpu_index].bus_id);
    POWER_LOG_TRACE(" PWR AVS write compl. \n");
}

void power_vrs_read_vcpu_voltage()
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    uint8_t vcpu_index;
    vcpu_index = p_config->vr_idx_info.vcpu_idx;

    if (!(all_requests_completed(pwr_avs_request, (p_config->num_vr / MAX_AVS_RAILS))))
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
    pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].error = false;

    scp_avs_client_read(&pwr_avs_interfaces[p_config->avs_details[vcpu_index].bus_id]->Header,
                        &pwr_avs_request[p_config->avs_details[vcpu_index].bus_id].request.Header,
                        AVSPwrReadRequestCompletion,
                        (void*)(uintptr_t)p_config->avs_details[vcpu_index].bus_id);

    POWER_LOG_TRACE(" PWR AVS read compl. \n");
}

static void calculate_soc_power()
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    // initialize with static rail power
    //! Only include static rail on die 0
    float soc_power = 0;
    if (p_config->is_primary_die)
    {
        soc_power = p_runconfig->knobs.static_rail_power_watts;
    }
    float vcpu_power = 0;
    //! pick right loadline knob for die (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/)
    float r_loadline_ohms = 0.000001F * (p_runconfig->p_sconfig->is_primary_die
                                             ? p_runconfig->knobs.r_loadline_vcpu0_uohm
                                             : p_runconfig->knobs.r_loadline_vcpu1_uohm); // loadline knobs in uOhms
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

    POWER_LOG_TRACE(" VCPU power = %f, VCPU current = %f, SOC power = %f\n",
                    s_power_vrs_ctx.vcpu_power_log[0],
                    s_power_vrs_ctx.vcpu_current_log[0],
                    s_power_vrs_ctx.soc_power[0]);
}

uint32_t power_vrs_get_recent_power_mw()
{
    // TODO: updates for remote die (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/)
    // likely want an API that can be used to pass in the remote power to also log and include in this calculation
    float total = 0.0F;
    for (int meas_idx = 0; meas_idx < SOC_POWER_AVG_COUNT; meas_idx++)
    {
        total += (s_power_vrs_ctx.soc_power[meas_idx] + s_power_vrs_ctx.remote_soc_power[meas_idx]);
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
    reset_errors(pwr_avs_request, (p_config->num_vr / MAX_AVS_RAILS));
    pwr_hw_vrs_init();
}

void store_remote_soc_power(power_latest_calcs_t* p_remote_power)
{
    // move 0-(x-1) to 1-x  -- leaving index 0 open for new measurement
    memmove(&s_power_vrs_ctx.remote_soc_power[1],
            &s_power_vrs_ctx.remote_soc_power[0],
            sizeof(s_power_vrs_ctx.remote_soc_power[0]) * (SOC_POWER_AVG_COUNT - 1));
    // save most recent measurement to index 0
    s_power_vrs_ctx.remote_soc_power[0] = p_remote_power->soc_power;
}

power_vrs_context_t* get_power_vrs_context()
{
    return &s_power_vrs_ctx;
}