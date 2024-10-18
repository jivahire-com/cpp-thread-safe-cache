//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file in_band_tlm_cmpnt_i.h
 * This file defines data and api's shared internally to the component and not with other components.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "telemetry_package_defs.h"

#include <event_trace_providers.h>
#include <fpfw_status.h>
#include <stddef.h>

/*-- Symbolic Constant Macros (defines) --*/


/*-------------- Typedefs ----------------*/
typedef struct
{
    telemetry_package_hdr_t package_header;
    pwr_core_record_pstate_t pstate_record;
    pwr_core_record_cstate_t cstate_record;
    pwr_core_record_throttle_t throttle_record;
    pwr_core_record_rack_priorities_t rack_priorities_record;
    pwr_core_record_voltage_t voltage_record;
    pwr_core_record_current_t current_record;
    pwr_core_record_temperature_t temperature_record;
    pwr_core_record_histogram_t histogram_record;
    pwr_soc_record_pc3_t pc3_record;
    pwr_soc_record_vr_rail_t vrail_record;
    pwr_soc_record_hnf_t hnf_record;
    pwr_soc_record_dimm_t dimm_record;
    pwr_soc_record_sensor_temp_t sensor_temp_record;
    pwr_record_mpam_pstate_t mpam_pstate_record;
    pwr_record_mpam_throttle_t mpam_throttle_record;
} power_report_full_package_t;

typedef struct
{
    telemetry_package_hdr_t package_header;
    inst_core_record_summary_t summary_record;
    inst_soc_record_rail_t rail_record;
    inst_soc_record_dimm_runtime_t dimm_runtime_record;
    inst_soc_record_dimm_config_t dimm_config_record;
    inst_soc_record_sensor_temp_t sensor_temp_record;
    inst_core_record_amu_counters_t amu_counters_record;

} inst_report_full_package_t;

/*-- Declarations (Statics and globals) --*/


/*--------- Function Prototypes ----------*/

/**
 * @brief Create a power report package
 *
 * @param[in] pkg_location - location to store the package
 * @param[in] pkg_available_size - available size of the storage location
 * @retval fpfw_status_t - returns FPFW_STATUS_BUFFER_TOO_SMALL if not all of the data could fit
 * send this package and call again with a new package until FPFW_STATUS_SUCCESS is returned
 */
fpfw_status_t package_create_power_report(uintptr_t pkg_location, size_t pkg_available_size);

/**
 * @brief Create a performance report package
 *
 * @param[in] pkg_location - location to store the package
 * @param[in] pkg_available_size - available size of the storage location
 * @retval fpfw_status_t - returns FPFW_STATUS_BUFFER_TOO_SMALL if not all of the data could fit
* send this package and call again with a new package until FPFW_STATUS_SUCCESS is returned
 */
fpfw_status_t package_create_inst_report(uintptr_t pkg_location, size_t pkg_available_size);

/**
 * @brief Allocate memory for power report
 *
 * @param[out] pkg_location - location to store the package
 * @param[out] available_size - size of the storage location
 * @retval fpfw_status_t
 */
fpfw_status_t ddr_manager_allocate_mem_for_pwr_report(uintptr_t *pkg_location, size_t* available_size);

/**
 * @brief Allocate memory for performance report
 *
 * @param[out] pkg_location - location to store the package
 * @param[out] available_size - size of the storage location
 * @retval fpfw_status_t
 */
fpfw_status_t ddr_manager_allocate_mem_for_inst_report(uintptr_t *pkg_location, size_t* available_size);