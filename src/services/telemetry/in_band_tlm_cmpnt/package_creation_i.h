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

/*-- Symbolic Constant Macros (defines) --*/
#define GEN_DATA_SCHEMA_ID(PROVIDER_ID, EVENT_ID) ((EVENT_ID << 16) | PROVIDER_ID)
/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern bool power_report_event_enable[POWER_TELEMETRY_EVENT_ID_MAX];
extern uint32_t power_report_record_number[POWER_TELEMETRY_EVENT_ID_MAX];

extern bool perf_report_event_enable[PERFORMANCE_TELEMETRY_EVENT_ID_MAX];
extern uint32_t perf_report_record_number[PERFORMANCE_TELEMETRY_EVENT_ID_MAX];

/*--------- Function Prototypes ----------*/
fpfw_status_t get_performance_record_include( size_t pkg_available_size, bool (*include_record)[PERFORMANCE_TELEMETRY_EVENT_ID_MAX]);
fpfw_status_t get_power_record_include( size_t pkg_available_size, bool (*include_record)[POWER_TELEMETRY_EVENT_ID_MAX]);

void create_pwr_core_pstate_record(p_pwr_core_record_pstate_t pstate_record);
void create_pwr_core_cstate_record(p_pwr_core_record_cstate_t cstate_record);
void create_pwr_core_throttle_record(p_pwr_core_record_throttle_t throttle_record);
void create_pwr_core_rack_priority_record(p_pwr_core_record_rack_priorities_t rack_priority_record);
void create_pwr_core_voltage_record(p_pwr_core_record_voltage_t voltage_record);
void create_pwr_core_current_record(p_pwr_core_record_current_t current_record);
void create_pwr_core_temperature_record(p_pwr_core_record_temperature_t temperature_record);
void create_pwr_core_histogram_record(p_pwr_core_record_histogram_t histogram_record);
void create_pwr_soc_pc3_record(p_pwr_soc_record_pc3_t pc3_record);
void create_pwr_soc_vr_rail_record(p_pwr_soc_record_vr_rail_t vr_rail_record);
void create_pwr_soc_hnf_record(p_pwr_soc_record_hnf_t hnf_record);
void create_pwr_soc_dimm_record(p_pwr_soc_record_dimm_t dimm_record);
void create_pwr_soc_sensor_temp_record(p_pwr_soc_record_sensor_temp_t snsr_temp_record);
void create_pwr_mpam_pstate_record(p_pwr_record_mpam_pstate_t mpam_record);
void create_pwr_mpam_throttle_record(p_pwr_record_mpam_throttle_t mpam_throttle_record);
void create_perf_core_summary_record(p_perf_core_record_summary_t summary_record);
void create_perf_soc_rail_record(p_perf_soc_record_rail_t rail_record);
void create_perf_soc_dimm_runtime_record(p_perf_soc_record_dimm_runtime_t dimm_record);
void create_perf_soc_dimm_config_record(p_perf_soc_record_dimm_config_t dimm_cfg_record);
void create_perf_soc_sensor_temp_record(p_perf_soc_record_sensor_temp_t snsr_temp_record);
void create_perf_core_amu_counters_record(p_perf_core_record_amu_counters_t amu_record);