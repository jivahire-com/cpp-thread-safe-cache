//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file out_of_band_tlm_cmpnt_i.h
 * This file contains the private interface for the out-of-band telemetry component.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_pldm_service.h>
#include <pldm_pdr.h>
/*-- Symbolic Constant Macros (defines) --*/
#define PRIMARY_DIE_ID (0)

#define NUM_PWR_TLM_PLDM_SENSORS (PLDM_SENSOR_ID_POWER_TLM_LAST - PLDM_SENSOR_ID_POWER_TLM_BASE - 1)

#define PWR_TLM_PDR_SENSOR_INDEX(sensor_id)  ((sensor_id) - PLDM_SENSOR_ID_POWER_TLM_BASE - 1)

#define RELEASE_PLDM_SERVICE (1 << 0)

/*-------------- Typedefs ----------------*/
typedef void (*pwr_tlm_numeric_sensor_get_handler_t)(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value);
/*-- Declarations (Statics and globals) --*/

extern pldm_numeric_sensor_context_t pwr_tlm_numeric_sensor_ctxts[NUM_PWR_TLM_PLDM_SENSORS];
extern pwr_tlm_numeric_sensor_get_handler_t pwr_tlm_numeric_sensor_get_handlers[NUM_PWR_TLM_PLDM_SENSORS];
extern pldm_numeric_sensor_context_t* active_sensor;
extern pwr_tlm_numeric_sensor_get_handler_t active_handler;
/*--------- Function Prototypes ----------*/


void pwr_tlm_oob_get_max_soc_temp(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value);
void pwr_tlm_oob_get_soc_pwr(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value);
void pwr_tlm_oob_get_max_dimm_temp(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value);
void pwr_tlm_oob_get_dimm_total_pwr(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value);
void pwr_tlm_oob_get_max_vr_temp(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value);
void on_pwr_tlm_numeric_sensor_get_ext_entry(pldm_numeric_sensor_context_t* p_sensor, void* p_context);