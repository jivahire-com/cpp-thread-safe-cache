//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_telemetry.c
 * Implementation of power telemetry interface functions.
 */

/*------------- Includes -----------------*/
#include "power_hw_int_i.h" // for power_telcfg_t, power_telemetry...

#include <sensor_ram_bridge.h>       // for init_telemetry_struct_configura...
#include <stdint.h>                  // for uint32_t
#include <telemetry_config_struct.h> // for telemetry_common_cfg_t
#include <telemetry_defines.h>       // for BUFFER_ADDRESS_INC_2QW, BUFFER_...

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// TODO:  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1811872
// these will be needed for sending telemetry and HW init

void power_telemetry_init_config(power_telcfg_t* telemetry_config)
{
    telemetry_common_cfg_t telem_config;
    // call sensor ram function which will provide a "default" configuration
    init_telemetry_struct_configuration(&telem_config);

    telemetry_config->pstate_telem_wr_address = telem_config.pstate_telem_fifo_start_address;
    telemetry_config->scp_msg_telem_wr_address = telem_config.scp_msg_fifo_start_address;

    telemetry_config->current_telem_start_addr = telem_config.current_telem_start_address;
    telemetry_config->current_telem_entry_size = BUFFER_ADDRESS_INC_2QW;
    telemetry_config->current_telem_buffer_size = telem_config.current_buffer_size;

    telemetry_config->temp_telem_start_addr = telem_config.temp_telem_start_address;
    telemetry_config->temp_telem_entry_size = BUFFER_ADDRESS_INC_4QW;
    telemetry_config->temp_telem_buffer_size = telem_config.temp_buffer_size;

    telemetry_config->volt_telem_start_addr = telem_config.volt_telem_start_address;
    telemetry_config->volt_telem_entry_size = BUFFER_ADDRESS_INC_2QW;
    telemetry_config->volt_telem_buffer_size = telem_config.volt_buffer_size;

    // TODO:  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1811872
    // read up on soc pvt implementation, looks like we're defining a buffer per sensor?
    telemetry_config->socpvttemp_telem_start_addr = telem_config.soc_pvt_temp_telem_start_address[0];
    telemetry_config->socpvttemp_telem_entry_size = BUFFER_ADDRESS_INC_2QW;

    telemetry_config->socpvtvolt_telem_start_addr = telem_config.soc_pvt_volt_telem_start_address[0];
    telemetry_config->socpvtvolt_telem_entry_size = BUFFER_ADDRESS_INC_2QW;

    telemetry_config->vrtemp_telem_start_addr = telem_config.vr_temp_start_address;
    telemetry_config->vrtemp_telem_entry_size = BUFFER_ADDRESS_INC_2QW;

    telemetry_config->vrcurr_telem_start_addr = telem_config.vr_curr_start_address;
    telemetry_config->vrcurr_telem_entry_size = BUFFER_ADDRESS_INC_2QW;
}
