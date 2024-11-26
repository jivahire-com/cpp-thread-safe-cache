//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_schema.c
 * This file creates a schema for the telemetry events that is packaged into the release and then used by
 * telemetry decoders to decode the telemetry events.
 */

// this file serves the purpose of pushing the schema into the elf file, but nothing calls these trace
// functions therefore unused warnings are disabled for this file
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"

/*------------- Includes -----------------*/
#define FPFW_ET_IMPLEMENTATION
#define FPFW_ET_METADATA
#define FPFW_ET_NO_ALIGNMENT_CHECKS

#include "telemetry_package_defs.h"

#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                           TlmSvcPowerEv,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                           TlmSvcPerfEv,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_PSTATE,
                     pwr_core_element_pstate,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, pstate_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, avg_power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, entry_count))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_CSTATE,
                     pwr_core_element_cstate,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, cstate_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, entry_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, avg_power_mW))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_THROTTLE,
                     pwr_core_element_throttle,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, max_pstate),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, avg_pstate),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, reserved),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, entry_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, exit_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, residency_mS))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES,
                     pwr_core_element_rack_priorities,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, priority_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, max_pstate),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, avg_pstate),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, reserved),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, entry_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, exit_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, residency_mS))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE,
                     pwr_core_element_voltage,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_CURRENT,
                     pwr_core_element_current,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE,
                     pwr_core_element_temperature,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM,
                     pwr_core_element_histogram,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, voltage_band),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, temperature_band),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, counter))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_PC3,
                     pwr_soc_element_pc3,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, duration_mS))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS,
                     pwr_soc_element_rail,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, voltage_reserved),

                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, max_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, min_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, average_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, current_reserved),

                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_HNF,
                     pwr_soc_element_hnf,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_DIMM,
                     pwr_soc_element_dimm,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s0_max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s0_min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s0_average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s0_temperature_reserved),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s1_max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s1_min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s1_average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s1_temperature_reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP,
                     pwr_soc_element_sensor_temp,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_MPAM,
                     pwr_element_mpam_pstate,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, pstate_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, reserved),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, max_power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, min_power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, avg_power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, residency_mS))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_MPAM_THROTTLE,
                     pwr_element_mpam_throttle,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, nominal_frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_pstate_frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, avg_throttle_frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, throttle_extent_pct),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, throttle_duration_mS))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_CORE,
                     inst_core_element_summary,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, pstate_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, cstate_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, cstate_residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, pstate_residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, cstate_plimit),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, cstate_entry_latency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, cstate_exit_latency_mS),

                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, throttle_type_priority_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, throttle_type_residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, throttle_priority_residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, throttle_start_stop_id),

                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, vct_voltage_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, vct_current_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, vct_temperature_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, vct_max_temp_dC))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_SOC_RAILS,
                     inst_soc_element_rail,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, voltage_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, current_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_dC))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_SOC_DIMM_RT,
                     inst_soc_element_dimm_runtime,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, memory_freq_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, reserved),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, throttling_flags),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temp_s0_latest_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temp_s1_latest_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, power_mW))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_SOC_DIMM_CONFIG,
                     inst_soc_element_dimm_config,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temp_threshold_high_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temp_threshold_low_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temp_threshold_critical_dC))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR,
                     inst_soc_element_sensor_temp,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserve))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_AMU,
                     inst_core_element_amu_counters,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, amevcntr_00),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, amevcntr_01),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, amevcntr_02),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, amevcntr_03),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, amevcntr_10),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, amevcntr_11),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, amevcntr_12))

#pragma GCC diagnostic pop