//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_package_defs.h
 * This file contains the definitions of structures used for packages for in band telemetry.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "stdint.h"
#include "common_types.h"

#include <diag_decoder.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...

/*-- Symbolic Constant Macros (defines) --*/

#define NUMBER_OF_TILES_PER_DIE (34)
#define NUMBER_OF_CORES_PER_DIE (NUMBER_OF_TILES_PER_DIE*2)
#define NUMBER_OF_HNF_CHANNELS_PER_DIE (NUMBER_OF_TILES_PER_DIE*2)
#define NUMBER_OF_PSTATES (32)
#define NUMBER_OF_CSTATES (5)

#define NUMBER_OF_THROTTLE_TYPES    (7)
#define NUMBER_OF_RACK_PRIORITIES   (8)
#define NUMBER_OF_HS_VOLTAGE_SCALES (17)
#define NUMBER_OF_HS_TEMP_SCALES    (7)

// TODO: These values are placeholders and need to be verified
#define NUMBER_OF_DIMM_MODULES      (12)
#define NUMBER_OF_DIMM_CHANNELS     (24)
#define NUMBER_OF_MPAMS             (128)
#define NUMBER_OF_AMU_COUNTERS      (7)

/*-------------- Typedefs ----------------*/

typedef enum
{
    POWER_TELEMETRY_EVENT_CORE_PSTATE,
    POWER_TELEMETRY_EVENT_CORE_CSTATE,
    POWER_TELEMETRY_EVENT_CORE_THROTTLE,
    POWER_TELEMETRY_EVENT_CORE_RACK_PRIORITIES,
    POWER_TELEMETRY_EVENT_CORE_VOLTAGE,
    POWER_TELEMETRY_EVENT_CORE_CURRENT,
    POWER_TELEMETRY_EVENT_CORE_TEMPERATURE,
    POWER_TELEMETRY_EVENT_CORE_HISTOGRAM,
    POWER_TELEMETRY_EVENT_SOC_PC3,
    POWER_TELEMETRY_EVENT_SOC_VR_RAILS,
    POWER_TELEMETRY_EVENT_SOC_HNF,
    POWER_TELEMETRY_EVENT_SOC_DIMM,
    POWER_TELEMETRY_EVENT_SOC_SENSOR_TEMP,
    POWER_TELEMETRY_EVENT_MPAM,
    POWER_TELEMETRY_EVENT_MPAM_THROTTLE,
    POWER_TELEMETRY_EVENT_ID_MAX
} power_telemetry_event_t;

typedef enum
{
    PERFORMANCE_TELEMETRY_EVENT_CORE,
    PERFORMANCE_TELEMETRY_EVENT_SOC_RAILS,
    PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_RT,
    PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_CONFIG,
    PERFORMANCE_TELEMETRY_EVENT_SOC_TEMP_SENSOR,
    PERFORMANCE_TELEMETRY_EVENT_AMU,
    PERFORMANCE_TELEMETRY_EVENT_ID_MAX
} performance_telemetry_event_t;

#pragma pack(push, 1)
typedef struct {
    guid_t manifest_id;
    uint64_t timestamp;
    uint32_t package_number;
    uint32_t number_of_records;
    uint32_t package_payload_size;
} telemetry_client_header_t;

typedef struct {
    diag_decoder_payload_header_t decoder_header;
    telemetry_client_header_t client_header;

} telemetry_package_hdr_t, *p_telemetry_package_hdr_t;


typedef struct {
    uint16_t provider_id;
    uint16_t event_id;
    uint16_t collection_id;
    uint16_t number_of_events;
    uint32_t collection_payload_size;   // minus the collection header size
} telemetry_collection_hdr_t, *p_telemetry_collection_hdr_t;
typedef struct {
    uint64_t timestamp;
    uint32_t record_number;
    uint32_t number_of_collections;
    uint32_t record_payload_size;  //  minus the record header size
} telemetry_record_hdr_t, *p_telemetry_record_hdr_t;


typedef struct {
    uint16_t max_mV;
    uint16_t min_mV;
    uint16_t average_mV;
    uint16_t latest_value_mV;  // defined as reserved for tlm reports
} voltage_t;

typedef struct {
    uint16_t max_dC;
    uint16_t min_dC;
    uint16_t average_dC;
    uint16_t latest_value_dC; // defined as reserved for tlm reports
} temperature_t;

typedef struct {
    uint16_t max_mA;
    uint16_t min_mA;
    uint16_t average_mA;
    uint16_t latest_value_mA; // defined as reserved for tlm reports
} current_t;

typedef struct {
    uint16_t max_uS;
    uint16_t min_uS;
    uint16_t average_uS;
    uint16_t latest_value_uS; // defined as reserved for tlm reports
} latency_t;

//----------------POWER_TELEMETRY_EVENT_CORE_PSTATE----------------
typedef struct {
    uint8_t pstate_id;
    uint16_t frequency_Mhz;
    uint16_t max_power_mW;
    uint16_t min_power_mW;
    uint16_t avg_power_mW;
    uint32_t residency_mS;
    uint32_t entry_count;
} pwr_core_event_pstate_t, *p_pwr_core_event_pstate_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_event_pstate_t pstate_event[NUMBER_OF_PSTATES];
} pwr_core_collection_pstate_t, *p_pwr_core_collection_pstate_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_pstate_t pstate_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_pstate_t, *p_pwr_core_record_pstate_t;

//----------------POWER_TELEMETRY_EVENT_CORE_CSTATE----------------
typedef struct {
    uint8_t cstate_id;
    uint32_t residency_mS;
    uint32_t entry_count;
    uint16_t max_power_mW;
    uint16_t min_power_mW;
    uint16_t avg_power_mW;
} pwr_core_event_cstate_t, *p_pwr_core_event_cstate_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_event_cstate_t cstate_event[NUMBER_OF_CSTATES];
} pwr_core_collection_cstate_t, *p_pwr_core_collection_cstate_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_cstate_t cstate_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_cstate_t, *p_pwr_core_record_cstate_t;

//----------------POWER_TELEMETRY_EVENT_CORE_THROTTLE----------------
typedef struct {
    uint8_t type_id;
    uint8_t max_pstate;
    uint8_t avg_pstate;
    uint8_t reserved;
    uint32_t entry_count;
    uint32_t exit_count;
    uint32_t residency_mS;
} pwr_core_event_throttle_t, *p_pwr_core_event_throttle_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_event_throttle_t throttle_event[NUMBER_OF_THROTTLE_TYPES];
} pwr_core_collection_throttle_t, *p_pwr_core_collection_throttle_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_throttle_t throttle_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_throttle_t, *p_pwr_core_record_throttle_t;

//----------------POWER_TELEMETRY_EVENT_CORE_RACK_PRIORITIES----------------

typedef struct {
    uint8_t priority_id;
    uint8_t max_pstate;
    uint8_t avg_pstate;
    uint8_t reserved;
    uint32_t entry_count;
    uint32_t exit_count;
    uint32_t residency_mS;
} pwr_core_event_rack_priorities_t, *p_pwr_core_event_rack_priorities_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_event_rack_priorities_t rack_priority_event[NUMBER_OF_RACK_PRIORITIES];
} pwr_core_collection_rack_priorities_t, *p_pwr_core_collection_rack_priorities_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_rack_priorities_t rack_priority_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_rack_priorities_t, *p_pwr_core_record_rack_priorities_t;

//----------------POWER_TELEMETRY_EVENT_CORE_VOLTAGE----------------

typedef voltage_t pwr_core_event_voltage_t, *p_pwr_core_event_voltage_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_event_voltage_t voltage_event;
} pwr_core_collection_voltage_t, *p_pwr_core_collection_voltage_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_voltage_t voltage_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_voltage_t, *p_pwr_core_record_voltage_t;

//----------------POWER_TELEMETRY_EVENT_CORE_CURRENT----------------

typedef current_t pwr_core_event_current_t, *p_pwr_core_event_current_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_event_current_t current_event;
} pwr_core_collection_current_t, *p_pwr_core_collection_current_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_current_t current_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_current_t, *p_pwr_core_record_current_t;

//----------------POWER_TELEMETRY_EVENT_CORE_TEMPERATURE----------------

typedef temperature_t pwr_core_event_temperature_t, *p_pwr_core_event_temperature_t;

typedef struct {
   telemetry_collection_hdr_t collection_header;
    pwr_core_event_temperature_t temperature_event;
} pwr_core_collection_temperature_t, *p_pwr_core_collection_temperature_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_temperature_t temperature_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_temperature_t, *p_pwr_core_record_temperature_t;

//----------------POWER_TELEMETRY_EVENT_CORE_HISTOGRAM----------------

typedef struct {
    uint8_t voltage_band;
    uint8_t temperature_band;
    uint32_t counter;
} pwr_core_event_histogram_t, *p_pwr_core_event_histogram_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_event_histogram_t histogram_event[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES];
} pwr_core_collection_histogram_t, *p_pwr_core_collection_histogram_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_histogram_t histogram_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_histogram_t, *p_pwr_core_record_histogram_t;

//----------------POWER_TELEMETRY_EVENT_SOC_PC3----------------

typedef struct {
    uint32_t duration_mS;
} pwr_soc_event_pc3_t, *p_pwr_soc_event_pc3_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_event_pc3_t pc3_event;
} pwr_soc_collection_pc3_t, *p_pwr_soc_collection_pc3_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_pc3_t pc3_collection;
} pwr_soc_record_pc3_t, *p_pwr_soc_record_pc3_t;

//----------------POWER_TELEMETRY_EVENT_SOC_VR_RAILS----------------

typedef struct {
    voltage_t voltage;
    current_t current;
    temperature_t temperature;
} pwr_soc_event_vr_rail_t, *p_pwr_soc_event_vr_rail_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_event_vr_rail_t rail_event;
} pwr_soc_collection_vr_rail_t, *p_pwr_soc_collection_vr_rail_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_vr_rail_t rail_collection[MAX_NUM_OF_VR_RAILS];
} pwr_soc_record_vr_rail_t, *p_pwr_soc_record_vr_rail_t;

//----------------POWER_TELEMETRY_EVENT_SOC_HNF----------------
typedef temperature_t pwr_soc_event_hnf_t, *p_pwr_soc_event_hnf_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_event_hnf_t hnf_event;
} pwr_soc_collection_hnf_t, *p_pwr_soc_collection_hnf_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_hnf_t hnf_collection[NUMBER_OF_HNF_CHANNELS_PER_DIE];
} pwr_soc_record_hnf_t, *p_pwr_soc_record_hnf_t;

//----------------POWER_TELEMETRY_EVENT_SOC_DIMM----------------

typedef struct {
    temperature_t s0;
    temperature_t s1;
} pwr_soc_event_dimm_t, *p_pwr_soc_event_dimm_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_event_dimm_t dimm_event;
} pwr_soc_collection_dimm_t, *p_pwr_soc_collection_dimm_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_dimm_t dimm_collection[NUMBER_OF_DIMM_MODULES];
} pwr_soc_record_dimm_t, *p_pwr_soc_record_dimm_t;

//----------------POWER_TELEMETRY_EVENT_SOC_SENSOR_TEMP----------------

typedef temperature_t pwr_soc_event_sensor_temp_t, *p_pwr_soc_event_sensor_temp_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_event_sensor_temp_t sensor_temp_event;
} pwr_soc_collection_sensor_temp_t, *p_pwr_soc_collection_sensor_temp_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_sensor_temp_t sensor_temp_collection[NUMBER_OF_SOC_TEMP_SENSORS];
} pwr_soc_record_sensor_temp_t, *p_pwr_soc_record_sensor_temp_t;

//----------------POWER_TELEMETRY_EVENT_MPAM----------------

typedef struct {
    uint8_t pstate_id;
    uint8_t reserved;
    uint16_t frequency_Mhz;
    uint32_t max_power_mW;
    uint32_t min_power_mW;
    uint32_t avg_power_mW;
    uint32_t residency_mS;
} pwr_event_mpam_pstate_t, *p_pwr_event_mpam_pstate_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_event_mpam_pstate_t mpam_pstate_event[NUMBER_OF_PSTATES];
} pwr_collection_mpam_pstate_t, *p_pwr_collection_mpam_pstate_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_collection_mpam_pstate_t mpam_pstate_collection[NUMBER_OF_MPAMS];
} pwr_record_mpam_pstate_t, *p_pwr_record_mpam_pstate_t;

//----------------POWER_TELEMETRY_EVENT_MPAM_THROTTLE----------------

typedef struct {
    uint16_t nominal_frequency;
    uint16_t max_pstate_frequency;
    uint16_t avg_throttle_frequency;
    uint16_t throttle_extent;
    uint32_t throttle_duration_mS;
} pwr_event_mpam_throttle_t, *p_pwr_event_mpam_throttle_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_event_mpam_throttle_t mpam_throttle_event;
} pwr_collection_mpam_throttle_t, *p_pwr_collection_mpam_throttle_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_collection_mpam_throttle_t mpam_throttle_collection[NUMBER_OF_MPAMS];
} pwr_record_mpam_throttle_t, *p_pwr_record_mpam_throttle_t;

//----------------PERFORMANCE_TELEMETRY_EVENT_CORE----------------

typedef struct {
    uint8_t pstate_id;
    uint8_t cstate_id;
    uint8_t cstate_residency_mS;
    uint8_t pstate_residency_mS;
    uint16_t frequency_Mhz;
    uint16_t power_mW;
    uint16_t cstate_plimit;
    uint16_t cstate_entry_latency_mS;
    uint16_t cstate_exit_latency_mS;
} perf_core_pcstate_info_t;

typedef struct {
    uint8_t throttle_type_priority_id;
    uint8_t throttle_type_residency;
    uint8_t throttle_priority_residency;
    uint8_t throttle_start_stop_id;
} perf_core_throttle_info_t;

typedef struct {
    uint16_t vct_voltage_mV;
    uint16_t vct_current_mA;
    uint16_t vct_temperature_dC;
    uint16_t vct_max_temp_dC;
} perf_core_vct_info_t;

typedef struct {
    perf_core_pcstate_info_t pc_state_info;
    perf_core_throttle_info_t throttle_info;
    perf_core_vct_info_t vct_info;
} perf_core_event_summary_t, *p_perf_core_event_summary_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    perf_core_event_summary_t summary_event;
} perf_core_collection_summary_t, *p_perf_core_collection_summary_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    perf_core_collection_summary_t perf_core_summary_collection[NUMBER_OF_CORES_PER_DIE];
} perf_core_record_summary_t, *p_perf_core_record_summary_t;

//----------------PERFORMANCE_TELEMETRY_EVENT_SOC_RAILS----------------

typedef struct {
    uint16_t voltage_mV;
    uint32_t current_mA;
    uint16_t temperature_dC;
} perf_soc_event_rail_t, *p_perf_soc_event_rail_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    perf_soc_event_rail_t rail_event;
} perf_soc_collection_rail_t, *p_perf_soc_collection_rail_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    perf_soc_collection_rail_t rail_collection[MAX_NUM_OF_VR_RAILS];
} perf_soc_record_rail_t, *p_perf_soc_record_rail_t;

//----------------PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_RT----------------

typedef struct {
    uint8_t memory_freq_id;
    uint8_t reserved;
    uint8_t throttling_flags;
    uint16_t residency_mS;
    uint16_t temp_s0_latest_dC;
    uint16_t temp_s1_latest_dC;
    uint16_t power_mW;
} perf_soc_event_dimm_runtime_t, *p_perf_soc_event_dimm_runtime_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    perf_soc_event_dimm_runtime_t dimm_rt_event;
} perf_soc_collection_dimm_runtime_t, *p_perf_soc_collection_dimm_runtime_t;


typedef struct {
    telemetry_record_hdr_t record_header;
    perf_soc_collection_dimm_runtime_t dimm_collection[NUMBER_OF_DIMM_MODULES];
} perf_soc_record_dimm_runtime_t, *p_perf_soc_record_dimm_runtime_t;

//----------------PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_CONFIG----------------

typedef struct {
    uint16_t temp_threshold_high_dC;
    uint16_t temp_threshold_low_dC;
    uint16_t temp_threshold_critical_dC;
} perf_soc_event_dimm_config_t, *p_perf_soc_event_dimm_config_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    perf_soc_event_dimm_config_t dimm_config_event;
} perf_soc_collection_dimm_config_t, *p_perf_soc_collection_dimm_config_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    perf_soc_collection_dimm_config_t dimm_config_collection;
} perf_soc_record_dimm_config_t, *p_perf_soc_record_dimm_config_t;

//----------------PERFORMANCE_TELEMETRY_EVENT_SOC_TEMP_SENSOR----------------

typedef temperature_t perf_soc_event_sensor_temp_t, *p_perf_soc_event_sensor_temp_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    perf_soc_event_sensor_temp_t temperature_event;
} perf_soc_collection_sensor_temp_t, *p_perf_soc_collection_sensor_temp_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    perf_soc_collection_sensor_temp_t temperature_collection[NUMBER_OF_SOC_TEMP_SENSORS];
} perf_soc_record_sensor_temp_t, *p_perf_soc_record_sensor_temp_t;

//----------------PERFORMANCE_TELEMETRY_EVENT_AMU----------------
typedef struct {
    uint64_t amevcntr_00;
    uint64_t amevcntr_01;
    uint64_t amevcntr_02;
    uint64_t amevcntr_03;
    uint64_t amevcntr_10;
    uint64_t amevcntr_11;
    uint64_t amevcntr_12;
} perf_core_event_amu_counters_t, *p_perf_core_event_amu_counters_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    union {
        uint64_t counters[NUMBER_OF_AMU_COUNTERS];
        perf_core_event_amu_counters_t amu_counter_event;
    };
} perf_core_collection_amu_counters_t, *p_perf_core_collection_amu_counters_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    perf_core_collection_amu_counters_t amu_counter_collection[NUMBER_OF_CORES_PER_DIE];
} perf_core_record_amu_counters_t, *p_perf_core_record_amu_counters_t;

#pragma pack(pop)

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
