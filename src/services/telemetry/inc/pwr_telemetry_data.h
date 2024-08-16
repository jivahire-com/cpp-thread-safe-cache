//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_telemetry_data.h
 * Header containing definitions for power telemetry data resources
 */

#pragma once

/*----------- Nested includes ------------*/

#include <stdint.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...

#ifndef NUMBER_OF_TILES_PER_DIE
#define NUMBER_OF_TILES_PER_DIE 34
#endif


#ifndef NUMBER_OF_CORES_PER_DIE
#define NUMBER_OF_CORES_PER_DIE (NUMBER_OF_TILES_PER_DIE*2)
#endif

#ifndef NUMBER_OF_HNF_CHANNELS_PER_DIE
#define NUMBER_OF_HNF_CHANNELS_PER_DIE (NUMBER_OF_TILES_PER_DIE*2)
#endif


#ifndef NUMBER_OF_PSTATES
#define NUMBER_OF_PSTATES 32
#endif

#ifndef NUMBER_OF_CSTATES
#define NUMBER_OF_CSTATES 5
#endif

#ifndef NUMBER_OF_CSTATES
#define NUMBER_OF_CSTATES 5
#endif


#define NUMBER_OF_THROTTLE_TYPES    7
#define NUMBER_OF_RACK_PRIORITIES   8

typedef enum
{
    NO_THROTTLING = 0,
    CURRENT_THROTTLING_START,
    TEMPERATURE_THROTTLING_START,
    RACK_LIMIT_THROTTLING_START,
    VR_HOT_THROTTLING_START,
    ADAPTIVE_CLOCKING_THROTTLING_START,
    CURRENT_THROTTLING_END,
    TEMPERATURE_THROTTLING_END,
    RACK_LIMIT_THROTTLING_END,
    VR_HOT_THROTTLING_END,
    ADAPTIVE_CLOCKING_THROTTLING_END,
} THROTTLING_STATUS;

typedef enum
{
    THROTTLING_EV_NONE = 0,
    THROTTLING_END,
    THROTTLING_START,
} THROTTLING_EVENT_TYPE;


typedef enum
{
    THROTTLING_TYPE_NONE = 0,
    THROTTLING_TYPE_CURRENT,
    THROTTLING_TYPE_TEMPERATURE,
    THROTTLING_TYPE_RACK_LIMIT,
    THROTTLING_TYPE_VR_HOT,
    THROTTLING_TYPE_ADAPTIVE_CLK,
} THROTTLING_TYPE_VAL;


/*-------------- Typedefs ----------------*/

/**
 *  @brief Core related runtime resources
 */

typedef struct {
    uint16_t max;
    uint16_t min;
    uint16_t average;
    union {
        uint16_t instantaneous;
        uint16_t new_value;
    };

} voltage_t, temperature_t, cstate_latency_t, mma16_t, current_t;

typedef struct {
    temperature_t temperature;
} pwr_soc_hnf_t;

typedef struct {
    uint8_t pstate_id;
    uint32_t residency;
    uint32_t entry_count;	
    uint16_t frequency;
    uint16_t max_power;
    uint16_t min_power;
    uint16_t avg_power;
} pwr_pstate_t;

typedef struct {
    uint8_t cstate_id;
    uint32_t residency;
    uint32_t entry_count;
    uint16_t max_power;
    uint16_t min_power;
    uint16_t avg_power;
} pwr_cstate_t;

typedef struct {
    uint8_t type_id;
    uint8_t max_pstate;
    uint8_t ave_pstate;
    uint8_t reserved;
    uint32_t entry_count;
    uint32_t exit_count;
    uint32_t residency;
} pwr_throttle_type_t;

typedef struct {
    uint8_t priority_id;
    uint8_t max_pstate;
    uint8_t ave_pstate;
    uint8_t reserved;
    uint32_t entry_count;
    uint32_t exit_count;
    uint32_t residency;
} pwr_rack_priorities_t;

typedef struct {
    voltage_t voltage;
    current_t current;
    temperature_t temperature;
} pwr_voltage_rail_t;
 
typedef struct {
    uint16_t pstate_change : 1;
    uint16_t cstate_change : 1;
    uint16_t throttling_ev_change : 1;
    uint16_t rack_priority_change : 1;
    uint16_t id_change_bit : 1;	
    uint8_t throttling_start : 1;
    uint8_t throttling_end : 1;	
} core_control_flags_t; 
 
typedef struct {
    core_control_flags_t flags;	
    uint64_t cstate_timestamp;
    uint64_t pstate_timestamp;	
    uint64_t throttle_timestamp;
    uint64_t current_pkt_timestamp;
    uint64_t throttling_counter;	
    uint8_t current_pstate;
    uint8_t current_cstate;
    uint8_t current_plimit;
    uint8_t ldo_voltage;
    uint8_t throttling_status;
    uint8_t throttling_event;
    uint8_t throttling_type;
    uint8_t throttling_priority_id;
    uint8_t current_tel_pstate;
    uint8_t current_power;
    uint8_t power_index;
    pwr_pstate_t pstate[NUMBER_OF_PSTATES];
    pwr_cstate_t cstate[NUMBER_OF_CSTATES];
    pwr_throttle_type_t throttle_info[NUMBER_OF_THROTTLE_TYPES];
    pwr_rack_priorities_t priorities[NUMBER_OF_RACK_PRIORITIES];	
    voltage_t voltage;
    current_t current;
    temperature_t temperature;
    uint16_t current_mpam_id;
    uint8_t nominal_pstate;	
} core_runtime_info_t;

typedef struct {
    uint16_t current_max_temperature;
    uint16_t max_tile_temperature;
    uint8_t current_max_id;
    uint8_t max_tile_id;
    voltage_t vcpu;
    voltage_t vsys;
} tile_runtime_info_t;

typedef struct {
    uint32_t soc_pc3_residency;
    uint32_t soc_pc4_residency;	
    pwr_voltage_rail_t rail[MAX_NUM_OF_VR_RAILS];
    pwr_soc_hnf_t hnf[NUMBER_OF_HNF_CHANNELS_PER_DIE];
} soc_runtime_info_t;

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif