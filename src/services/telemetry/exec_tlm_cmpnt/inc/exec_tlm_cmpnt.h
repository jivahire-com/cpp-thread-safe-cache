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
    TLM_OP_MODE_PUBLISHING = 0, // telemetry is publishing data and exchanging with the host
    TLM_OP_MODE_COLLECTING_DATA = 1, // hardware fifos are emptied and data is being collected
    TLM_OP_MODE_DISABLED = 2, // telemetry is disabled and no data is being collected, test mode
    TLM_OP_MODE_SENSOR_FIFO_RAW_DATA = 3, // telemetry is collecting raw data from the sensor fifos and packaging for host to read
} tlm_operating_mode_t;

typedef struct
{
    tlm_operating_mode_t op_mode;
    uint32_t data_aggr_period_ms;
    uint32_t inst_pkg_sample_period_ms;
    uint32_t pwr_pkg_period_ms;
    uint32_t twenty_four_hr_pkg_period_ms; // may be overridden for testing
    bool  data_aggr_timer_active;
    bool  inst_sample_timer_active;
    bool  pwr_pkg_timer_active;
    bool  twenty_four_hr_pkg_timer_active;
} telemetry_executive_status_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the telemetry executive component.
 * @param[in] die_id The ID of this die
 * param[in] pwr_pkg_period_ms The power package period in milliseconds.
 * param[in] inst_pkg_sample_period_ms The instantaneous package period in milliseconds.
 */
void exec_tlm_cmpnt_init(uint8_t die_id, uint32_t pwr_pkg_period_ms, uint32_t inst_pkg_sample_period_ms);

/**
 * @brief Check if the telemetry collection is enabled.
 *
 * @return true if enabled, false if disabled.
 */
bool exec_tlm_cmpnt_is_telemetry_publishing_enabled(void);

/**
 * @brief Change Telemetry operating mode
 * param[in] new_mode switch to this mode
 *
 * @return None
 */
void exec_tlm_cmpnt_change_telemetry_mode(tlm_operating_mode_t new_mode);

/**
 * @brief Used to change the telemetry mode from an external thread. Sets the pending mode and notifies
 * the telemetry executive component which will then process the mode change on the telemetry service thread.
 *
 * @param[in] pending_mode The new telemetry mode.
 *
 * @return None
 */
void exec_tlm_cmpnt_set_mode_change_pending(tlm_operating_mode_t pending_mode);

/**
 * @brief Get the status of the telemetry executive component.
 *
 * @param[out] status The status of the telemetry executive component.
 *
 * @return None
 */
void exec_tlm_cmpnt_get_status(telemetry_executive_status_t *status);

/**
 * @brief Notify the telemetry executive component that a new in-band MTS message has been received.
 *
 * @return None
 */
void exec_tlm_cmpnt_notify_new_in_band_mts_message(void);

/**
 * @brief The exec_tlm_get_timestamp_microseconds function retrieves the current timestamp in microseconds.
 * It calculates this timestamp based on the current tick count and the frequency of the timer.
 * @return uint64_t  The function returns a uint64_t value representing the current timestamp in microseconds.
 */
uint64_t exec_tlm_cmpnt_get_timestamp_microseconds(void);

/**
 * @brief Update the telemetry executive component with new timer periods.
 * param[in] data_aggr_period_ms The power aggregation period in milliseconds.
 * param[in] inst_pkg_sample_period_ms The instantaneous package sample period in milliseconds.
 * param[in] pwr_pkg_period_ms The power package period in milliseconds.
 * param[in] twenty_four_hr_pkg_period_ms The 24-hour package period in milliseconds.
 *
 * @return None
 */
void exec_tlm_cmpnt_udpdate_timer_periods(uint32_t data_aggr_period_ms, uint32_t inst_pkg_sample_period_ms, uint32_t pwr_pkg_period_ms, uint32_t twenty_four_hr_pkg_period_ms);
