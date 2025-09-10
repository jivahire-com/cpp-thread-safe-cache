//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file aging_counters_i.h
 *  header for aging counters telemetry
 */
#pragma once

/*----------- Nested includes ------------*/

#include "data_sampling_i.h" // internal APIs
#include "data_utilities_i.h"

#include <exec_tlm_cmpnt.h>
#include <fpfw_status.h>
#include <stdint.h>
#include <telemetry_package_defs.h>
/*-- Symbolic Constant Macros (defines) --*/
/*-------------- Typedefs ----------------*/
typedef struct {
    bool    measurement_armed; //true if armed to read the counter data 
    uint8_t measurement_index; //next counter id to read
} aging_counter_t;

typedef enum {
    /*Note : C2_PCM_AGING_SENSOR_SR  can have measurement_complete or measurement_not_complete*/
    MEASUREMENT_NOT_COMPLETE  = 0,
    MEASUREMENT_COMPLETE  = 1
} aging_sensor_status_t;

extern aging_counter_t core_aging[NUMBER_OF_CORES_PER_DIE]; 

/**
 * @brief Initialize the aging counters on telemetry service startup, armed the counters for all cores 
 *        first counters pairs. 
 * @return  None
 */
void aging_counter_init(void);

/**
 * @brief C2 PCM aging sensor measurement completion status.
 * Retrun C2 PCM aging sensor measurement completion status. When the measurement completes, the HW will also clear the
 * enable control.
 * @param[in]  core_id 
 * @return aging_sensor_status_t - C2 PCM aging sensor measurement complete status (1 - complete, 0 - not complete) 
 */
aging_sensor_status_t aging_counter_get_sensor_status(uint8_t core_id);

/**
 * @brief Enable measurement for C2 PCM counters.
 * @param[in]  core_id  
 * @param[in]  counter_id counter id to arm for measurement.
 * @return none
 */
void aging_counter_enable_sensor_measurement(uint8_t core_id, uint8_t counter_id);

/**
 * @brief  Read the PCM bank sensor data.
 * Read the PCM sensor data from the designated bank and counter.
 * @param[in] core_id 
 * @param[out]  counter_a bank_a_counter Pointer to the returned counter value of bank A if enabled
 * @param[out] counter_b ank_b_counter Pointer to the returned counter value of bank B if enabled
 * @return  true if silib return SILIBS_SUCCESS - The operation succeeded.
 */
bool aging_counter_read(uint8_t core_id, uint32_t *counter_a,uint32_t *counter_b);

/**
 * @brief reset all counter setting for all the cores.
 * @param none
 */
void aging_counter_reset(void);

