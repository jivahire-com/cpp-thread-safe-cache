//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_loops_i.h
 * Header containing internal definitions for power loops
 */

#pragma once

/*----------- Nested includes ------------*/
#include "power_i.h"

#include <DfwkCommon.h>
#include <dvfs_struct.h> // for NUM_PSTATES
#include <fpfw_status.h>       // for FPFW_STATUS_SUCCESS, FPFW_STATUS_INVA...
#include <kng_error.h>
#include <power_runconfig.h>
#include <power_runconfig_i.h>
#include <FpFwAssert.h>
#include <stdbool.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

// common values for enums
#define POWER_LOOP_IDLE_STATE_ID 0
#define POWER_LOOP_STATE_SIGNAL_ENTRY 0
#define POWER_LOOP_STATE_SIGNAL_INTERVAL 1
// retries 
#define POWER_LOOP_RETRY_COUNT 10


/* Macros to convert between plimit and resource count */
#define PLIMIT_TO_RESOURCES(x) (MAX_PLIMIT - (x))  // for now, these are 1:1, reversed
#define RESOURCES_TO_PLIMIT(x) (MAX_PLIMIT - (x))

/*-------------- Typedefs ----------------*/
/**
 * @brief enum for queue entry
 */
typedef enum _power_loops_queue_entry_type_t
{
    LOOP_QUEUE_ENTRY_TYPE_STATE_CHANGE,
    LOOP_QUEUE_ENTRY_TYPE_STATE_SIGNAL,
    LOOP_QUEUE_ENTRY_TYPE_EXEC_IN_IDLE,
} power_loops_queue_entry_type_t;

/**
 * @brief enum of loop state retry types
 */
typedef enum _power_loop_retries_t
{
    POWER_LOOP_RETRY_TYPE_INTERVAL,
    POWER_LOOP_RETRY_TYPE_STATE,
    POWER_LOOP_RETRY_TYPE_COUNT
} power_loop_retries_t;

/**
 * @brief enum of loop IDs
 */
typedef enum _power_loop_id_t
{
    LOOP_ID_CONTROL,
    LOOP_ID_VR_TELEM,
    LOOP_ID_PVT_TELEM,
    LOOP_ID_COUNT
} power_loop_id_t;

/**
 * @brief common power state handler function pointer type
 */
typedef void (*power_state_handler_t)(int, const void *);  // state handler function pointer type

/**
 * @brief exec on idle function pointer type
 */
typedef void (*power_exec_in_idle_handler_t)(PDFWK_ASYNC_REQUEST_HEADER, void *);  

/**
 *  @brief Structure for basic loop state
 */
typedef struct _power_loop_state_t {
    int current_state;
    int last_state;
    int last_event;
    bool interval_in_flight;
    uint16_t retries[POWER_LOOP_RETRY_TYPE_COUNT];
} power_loop_state_t;

/**
 *  @brief Structure for loop residency detail
 */
typedef struct _power_loop_residency_t {
    uint64_t count;        // count of total state exits
    uint64_t ticks;        // number of clock ticks in state
    uint64_t last_entry;   // timestamp of last state entry
    uint64_t retries;      // total retries for this state
    uint16_t max_retries;  // max retries any transition
} power_loop_residency_t;

/**
 * @brief Struct for loop context, passed to common loop functions
 */
typedef struct _power_loop_context_t {
    unsigned int state_count;  // number of states in the loop
    power_loop_state_t status; // current loop status
    const power_state_handler_t *handlers; // state handler function pointer table
    power_loop_residency_t *residency; // state residency tracking
    power_loop_id_t id; // loop ID
} power_loop_context_t;

/**
 * @brief struct for state change queue entry data
 */
typedef struct _power_loop_change_entry_t {
    power_loop_id_t id; // loop ID
    power_loop_context_t* p_context; // loop context
    int state; // new state
} power_loop_change_entry_t;

/**
 * @brief struct for state signal queue entry data
 */
typedef struct _power_loop_signal_entry_t {
    power_loop_context_t* p_context; // loop context
    int event; // signal / event
    const void *event_data; // event data
} power_loop_signal_entry_t;


/**
 * @brief struct for exec in idle queue entry data
 */
typedef struct _power_loop_exec_in_idle_entry_t {
    power_exec_in_idle_handler_t p_handler; // handler function
    PDFWK_ASYNC_REQUEST_HEADER p_request; // pointer to request
    void *p_context; // exec context
} power_loop_exec_in_idle_entry_t;

/**
 * @brief struct for queue entry
 */
typedef struct
{
    power_loops_queue_entry_type_t type; // type of entry
    union
    {
        power_loop_change_entry_t state_change;
        power_loop_signal_entry_t state_signal;
        power_loop_exec_in_idle_entry_t exec_in_idle;
    } data;
} power_loops_queue_entry_t;


// ---------------------------------
// Control Loop Specific Definitions
// ---------------------------------


/**
 * @brief enum of control loop states
 */
typedef enum _power_ctrl_loop_state_t
{
    POWER_CONTROL_STATE_IDLE = POWER_LOOP_IDLE_STATE_ID,
    POWER_CONTROL_STATE_WARMSTART_ENTRY,
    POWER_CONTROL_STATE_COLLECT_INPUTS,
    POWER_CONTROL_STATE_EXCHANGE_INPUTS,
    POWER_CONTROL_STATE_DISTRIBUTE_AVAILABLE,
    POWER_CONTROL_STATE_SET_PLIMIT_BEFORE_VR,
    POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT,
    POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT,
    POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT,
    POWER_CONTROL_STATE_WAIT_VR_BEFORE_PLIMIT,
    POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR,
    POWER_CONTROL_STATE_EXCHANGE_COMPLETION,
    POWER_CONTROL_STATE_ERROR,
    POWER_CONTROL_STATE_MAX,
} power_ctrl_loop_state_t;

/**
 * @brief enum of control loop signals
 */
typedef enum _power_ctrl_loop_signal_t
{
    POWER_CTRL_LOOP_SIGNAL_ENTRY = POWER_LOOP_STATE_SIGNAL_ENTRY,  // initial entry into state
    POWER_CTRL_LOOP_SIGNAL_INTERVAL = POWER_LOOP_STATE_SIGNAL_INTERVAL,
    POWER_CTRL_LOOP_SIGNAL_VR_READ,
    POWER_CTRL_LOOP_SIGNAL_PLIMIT_PENDING,
    POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING,
    POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL,
    POWER_CTRL_LOOP_SIGNAL_VCPU_DONE,
    POWER_CTRL_LOOP_SIGNAL_MAX,
} power_ctrl_loop_signal_t;

// -----------------------------------
// Telemetry Loop Specific Definitions
// -----------------------------------

/**
 *  @brief Enum of VR telemetry states
 */
typedef enum _power_vr_telem_state_t
{
    POWER_VR_TELEM_STATE_IDLE = POWER_LOOP_IDLE_STATE_ID,
    POWER_VR_TELEM_STATE_CURRENT_TELEMETRY,
    POWER_VR_TELEM_STATE_TEMP_TELEMETRY,
    POWER_VR_TELEM_STATE_ERROR,
    POWER_VR_TELEM_STATE_MAX,
} power_vr_telem_state_t;

/**
 *  @brief Enum of VR telemetry loop signals
 */
typedef enum _power_vr_telem_signal_t
{
    POWER_VR_TELEM_SIGNAL_ENTRY = POWER_LOOP_STATE_SIGNAL_ENTRY,  // initial entry into state
    POWER_VR_TELEM_SIGNAL_INTERVAL = POWER_LOOP_STATE_SIGNAL_INTERVAL,
    POWER_VR_TELEM_SIGNAL_VR_CURRENT,
    POWER_VR_TELEM_SIGNAL_VR_CURRENT_FAIL,
    POWER_VR_TELEM_SIGNAL_VR_TEMP,
    POWER_VR_TELEM_SIGNAL_MAX,
} power_vr_telem_signal_t;

/**
 *  @brief Enum of PVT telemetry states
 *
 *  PVT loop is simpler; no events other than interval -- read_pvt does not
 * require a signal/event to return to idle, so no chance for entry into an
 * error state. Keeping the same basic structure to allow for timestamping, etc.
 */
typedef enum _power_pvt_telem_state_t
{
    POWER_PVT_TELEM_STATE_IDLE = POWER_LOOP_IDLE_STATE_ID,
    POWER_PVT_TELEM_STATE_READ_PVT,
    POWER_PVT_TELEM_STATE_MAX,
} power_pvt_telem_state_t;

/**
 *  @brief Enum of PVT telemetry loop signals
 */
typedef enum _power_pvt_telem_signal_t
{
    POWER_PVT_TELEM_SIGNAL_ENTRY = POWER_LOOP_STATE_SIGNAL_ENTRY,  // initial entry into state
    POWER_PVT_TELEM_SIGNAL_INTERVAL = POWER_LOOP_STATE_SIGNAL_INTERVAL,
    POWER_PVT_TELEM_SIGNAL_MAX,
} power_pvt_telem_signal_t;

/**
 *  @brief Structure for PVT state
 */
typedef struct _power_telemetry_pvt_state_t {
    uint64_t valid_samples;    // count of valid samples
    uint16_t last_sample;      // last sample value (converted from raw)
    uint16_t last_sample_raw;  // last sample value
    uint16_t sample_hi;        // highest sample value (converted)
    uint16_t sample_lo;        // lowest sample value (converted)
} power_telemetry_pvt_state_t;

/**
 *  @brief Struct for telemetry loops state
 */
typedef struct _power_telem_loop_detail_t {
    power_telemetry_pvt_state_t soc_top_temp_data[SOC_PVT_TOTAL_CHANNELS_DTS];
    power_telemetry_pvt_state_t soc_top_voltage_data[SOC_PVT_TOTAL_CHANNELS_VM];
    uint16_t soc_max_temp_dC;    // soc max temp in 0.1C
    power_vrs_avs_latest_t* vr_data;
} power_telem_loop_detail_t;

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

void power_loops_init();
void power_loops_control_init();
void power_loops_telemetry_init();

void power_loops_init_loop(power_loop_context_t * context);
void power_loops_exec_in_idle(power_exec_in_idle_handler_t p_handler, PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context);
void power_loops_handle_event(power_loop_context_t* context, int event, const void* event_data);
void power_loops_change_state(power_loop_context_t* context, int state);
bool power_loops_retry_fail(power_loop_context_t *context, power_loop_retries_t type);

void power_loops_control_handle_event(power_ctrl_loop_signal_t event, const void* event_data); 
void power_loops_vr_telem_handle_event(power_vr_telem_signal_t event, const void* event_data);
void power_loops_pvt_telem_handle_event(power_pvt_telem_signal_t event, const void* event_data);
power_ctrl_loop_detail_t* power_ctrl_loop_get();

#ifdef __cplusplus
}
#endif
