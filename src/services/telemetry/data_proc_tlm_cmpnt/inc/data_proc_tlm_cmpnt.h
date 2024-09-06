//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file  data_proc_tlm_cmpnt.h
 * Internal public interface for the data processing component
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

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
