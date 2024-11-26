//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file exec_tlm_cmpnt.h
 * Internal public interface for the telemetry executive component
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef enum
{
    TLM_OP_MODE_NOMINAL = 0,
    TLM_OP_MODE_DISABLED = 1,
    TLM_OP_MODE_SENSOR_FIFO_RAW_DATA = 2,
} tlm_operating_mode_t;

typedef struct
{
    tlm_operating_mode_t op_mode;
    uint32_t pwr_pkg_period_ms;
    uint32_t inst_pkg_sample_period_ms;
    uint32_t pwr_aggr_period_ms;
    bool  pwr_pkg_timer_active;
    bool  inst_sample_timer_active;
    bool  pwr_aggr_timer_active;
    bool  twenty_four_hr_pkg_timer_active;
} telemetry_executive_status_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the telemetry executive component.
 * param[in] pwr_pkg_period_ms The power package period in milliseconds.
 * param[in] inst_pkg_sample_period_ms The instantaneous package period in milliseconds.
 */
void exec_tlm_cmpnt_init(uint32_t pwr_pkg_period_ms, uint32_t inst_pkg_sample_period_ms);

/**
 * @brief Disable power telemetry data collection. Used for testing purposes.
 *
 * @return None
 */
void exec_tlm_cmpnt_disable_data_collection(void);

/**
 * @brief Get the status of the telemetry executive component.
 *
 * @param[out] status The status of the telemetry executive component.
 *
 * @return None
 */
void exec_tlm_cmpnt_get_status(telemetry_executive_status_t *status);