//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file element_schema.h
 * This file creates a schema for the telemetry elements that are packaged into the release and then used by
 * telemetry decoders to decode the telemetry elements.
 */


/*------------- Includes -----------------*/
#include "telemetry_package_defs.h"

// telemetry packages are unaligned to avoid padding
// turn off checks for unaligned access
#define FPFW_ET_NO_ALIGNMENT_CHECKS

#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/



/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                           TlmSvcPowerSchema,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                           TlmSvcInstSchema,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_PSTATE,
                     pwr_core_element_pstate,
                     FPFW_ET_LEVEL_DEBUG,
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
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, cstate_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, entry_count))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_THROTTLE,
                     pwr_core_element_throttle,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, max_pstate),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, avg_pstate),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, reserved),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, entry_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, residency_mS))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES,
                     pwr_core_element_rack_priorities,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, entry_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, priority_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE,
                     pwr_core_element_voltage,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_CURRENT,
                     pwr_core_element_current,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE,
                     pwr_core_element_temperature,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_POWER,
                     pwr_core_element_power,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM,
                     pwr_core_element_histogram,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, voltage_band),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, temperature_band),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, counter))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_AGING,
                     pwr_core_element_aging,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, timestamp_uS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, unaged_counter),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, aged_counter),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, voltage_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, counter_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_DROOPS,
                     pwr_core_element_droop_count,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, droop_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, ldo_output_max_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, ldo_output_min_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, ldo_output_average_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved1),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, vcpu_input_max_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, vcpu_input_min_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, vcpu_input_average_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved2))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_CORE_RELIABILITY_GUARD_BAND,
                     pwr_core_element_guard_band,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, timestamp_uS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, guard_band_voltage_mV))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_PKG_MON,
                     pwr_soc_element_pkg_monitor,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, pc3_duration_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, pc4_duration_mS))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS,
                     pwr_soc_element_vr_rail,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, voltage_reserved),

                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, current_reserved),

                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE,
                     pwr_soc_element_dimm_temp,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s0_max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s0_min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s0_average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s0_temperature_reserved),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s1_max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s1_min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s1_average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, s1_temperature_reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_DIMM_POWER,
                     pwr_soc_element_dimm_power,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_HNF_TEMP,
                     pwr_soc_element_hnf,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP,
                     pwr_soc_element_sensor_temp,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_PER_DIE_MESH,
                     pwr_soc_element_die_mesh,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, m1_entry_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, m2_entry_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, m0_residency_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, m1_residency_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, m2_residency_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, delivery_perf_count))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_DIE_TO_DIE_LINK_STATE,
                     pwr_soc_element_d2d_link,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, residency_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, bw_transmit),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, bw_receive),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, link_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_PER_DIE_PHY_COUNTERS,
                     pwr_soc_element_die_phy,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, counter0),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, counter1),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, counter2),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, counter3),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, counter4),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, counter5),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, counter6),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, counter7))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_MAX_TEMPERATURE,
                     pwr_soc_element_max_soc_temp,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_max_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, peak_max_dC))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_ACCEL_COUNTERS,
                     pwr_soc_element_accel_count,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, counter))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM,
                     pwr_soc_element_mpam_pstate,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, residency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, transition_count),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, pstate_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_THROTTLE,
                     pwr_soc_element_mpam_throttle,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, throttle_duration_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, nominal_pstate_frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_pstate_frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, avg_throttle_frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, throttle_extent_centipct))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA,
                     POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_POWER,
                     pwr_soc_element_mpam_power,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, max_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, min_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, average_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, reserved))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_CORE,
                     inst_core_element_summary,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, cstate_entry_latency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, cstate_exit_latency_mS),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, voltage_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, current_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, frequency_Mhz),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, guard_band_voltage_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, pstate_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, cstate_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, plimit),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, mpam_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, velocity_boost_priority),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, throttling_type_and_rack_priority))


FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS,
                     inst_soc_element_rail,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, voltage_mV),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, current_mA),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_dC))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_SOC_DIMM_RT,
                     inst_soc_element_dimm_runtime,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, threshold_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, power_mW),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, memory_freq_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, throttling_flags))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP,
                     inst_soc_element_die_temp,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, temperature_dC))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA,
                     INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP,
                     inst_soc_element_max_temp,
                     FPFW_ET_LEVEL_DEBUG,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, die0_max_temperature_dC),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, die1_max_temperature_dC))

