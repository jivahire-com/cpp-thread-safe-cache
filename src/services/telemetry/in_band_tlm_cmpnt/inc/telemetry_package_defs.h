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
    POWER_TELEMETRY_ELEMENT_CORE_PSTATE,
    POWER_TELEMETRY_ELEMENT_CORE_CSTATE,
    POWER_TELEMETRY_ELEMENT_CORE_THROTTLE,
    POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES,
    POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE,
    POWER_TELEMETRY_ELEMENT_CORE_CURRENT,
    POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE,
    POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM,
    POWER_TELEMETRY_ELEMENT_SOC_PC3,
    POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS,
    POWER_TELEMETRY_ELEMENT_SOC_HNF,
    POWER_TELEMETRY_ELEMENT_SOC_DIMM,
    POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP,
    POWER_TELEMETRY_ELEMENT_MPAM,
    POWER_TELEMETRY_ELEMENT_MPAM_THROTTLE,
    POWER_TELEMETRY_ELEMENT_ID_MAX
} pwr_telemetry_element_id_t;

typedef enum
{
    INST_TELEMETRY_ELEMENT_CORE,
    INST_TELEMETRY_ELEMENT_SOC_RAILS,
    INST_TELEMETRY_ELEMENT_SOC_DIMM_RT,
    INST_TELEMETRY_ELEMENT_SOC_DIMM_CONFIG,
    INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR,
    INST_TELEMETRY_ELEMENT_AMU,
    INST_TELEMETRY_ELEMENT_ID_MAX
} instantaneous_telemetry_element_id_t;

#pragma pack(push, 1)
typedef struct {
    guid_t manifest_id;
    uint64_t timestamp;  /* Timestamp when the package was created  */
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
    uint16_t element_id;
    uint16_t collection_id;
    uint16_t number_of_elements;
    uint32_t collection_payload_size;   // minus the collection header size
} telemetry_collection_hdr_t, *p_telemetry_collection_hdr_t;
typedef struct {
    uint64_t timestamp; /* Timestamp when the record was created  */
    uint32_t record_number;
    uint32_t number_of_collections;
    uint32_t record_payload_size;  //  minus the record header size
} telemetry_record_hdr_t, *p_telemetry_record_hdr_t;

typedef struct {
    uint16_t max_mV;
    uint16_t min_mV;
    uint16_t average_mV;
    uint16_t latest_value_mV;  // defined as reserved for tlm packages
} voltage_t;

typedef struct {
    uint16_t max_dC;
    uint16_t min_dC;
    uint16_t average_dC;
    uint16_t latest_value_dC; // defined as reserved for tlm packages
} temperature_t;

typedef struct {
    uint16_t max_mA;
    uint16_t min_mA;
    uint16_t average_mA;
    uint16_t latest_value_mA; // defined as reserved for tlm packages
} current_t;

typedef struct {
    uint16_t max_uS;
    uint16_t min_uS;
    uint16_t average_uS;
    uint16_t latest_value_uS; // defined as reserved for tlm packages
} latency_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_PSTATE----------------
typedef struct {
    uint8_t pstate_id;
    uint16_t frequency_Mhz;
    uint16_t max_power_mW;
    uint16_t min_power_mW;
    uint16_t avg_power_mW;
    uint32_t residency_mS;
    uint32_t entry_count;
} pwr_core_element_pstate_t, *p_pwr_core_element_pstate_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_pstate_t pstate_element[NUMBER_OF_PSTATES];
} pwr_core_collection_pstate_t, *p_pwr_core_collection_pstate_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_pstate_t pstate_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_pstate_t, *p_pwr_core_record_pstate_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_CSTATE----------------
typedef struct {
    uint8_t cstate_id;
    uint32_t residency_mS;
    uint32_t entry_count;
    uint16_t max_power_mW;
    uint16_t min_power_mW;
    uint16_t avg_power_mW;
} pwr_core_element_cstate_t, *p_pwr_core_element_cstate_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_cstate_t cstate_element[NUMBER_OF_CSTATES];
} pwr_core_collection_cstate_t, *p_pwr_core_collection_cstate_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_cstate_t cstate_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_cstate_t, *p_pwr_core_record_cstate_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_THROTTLE----------------
typedef struct {
    uint8_t type_id;
    uint8_t max_pstate;
    uint8_t avg_pstate;
    uint8_t reserved;
    uint32_t entry_count;
    uint32_t exit_count;
    uint32_t residency_mS;
} pwr_core_element_throttle_t, *p_pwr_core_element_throttle_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_throttle_t throttle_element[NUMBER_OF_THROTTLE_TYPES];
} pwr_core_collection_throttle_t, *p_pwr_core_collection_throttle_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_throttle_t throttle_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_throttle_t, *p_pwr_core_record_throttle_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES----------------

typedef struct {
    uint8_t priority_id;
    uint8_t max_pstate;
    uint8_t avg_pstate;
    uint8_t reserved;
    uint32_t entry_count;
    uint32_t exit_count;
    uint32_t residency_mS;
} pwr_core_element_rack_priorities_t, *p_pwr_core_element_rack_priorities_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_rack_priorities_t rack_priority_element[NUMBER_OF_RACK_PRIORITIES];
} pwr_core_collection_rack_priorities_t, *p_pwr_core_collection_rack_priorities_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_rack_priorities_t rack_priority_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_rack_priorities_t, *p_pwr_core_record_rack_priorities_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE----------------

typedef voltage_t pwr_core_element_voltage_t, *p_pwr_core_element_voltage_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_voltage_t voltage_element;
} pwr_core_collection_voltage_t, *p_pwr_core_collection_voltage_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_voltage_t voltage_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_voltage_t, *p_pwr_core_record_voltage_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_CURRENT----------------

typedef current_t pwr_core_element_current_t, *p_pwr_core_element_current_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_current_t current_element;
} pwr_core_collection_current_t, *p_pwr_core_collection_current_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_current_t current_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_current_t, *p_pwr_core_record_current_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE----------------

typedef temperature_t pwr_core_element_temperature_t, *p_pwr_core_element_temperature_t;

typedef struct {
   telemetry_collection_hdr_t collection_header;
    pwr_core_element_temperature_t temperature_element;
} pwr_core_collection_temperature_t, *p_pwr_core_collection_temperature_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_temperature_t temperature_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_temperature_t, *p_pwr_core_record_temperature_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM----------------

typedef struct {
    uint8_t voltage_band; /* voltage band, it will hold and index */
    uint8_t temperature_band; /* it's a temperature band, will hold an index*/
    uint32_t counter;
} pwr_core_element_histogram_t, *p_pwr_core_element_histogram_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_histogram_t histogram_element[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES];
} pwr_core_collection_histogram_t, *p_pwr_core_collection_histogram_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_histogram_t histogram_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_histogram_t, *p_pwr_core_record_histogram_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_PC3----------------

typedef struct {
    uint32_t duration_mS;
} pwr_soc_element_pc3_t, *p_pwr_soc_element_pc3_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_pc3_t pc3_element;
} pwr_soc_collection_pc3_t, *p_pwr_soc_collection_pc3_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_pc3_t pc3_collection;
} pwr_soc_record_pc3_t, *p_pwr_soc_record_pc3_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS----------------

typedef struct {
    voltage_t voltage;
    current_t current;
    temperature_t temperature;
} pwr_soc_element_vr_rail_t, *p_pwr_soc_element_vr_rail_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_vr_rail_t rail_element;
} pwr_soc_collection_vr_rail_t, *p_pwr_soc_collection_vr_rail_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_vr_rail_t rail_collection[MAX_NUM_OF_VR_RAILS];
} pwr_soc_record_vr_rail_t, *p_pwr_soc_record_vr_rail_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_HNF----------------
typedef temperature_t pwr_soc_element_hnf_t, *p_pwr_soc_element_hnf_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_hnf_t hnf_element;
} pwr_soc_collection_hnf_t, *p_pwr_soc_collection_hnf_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_hnf_t hnf_collection[NUMBER_OF_HNF_CHANNELS_PER_DIE];
} pwr_soc_record_hnf_t, *p_pwr_soc_record_hnf_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_DIMM----------------

typedef struct {
    temperature_t s0;
    temperature_t s1;
    uint8_t dimm_throttling;
    uint8_t dimm_memory_frequency_id;
    uint16_t dimm_power_mW;
} pwr_soc_element_dimm_t, *p_pwr_soc_element_dimm_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_dimm_t dimm_element;
} pwr_soc_collection_dimm_t, *p_pwr_soc_collection_dimm_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_dimm_t dimm_collection[NUMBER_OF_DIMM_MODULES];
} pwr_soc_record_dimm_t, *p_pwr_soc_record_dimm_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP----------------

typedef temperature_t pwr_soc_element_sensor_temp_t, *p_pwr_soc_element_sensor_temp_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_sensor_temp_t sensor_temp_element;
} pwr_soc_collection_sensor_temp_t, *p_pwr_soc_collection_sensor_temp_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_sensor_temp_t sensor_temp_collection[NUMBER_OF_SOC_TEMP_SENSORS];
} pwr_soc_record_sensor_temp_t, *p_pwr_soc_record_sensor_temp_t;

//----------------POWER_TELEMETRY_ELEMENT_MPAM----------------

typedef struct {
    uint8_t pstate_id;
    uint8_t reserved;
    uint16_t frequency_Mhz;
    uint32_t max_power_mW;
    uint32_t min_power_mW;
    uint32_t avg_power_mW;
    uint32_t residency_mS;
} pwr_element_mpam_pstate_t, *p_pwr_element_mpam_pstate_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_element_mpam_pstate_t mpam_pstate_element[NUMBER_OF_PSTATES];
} pwr_collection_mpam_pstate_t, *p_pwr_collection_mpam_pstate_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_collection_mpam_pstate_t mpam_pstate_collection[NUMBER_OF_MPAMS];
} pwr_record_mpam_pstate_t, *p_pwr_record_mpam_pstate_t;

//----------------POWER_TELEMETRY_ELEMENT_MPAM_THROTTLE----------------

typedef struct {
    uint16_t nominal_frequency_Mhz;
    uint16_t max_pstate_frequency_Mhz;
    uint16_t avg_throttle_frequency_Mhz;
    uint16_t throttle_extent_pct; /*mpam throttle in % we did,  w.r.t the nominal frequency (nominal_frequency_Mhz;).*/
    uint32_t throttle_duration_mS;
} pwr_element_mpam_throttle_t, *p_pwr_element_mpam_throttle_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_element_mpam_throttle_t mpam_throttle_element;
} pwr_collection_mpam_throttle_t, *p_pwr_collection_mpam_throttle_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_collection_mpam_throttle_t mpam_throttle_collection[NUMBER_OF_MPAMS];
} pwr_record_mpam_throttle_t, *p_pwr_record_mpam_throttle_t;

//----------------INST_TELEMETRY_ELEMENT_CORE----------------

//Latency in micro sec and residency in miliseconds. 
typedef struct {
    uint8_t pstate_id;
    uint8_t cstate_id;
    uint8_t cstate_residency_mS;
    uint8_t pstate_residency_mS;
    uint16_t frequency_Mhz;
    uint16_t power_mW;
    uint16_t cstate_plimit;
    uint16_t cstate_entry_latency_uS;
    uint16_t cstate_exit_latency_uS;
} inst_core_pcstate_info_t;

typedef struct {
    uint8_t throttle_type_priority_id;/* Priority ID represent  the Priority, there are 8 Priority Levels (0 to 7) */
    uint8_t throttle_type_residency_mS; /* Duration of such Throttling Type in mS : for throttle_type Current, Temp, VR Hot, Adaptive, Current Overrun, Aclk Overrun */
    /* Throttling Priorities are only for the Rack Throttling type: This residency indicate the duration of such a Priority Level in mS*/
    uint8_t throttle_priority_residency_mS;
    uint8_t throttle_start_stop_id;
} inst_core_throttle_info_t;

typedef struct {
    uint16_t vct_voltage_mV;
    uint16_t vct_current_mA;
    uint16_t vct_temperature_dC;
    uint16_t vct_max_temp_dC;
} inst_core_vct_info_t;

typedef struct {
    inst_core_pcstate_info_t pc_state_info;
    inst_core_throttle_info_t throttle_info;
    inst_core_vct_info_t vct_info;
} inst_core_element_summary_t, *p_inst_core_element_summary_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    inst_core_element_summary_t summary_element;
} inst_core_collection_summary_t, *p_inst_core_collection_summary_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    inst_core_collection_summary_t inst_core_summary_collection[NUMBER_OF_CORES_PER_DIE];
} inst_core_record_summary_t, *p_inst_core_record_summary_t;

//----------------INST_TELEMETRY_ELEMENT_SOC_RAILS----------------

typedef struct {
    uint16_t voltage_mV;
    uint32_t current_mA;
    uint16_t temperature_dC;
} inst_soc_element_rail_t, *p_inst_soc_element_rail_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    inst_soc_element_rail_t rail_element;
} inst_soc_collection_rail_t, *p_inst_soc_collection_rail_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    inst_soc_collection_rail_t rail_collection[MAX_NUM_OF_VR_RAILS];
} inst_soc_record_rail_t, *p_inst_soc_record_rail_t;

//----------------INST_TELEMETRY_ELEMENT_SOC_DIMM_RT----------------

typedef struct {
    uint8_t memory_freq_id;
    uint8_t reserved;
    uint8_t throttling_flags;
    uint16_t residency_mS;
    uint16_t temp_s0_latest_dC;
    uint16_t temp_s1_latest_dC;
    uint16_t power_mW;
} inst_soc_element_dimm_runtime_t, *p_inst_soc_element_dimm_runtime_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    inst_soc_element_dimm_runtime_t dimm_rt_element;
} inst_soc_collection_dimm_runtime_t, *p_inst_soc_collection_dimm_runtime_t;


typedef struct {
    telemetry_record_hdr_t record_header;
    inst_soc_collection_dimm_runtime_t dimm_collection[NUMBER_OF_DIMM_MODULES];
} inst_soc_record_dimm_runtime_t, *p_inst_soc_record_dimm_runtime_t;

//----------------INST_TELEMETRY_ELEMENT_SOC_DIMM_CONFIG----------------

typedef struct {
    uint16_t temp_threshold_high_dC;
    uint16_t temp_threshold_low_dC;
    uint16_t temp_threshold_critical_dC;
} inst_soc_element_dimm_config_t, *p_inst_soc_element_dimm_config_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    inst_soc_element_dimm_config_t dimm_config_element;
} inst_soc_collection_dimm_config_t, *p_inst_soc_collection_dimm_config_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    inst_soc_collection_dimm_config_t dimm_config_collection;
} inst_soc_record_dimm_config_t, *p_inst_soc_record_dimm_config_t;

//----------------INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR----------------

typedef temperature_t inst_soc_element_sensor_temp_t, *p_inst_soc_element_sensor_temp_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    inst_soc_element_sensor_temp_t temperature_element;
} inst_soc_collection_sensor_temp_t, *p_inst_soc_collection_sensor_temp_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    inst_soc_collection_sensor_temp_t temperature_collection[NUMBER_OF_SOC_TEMP_SENSORS];
} inst_soc_record_sensor_temp_t, *p_inst_soc_record_sensor_temp_t;

//----------------INST_TELEMETRY_ELEMENT_AMU----------------
typedef struct {
    uint64_t amevcntr_00;
    uint64_t amevcntr_01;
    uint64_t amevcntr_02;
    uint64_t amevcntr_03;
    uint64_t amevcntr_10;
    uint64_t amevcntr_11;
    uint64_t amevcntr_12;
} inst_core_element_amu_counters_t, *p_inst_core_element_amu_counters_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    union {
        uint64_t counters[NUMBER_OF_AMU_COUNTERS];
        inst_core_element_amu_counters_t amu_counter_element;
    };
} inst_core_collection_amu_counters_t, *p_inst_core_collection_amu_counters_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    inst_core_collection_amu_counters_t amu_counter_collection[NUMBER_OF_CORES_PER_DIE];
} inst_core_record_amu_counters_t, *p_inst_core_record_amu_counters_t;

#pragma pack(pop)

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
