//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_telemetry.c
 * Implementation of power telemetry interface functions.
 */

/*------------- Includes -----------------*/
#include "power_hw_int_i.h" // for power_telcfg_t, power_telemetry...
#include "power_loops_i.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <corebits.h>   // for corebits_set_bit
#include <power_runconfig.h>
#include <sensor_fifo_service.h>     // for sensor_fifo_api
#include <stdint.h>                  // for uint32_t
#include <telemetry_config_struct.h> // for telemetry_common_cfg_t
#include <telemetry_defines.h>       // for BUFFER_ADDRESS_INC_2QW, BUFFER_...

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void power_telemetry_init_config(power_telcfg_t* telemetry_config)
{
    FPFW_RUNTIME_ASSERT(telemetry_config != NULL);

    // read properties of SCF buffers/fifos to use for HW init
    sensor_fifo_properties_t properties;

    sensor_fifo_svc_get_properties(SENSOR_FIFO_PSTATE_TELEMETRY_HW, &properties);
    telemetry_config->pstate_telem_wr_address = properties.start_address_incl + FIFO_TIMESTAMP_SIZE;

    sensor_fifo_svc_get_properties(SENSOR_FIFO_SCP_MSG_TELEMETRY_HW, &properties);
    telemetry_config->scp_msg_telem_wr_address = properties.start_address_incl + FIFO_TIMESTAMP_SIZE;

    sensor_fifo_svc_get_properties(SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW, &properties);
    telemetry_config->current_telem_start_addr = properties.start_address_incl + FIFO_TIMESTAMP_SIZE;
    telemetry_config->current_telem_entry_size = properties.entry_size_bytes;
    telemetry_config->current_telem_buffer_size = properties.num_entries_or_strides - 1;
    telemetry_config->current_telem_stride_size = properties.stride_size_bytes;

    sensor_fifo_svc_get_properties(SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW, &properties);
    telemetry_config->temp_telem_start_addr = properties.start_address_incl + FIFO_TIMESTAMP_SIZE;
    telemetry_config->temp_telem_entry_size = properties.entry_size_bytes;
    telemetry_config->temp_telem_buffer_size = properties.num_entries_or_strides - 1;
    telemetry_config->temp_telem_stride_size = properties.stride_size_bytes;

    sensor_fifo_svc_get_properties(SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW, &properties);
    telemetry_config->volt_telem_start_addr = properties.start_address_incl + FIFO_TIMESTAMP_SIZE;
    telemetry_config->volt_telem_entry_size = properties.entry_size_bytes;
    telemetry_config->volt_telem_buffer_size = properties.num_entries_or_strides - 1;
    telemetry_config->volt_telem_stride_size = properties.stride_size_bytes;
}

void power_telemetry_enable()
{
    // after DVFS, PVT, ODCM telemetry is enabled, we need to enable individual fifos and deassert SCF trigger

    // enable fifos required/configured by power service
    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_PSTATE_TELEMETRY_HW);
    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_SCP_MSG_TELEMETRY_HW);
    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW);
    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW);
    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW);
    // sw fifos
    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_PVT_TEMP_FW);
    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_PVT_VOLTAGE_FW);
    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_VR_TEMP_FW);
    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_VR_CURRENT_FW);

    // deassert SCF trigger
    sensor_fifo_svc_set_global_hw_enable(true);
}

static void process_plimit_message(plimit_telem_msg_t* p_msg, power_hw_update_cb_t p_update_cb, power_hw_success_cb_t p_success_cb)
{
    // process message
    const unsigned int core_id = p_msg->data.core_id;
    if (core_id >= NUM_AP_CORES_PER_DIE)
    {
        // this is unexpected, but keep the print for early HW issues
        POWER_LOG_WARN("process_scp_message core%d unexpected", core_id);
        return;
    }

    switch (p_msg->data.type)
    {
    case PLIMIT_SUCCESS_TYPE:
        // a success message packet will be sent after evert PLIMIT register write
        // success messages will always include current CPPC desired perf value, so save it here
        p_success_cb(core_id, p_msg->data.plimit_success_msg.cppc_desired, p_msg->data.plimit);
        break;
    case PLIMIT_UPDATE_TYPE:
        // update message packets will be sent when a CPPC register is update in DVFS NON-SEC (OS write)
        // note that some fields in the definition are not named correctly.  The desired_perf in cppc_update_msg is actually the base pstate and pstate field is desired pstate in the update message (current pstate in success message)
        p_update_cb(core_id,
                    p_msg->data.pstate,                       // desired pstate
                    p_msg->data.cppc_update_msg.desired_perf, // base pstate
                    p_msg->data.cppc_update_msg.throttle_pri, // throttle priority
                    p_msg->data.cppc_update_msg.boost_pri);   // boost priority
        break;
    }
}

void power_telemetry_message_poll(power_hw_update_cb_t p_update_cb, power_hw_success_cb_t p_success_cb)
{
    // confirm the parameter isn't null
    FPFW_RUNTIME_ASSERT(p_update_cb != NULL);
    FPFW_RUNTIME_ASSERT(p_success_cb != NULL);

    // poll for messages from SCF
    sensor_ram_poll_status_t status = {.more_entries = true};
    while (status.more_entries)
    {
        plimit_telem_msg_t plimit_msg;
        status = sensor_fifo_svc_poll_scp_message(&plimit_msg);
        if (status.curr_data_is_valid)
        {
            // process valid messages
            process_plimit_message(&plimit_msg, p_update_cb, p_success_cb);
        }
    }
}