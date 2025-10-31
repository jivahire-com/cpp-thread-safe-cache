//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_package_defs.h
 * This file contains the definitions of structures used for packages for in band telemetry.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <IFpFwEventTracingBuffers.h>
#include <diag_decoder.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>



/*-- Symbolic Constant Macros (defines) --*/

#define NUMBER_OF_TILES_PER_DIE (34)
#define NUMBER_OF_CORES_PER_DIE (NUMBER_OF_TILES_PER_DIE*2)
#define NUMBER_OF_HNF_CHANNELS_PER_DIE (NUMBER_OF_TILES_PER_DIE*2)
#define NUMBER_OF_PSTATES (32)
#define NUMBER_OF_CSTATES (5)
#define NUMBER_OF_AGING_COUNTER_PAIRS (8)
#define NUMBER_OF_ACCEL_COUNTERS (100)

//current, temperature, RACK, VR HOT, ADP_CLK, currnt overrun, adp_clk overrun
#define NUMBER_OF_THROTTLE_SOURCES    (7)  //(5+2)
#define NUMBER_OF_RACK_THROTTLE_PRIORITIES   (8)
#define NUMBER_OF_HS_VOLTAGE_SCALES (20)
#define NUMBER_OF_HS_TEMP_SCALES    (15)

/*Note : total DIMM in SoC is 12, per Die 6 module*/
#define NUMBER_OF_DIMMS_PER_DIE      (6)
#define NUMBER_OF_DIMMS_PER_SOC (NUMBER_OF_DIMMS_PER_DIE * 2) // 2 dies per SOC
#define NUMBER_OF_MPAMS             (128)


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
    POWER_TELEMETRY_ELEMENT_CORE_POWER,
    POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM,
    POWER_TELEMETRY_ELEMENT_CORE_AGING,
    POWER_TELEMETRY_ELEMENT_CORE_DROOPS,
    POWER_TELEMETRY_ELEMENT_SOC_PKG_MON,
    POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS,
    POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE,
    POWER_TELEMETRY_ELEMENT_SOC_DIMM_POWER,
    POWER_TELEMETRY_ELEMENT_SOC_HNF_TEMP,
    POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP,
    POWER_TELEMETRY_ELEMENT_SOC_PER_DIE_MESH,
    POWER_TELEMETRY_ELEMENT_SOC_DIE_TO_DIE_LINK_STATE,
    POWER_TELEMETRY_ELEMENT_SOC_MAX_TEMPERATURE,
    POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_CORE_POWER,
    POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_THROTTLE,
    POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER,
    POWER_TELEMETRY_ELEMENT_SOC_MEMORY_THROTTLE,
    POWER_TELEMETRY_ELEMENT_ID_MAX
} pwr_telemetry_element_id_t;

typedef enum
{
    INST_TELEMETRY_ELEMENT_CORE,
    INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS,
    INST_TELEMETRY_ELEMENT_SOC_DIMM_RT,
    INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP,
    INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP,
    INST_TELEMETRY_ELEMENT_ID_MAX
} instantaneous_telemetry_element_id_t;

#pragma pack(push, 1)
typedef struct {
    FPFW_ET_MANIFEST_ID manifest_id;
    uint64_t timestamp_uS;  /* Timestamp when the package was created  */
    uint64_t timestamp_utc;
    uint32_t source_die;
    uint32_t package_number;
    uint32_t number_of_records;
    uint32_t package_payload_size;
} telemetry_payload_header_t;

typedef struct {
    diag_decoder_payload_header_t decoder_header;
    telemetry_payload_header_t payload_header;

} telemetry_package_hdr_t, *p_telemetry_package_hdr_t;

typedef struct {
    uint16_t provider_id;
    uint16_t element_id;
    uint16_t collection_id;
    uint16_t number_of_elements;
    uint32_t collection_payload_size;   // minus the collection header size
} telemetry_collection_hdr_t, *p_telemetry_collection_hdr_t;

typedef struct {
    uint64_t timestamp_uS; /* Timestamp when the record was created  */
    uint32_t record_number;
    uint32_t number_of_collections;
    uint32_t record_payload_size;  //  minus the record header size
} telemetry_record_hdr_t, *p_telemetry_record_hdr_t;

typedef struct {
    uint16_t max_mV;
    uint16_t min_mV;
    uint16_t average_mV;
} voltage_t;

typedef struct {
    uint16_t max_dC;
    uint16_t min_dC;
    uint16_t average_dC;
} temperature_t;

typedef struct {
    uint16_t max_mA;
    uint16_t min_mA;
    uint16_t average_mA;
} current_t;

typedef struct {
    uint32_t max_mA;
    uint32_t min_mA;
    uint32_t average_mA;
} large_current_t;

typedef struct {
    uint16_t max_mW;
    uint16_t min_mW;
    uint16_t average_mW;
} power_t;

typedef struct {
    uint32_t max_mW;
    uint32_t average_mW;
} total_power_t;

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
    uint32_t entry_count;
    uint32_t residency_mS;
    uint16_t overrun_count;
    uint8_t type_id;
    uint8_t max_pstate;
    uint8_t avg_pstate;
} pwr_core_element_throttle_t, *p_pwr_core_element_throttle_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_throttle_t throttle_element[NUMBER_OF_THROTTLE_SOURCES];
} pwr_core_collection_throttle_t, *p_pwr_core_collection_throttle_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_throttle_t throttle_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_throttle_t, *p_pwr_core_record_throttle_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES----------------

typedef struct {
    uint32_t entry_count;
    uint32_t residency_mS;
    uint8_t priority_id;
} pwr_core_element_rack_priorities_t, *p_pwr_core_element_rack_priorities_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_rack_priorities_t rack_priority_element[NUMBER_OF_RACK_THROTTLE_PRIORITIES];
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

//----------------POWER_TELEMETRY_ELEMENT_CORE_POWER----------------

typedef power_t pwr_core_element_power_t, *p_pwr_core_element_power_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_power_t power_element;
} pwr_core_collection_power_t, *p_pwr_core_collection_power_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_power_t power_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_power_t, *p_pwr_core_record_power_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM----------------

typedef struct {
    uint32_t bin_count;
    uint8_t voltage_band;
    uint8_t temperature_band;
} pwr_core_element_histogram_t, *p_pwr_core_element_histogram_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_histogram_t histogram_element[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES];
} pwr_core_collection_histogram_t, *p_pwr_core_collection_histogram_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_histogram_t histogram_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_histogram_t, *p_pwr_core_record_histogram_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_AGING----------------

typedef struct {
    uint64_t timestamp_uS;
    uint32_t unaged_counter;
    uint32_t aged_counter;
    uint16_t voltage_mV;
    uint16_t temperature_dC;
    uint8_t counter_id;
} pwr_core_element_aging_t, *p_pwr_core_element_aging_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_aging_t aging_element[NUMBER_OF_AGING_COUNTER_PAIRS];
} pwr_core_collection_aging_t, *p_pwr_core_collection_aging_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_aging_t aging_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_aging_t, *p_pwr_core_record_aging_t;

//----------------POWER_TELEMETRY_ELEMENT_CORE_DROOPS----------------

typedef struct {
    uint64_t droop_count;
    voltage_t ldo_output_voltage;
    voltage_t vcpu_input_voltage;
} pwr_core_element_droop_count_t, *p_pwr_core_element_droop_count_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_core_element_droop_count_t droop_count_element;
} pwr_core_collection_droop_count_t, *p_pwr_core_collection_droop_count_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_core_collection_droop_count_t droop_count_collection[NUMBER_OF_CORES_PER_DIE];
} pwr_core_record_droop_count_t, *p_pwr_core_record_droop_count_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_PKG_MON----------------

typedef struct {
    uint32_t pc3_duration_mS;
    uint32_t pc4_duration_mS;
} pwr_soc_element_pkg_monitor_t, *p_pwr_soc_element_pkg_monitor_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_pkg_monitor_t pkg_mon_element;
} pwr_soc_collection_pkg_monitor_t, *p_pwr_soc_collection_pkg_monitor_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_pkg_monitor_t pkg_mon_collection;
} pwr_soc_record_pkg_monitor_t, *p_pwr_soc_record_pkg_monitor_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS----------------

typedef struct {
    large_current_t current;
    voltage_t voltage;
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

//----------------POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE----------------

typedef struct {
    temperature_t s0;
    temperature_t s1;
    uint8_t dimm_id; // DIMM module ID for identification
} pwr_soc_element_dimm_temp_t, *p_pwr_soc_element_dimm_temp_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_dimm_temp_t dimm_element;
} pwr_soc_collection_dimm_temp_t, *p_pwr_soc_collection_dimm_temp_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_dimm_temp_t dimm_collection[NUMBER_OF_DIMMS_PER_DIE];
} pwr_soc_record_dimm_temp_t, *p_pwr_soc_record_dimm_temp_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_DIMM_POWER----------------

typedef struct {
    power_t power_mW;
    uint8_t dimm_id;
} pwr_soc_element_dimm_power_t, *p_pwr_soc_element_dimm_power_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_dimm_power_t dimm_element;
} pwr_soc_collection_dimm_power_t, *p_pwr_soc_collection_dimm_power_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_dimm_power_t dimm_collection[NUMBER_OF_DIMMS_PER_DIE];
} pwr_soc_record_dimm_power_t, *p_pwr_soc_record_dimm_power_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_HNF_TEMP----------------
typedef temperature_t pwr_soc_element_hnf_t, *p_pwr_soc_element_hnf_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_hnf_t hnf_element;
} pwr_soc_collection_hnf_t, *p_pwr_soc_collection_hnf_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_hnf_t hnf_collection[NUMBER_OF_HNF_CHANNELS_PER_DIE];
} pwr_soc_record_hnf_t, *p_pwr_soc_record_hnf_t;

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

//----------------POWER_TELEMETRY_ELEMENT_SOC_PER_DIE_MESH----------------

typedef struct {
    uint64_t m1_entry_count;
    uint64_t m2_entry_count;
    uint64_t m0_residency_mS;
    uint64_t m1_residency_mS;
    uint64_t m2_residency_mS;
    uint64_t delivery_perf_mS;
} pwr_soc_element_die_mesh_t, *p_pwr_soc_element_die_mesh_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_die_mesh_t die_mesh_element;
} pwr_soc_collection_die_mesh_t, *p_pwr_soc_collection_die_mesh_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_die_mesh_t die_mesh_collection;
} pwr_soc_record_die_mesh_t, *p_pwr_soc_record_die_mesh_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_DIE_TO_DIE_LINK_STATE----------------

typedef struct {
    uint64_t residency_count;
    uint64_t bw_transmit;
    uint64_t bw_receive;;
    uint8_t  link_id;
} pwr_soc_element_d2d_link_t, *p_pwr_soc_element_d2d_link_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_d2d_link_t d2d_link_element;
} pwr_soc_collection_d2d_link_t, *p_pwr_soc_collection_d2d_link_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_d2d_link_t d2d_link_collection;
} pwr_soc_record_d2d_link_t, *p_pwr_soc_record_d2d_link_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_MAX_TEMPERATURE----------------

typedef struct {
    uint16_t average_max_dC;
    uint16_t peak_max_dC;
} pwr_soc_element_max_soc_temp_t, *p_pwr_soc_element_max_soc_temp_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_max_soc_temp_t max_soc_temp_element;
} pwr_soc_collection_max_soc_temp_t, *p_pwr_soc_collection_max_soc_temp_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_max_soc_temp_t max_soc_temp_collection;
} pwr_soc_record_max_soc_temp_t, *p_pwr_soc_record_max_soc_temp_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_CORE_POWER----------------
typedef struct {
    uint32_t max_mW;
    uint32_t average_mW;
    uint8_t  mpam_id;
} pwr_soc_element_mpam_core_power_t, *p_pwr_soc_element_mpam_core_power_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_mpam_core_power_t mpam_core_power_element;
} pwr_soc_collection_mpam_core_power_t, *p_pwr_soc_collection_mpam_core_power_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_mpam_core_power_t mpam_core_power_collection[NUMBER_OF_MPAMS];
} pwr_soc_record_mpam_core_power_t, *p_pwr_soc_record_mpam_core_power_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_THROTTLE----------------

typedef struct {
    uint32_t throttle_duration_mS;
    uint16_t nominal_pstate_frequency_Mhz;
    uint16_t max_pstate_frequency_Mhz;
    uint16_t avg_pstate_frequency_Mhz;
    uint16_t throttle_extent_centipct;
    uint8_t  mpam_id;
} pwr_soc_element_mpam_throttle_t, *p_pwr_soc_element_mpam_throttle_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_mpam_throttle_t mpam_throttle_element;
} pwr_soc_collection_mpam_throttle_t, *p_pwr_soc_collection_mpam_throttle_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_mpam_throttle_t mpam_throttle_collection[NUMBER_OF_MPAMS];
} pwr_soc_record_mpam_throttle_t, *p_pwr_soc_record_mpam_throttle_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER----------------

typedef struct {
    uint32_t max_mW;
    uint32_t average_mW;
    uint8_t  mpam_id;
} pwr_soc_element_mpam_memory_power_t, *p_pwr_soc_element_mpam_memory_power_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_mpam_memory_power_t mpam_memory_power_element;
} pwr_soc_collection_mpam_memory_power_t, *p_pwr_soc_collection_mpam_memory_power_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_mpam_memory_power_t mpam_memory_power_collection[NUMBER_OF_MPAMS];
} pwr_soc_record_mpam_memory_power_t, *p_pwr_soc_record_mpam_memory_power_t;

//----------------POWER_TELEMETRY_ELEMENT_SOC_MEMORY_THROTTLE----------------

typedef struct {
    uint32_t total_duration_mS;
    uint32_t entry_counts;
    uint8_t throttle_source;  // dimm_throttle_source_t
    uint8_t dimm_id;
} pwr_soc_element_memory_throttle_t, *p_pwr_soc_element_memory_throttle_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    pwr_soc_element_memory_throttle_t memory_throttle_element;
} pwr_soc_collection_memory_throttle_t, *p_pwr_soc_collection_memory_throttle_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    pwr_soc_collection_memory_throttle_t memory_throttle_collection[NUMBER_OF_DIMMS_PER_DIE];
} pwr_soc_record_memory_throttle_t, *p_pwr_soc_record_memory_throttle_t;

//----------------INST_TELEMETRY_ELEMENT_CORE----------------

typedef struct {
    uint32_t cstate_entry_latency_uS;
    uint32_t cstate_exit_latency_uS;
    uint16_t voltage_mV;
    uint16_t current_mA;
    uint16_t temperature_dC;
    uint16_t power_mW;
    uint16_t frequency_Mhz;
    uint8_t pstate;
    uint8_t cstate;
    uint8_t plimit;
    uint8_t mpam_id;
    uint8_t velocity_boost_priority;
    uint8_t throttling_type;
    uint8_t throttling_rack_priority;
} inst_core_element_summary_t, *p_inst_core_element_summary_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    inst_core_element_summary_t summary_element;
} inst_core_collection_summary_t, *p_inst_core_collection_summary_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    inst_core_collection_summary_t inst_core_summary_collection[NUMBER_OF_CORES_PER_DIE];
} inst_core_record_summary_t, *p_inst_core_record_summary_t;

//----------------INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS----------------

typedef struct {
    uint32_t current_mA;
    uint16_t voltage_mV;
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
    uint16_t temperature_dC;
    uint16_t threshold_dC;
    uint16_t power_mW;
    uint8_t memory_freq_id;
    uint8_t throttle_source;
} inst_soc_element_dimm_runtime_t, *p_inst_soc_element_dimm_runtime_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    inst_soc_element_dimm_runtime_t dimm_rt_element;
} inst_soc_collection_dimm_runtime_t, *p_inst_soc_collection_dimm_runtime_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    inst_soc_collection_dimm_runtime_t dimm_collection[NUMBER_OF_DIMMS_PER_DIE];
} inst_soc_record_dimm_runtime_t, *p_inst_soc_record_dimm_runtime_t;

//----------------INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP----------------

typedef struct {
    uint16_t temperature_dC;
    uint8_t sensor_id; // Sensor ID for identification
} inst_soc_element_die_temp_t, *p_inst_soc_element_die_temp_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    inst_soc_element_die_temp_t temperature_element;
} inst_soc_collection_die_temp_t, *p_inst_soc_collection_die_temp_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    inst_soc_collection_die_temp_t temperature_collection[NUMBER_OF_SOC_TEMP_SENSORS];
} inst_soc_record_die_temp_t, *p_inst_soc_record_die_temp_t;

//----------------INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP----------------

typedef struct {
    uint16_t die0_max_temperature_dC;
    uint16_t die1_max_temperature_dC;
} inst_soc_element_max_temp_t, *p_inst_soc_element_max_temp_t;

typedef struct {
    telemetry_collection_hdr_t collection_header;
    inst_soc_element_max_temp_t temperature_element;
} inst_soc_collection_max_temp_t, *p_inst_soc_collection_max_temp_t;

typedef struct {
    telemetry_record_hdr_t record_header;
    inst_soc_collection_max_temp_t temperature_collection;
} inst_soc_record_max_temp_t, *p_inst_soc_record_max_temp_t;


#pragma pack(pop)

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
