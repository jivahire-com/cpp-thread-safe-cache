//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file  data_proc_tlm_cmpnt.h
 * Internal public interface for the data processing component
 */

#pragma once

/*----------- Nested includes ------------*/
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

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the data processing component.
 */
void data_proc_tlm_cmpnt_init(void);

/**
 * @brief Primary power data collection entry point.  Read raw data from sensor fifo and update telemetry aggregation structures.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_aggregate_pwr_tlm_data(void);

/**
 * @brief Primary performance data collection entry point.  Read raw data from sensor fifo and/or registers and update telemetry aggregation structures.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_aggregate_perf_tlm_data(void);

/**
 * @brief Primary 24 hour data collection entry point.  Read raw data from sensor fifo and/or registers and update telemetry aggregation structures.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_aggregate_24hr_tlm_data(void);
