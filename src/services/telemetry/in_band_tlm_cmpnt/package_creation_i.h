//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file package_creation_i.h
 * This data could be static, but is provided in this header to facilitate unit testing.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "telemetry_package_defs.h"

#include <event_trace_providers.h>
#include <fpfw_status.h>
#include <message_transfer_service.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

extern bool power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_ID_MAX];
extern uint32_t power_pkg_record_number[POWER_TELEMETRY_ELEMENT_ID_MAX];

extern bool inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_ID_MAX];
extern uint32_t inst_pkg_record_number[INST_TELEMETRY_ELEMENT_ID_MAX];

extern uint8_t core_offset_per_die;
#define CORE_ID_WITH_DIE_OFFSET(core_id) ((core_id) + (core_offset_per_die))

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize any constant, post initialization, values for package creation.
 * 
 * @retval none
 */
void package_creation_init();

/**
 * @brief enable/disable a power record for packaging
 *
 * @param[in] element_id - record to enable
 * @param[in] enable_record - true to enable, false to disable
 * @retval none
 */
void package_create_enable_disable_pwr_record(pwr_telemetry_element_id_t element_id, bool enable_record);

/**
 * @brief enable/disable a instantaneous record for packaging
 *
 * @param[in] element_id - record to enable
 * @param[in] enable_record - true to enable, false to disable
 * @retval none
 */
void package_create_enable_disable_inst_record(instantaneous_telemetry_element_id_t element_id,  bool enable_record);

/**
 * @brief Populate the package header
 *
 * @param[in] package_hdr - package header
 * @retval none
 */
void package_create_populate_hdr(p_telemetry_package_hdr_t package_hdr);

/**
 * @brief Create a power package
 *
 * @param[in] pkg_location - location to store the package
 * @param[in] pkg_available_size - available size of the storage location
 * @retval uint32_t - Number of bytes in the package
 */
uint32_t package_create_power_pkg(uintptr_t pkg_location, size_t pkg_available_size);

/**
 * @brief Create a instantaneous package
 *
 * @param[in] curr_pkg_position - location in the current package to append the next records
 * @param[in] pkg_remaining_size - available size remaining in the package
 * @param[in,out] pkg_hdr - package header
 * @retval uint32_t - Number of bytes in the package
 */
uint32_t package_create_append_to_inst_pkg(uintptr_t curr_pkg_position, size_t pkg_remaining_size, p_telemetry_package_hdr_t pkg_hdr);

/**
 * @brief   Create a power core pstate record
 * @param[out] pstate_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_core_pstate_record(p_pwr_core_record_pstate_t pstate_record);

/**
 * @brief   Create a power core cstate record
 * @param[out] cstate_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_core_cstate_record(p_pwr_core_record_cstate_t cstate_record);

/**
 * @brief   Create a power core throttle record
 * @param[out] throttle_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_core_throttle_record(p_pwr_core_record_throttle_t throttle_record);

/**
 * @brief   Create a power core rack priority record
 * @param[out] rack_priority_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_core_rack_priority_record(p_pwr_core_record_rack_priorities_t rack_priority_record);

/**
 * @brief   Create a power core voltage record
 * @param[out] voltage_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_core_voltage_record(p_pwr_core_record_voltage_t voltage_record);

/**
 * @brief   Create a power core current record
 * @param[out] current_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_core_current_record(p_pwr_core_record_current_t current_record);

/**
 * @brief   Create a power core temperature record
 * @param[out] temperature_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_core_temperature_record(p_pwr_core_record_temperature_t temperature_record);

/**
 * @brief   Create a power core histogram record
 * @param[out] histogram_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_core_histogram_record(p_pwr_core_record_histogram_t histogram_record);

/**
 * @brief   Create a power soc pc3 record
 * @param[out] pc3_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_soc_pc3_record(p_pwr_soc_record_pc3_t pc3_record);

/**
 * @brief   Create a power soc vr rail record
 * @param[out] vr_rail_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_soc_vr_rail_record(p_pwr_soc_record_vr_rail_t vr_rail_record);

/**
 * @brief   Create a power soc hnf record
 * @param[out] hnf_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_soc_hnf_record(p_pwr_soc_record_hnf_t hnf_record);

/**
 * @brief   Create a power soc dimm record
 * @param[out] dimm_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_soc_dimm_record(p_pwr_soc_record_dimm_t dimm_record);

/**
 * @brief   Create a power soc sensor temp record
 * @param[out] snsr_temp_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_soc_sensor_temp_record(p_pwr_soc_record_sensor_temp_t snsr_temp_record);

/**
 * @brief   Create a power mpam pstate record
 * @param[out] mpam_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_mpam_pstate_record(p_pwr_record_mpam_pstate_t mpam_record);

/**
 * @brief   Create a power mpam throttle record
 * @param[out] mpam_throttle_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_pwr_mpam_throttle_record(p_pwr_record_mpam_throttle_t mpam_throttle_record);

/**
 * @brief   Create a instantaneous core summary record
 * @param[out] summary_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_inst_core_summary_record(p_inst_core_record_summary_t summary_record);

/**
 * @brief   Create a instantaneous soc rail record
 * @param[out] rail_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_inst_soc_rail_record(p_inst_soc_record_rail_t rail_record);

/**
 * @brief   Create a instantaneous soc dimm runtime record
 * @param[out] dimm_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_inst_soc_dimm_runtime_record(p_inst_soc_record_dimm_runtime_t dimm_record);

/**
 * @brief   Create a instantaneous soc dimm config record
 * @param[out] dimm_cfg_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_inst_soc_dimm_config_record(p_inst_soc_record_dimm_config_t dimm_cfg_record);

/**
 * @brief   Create a instantaneous soc sensor temp record
 * @param[out] snsr_temp_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_inst_soc_sensor_temp_record(p_inst_soc_record_sensor_temp_t snsr_temp_record);

/**
 * @brief   Create a instantaneous core amu counters record
 * @param[out] amu_record - location where the record will be stored
 * @return uint32_t - Number of bytes in the record
 */
uint32_t package_create_inst_core_amu_counters_record(p_inst_core_record_amu_counters_t amu_record);
