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
 * @brief exec on idle function pointer type
*/
typedef void (*power_exec_in_idle_handler_t)(PDFWK_ASYNC_REQUEST_HEADER, void *);  


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
    POWER_CTRL_LOOP_SIGNAL_EXCHANGE_INPUTS,
    POWER_CTRL_LOOP_SIGNAL_EXCHANGE_COMPLETE,
    POWER_CTRL_LOOP_SIGNAL_MAX,
} power_ctrl_loop_signal_t;

// -----------------------------------
// Telemetry Loop Specific Definitions
// -----------------------------------

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
 *  @brief Enum of PVT telemetry loop signals
 */
typedef enum _power_pvt_telem_signal_t
{
    POWER_PVT_TELEM_SIGNAL_ENTRY = POWER_LOOP_STATE_SIGNAL_ENTRY,  // initial entry into state
    POWER_PVT_TELEM_SIGNAL_INTERVAL = POWER_LOOP_STATE_SIGNAL_INTERVAL,
    POWER_PVT_TELEM_SIGNAL_MAX,
} power_pvt_telem_signal_t;


/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

void power_loops_init();
void power_loops_control_init();
void power_loops_control_post_core_init();
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

power_loop_context_t* get_s_loop_context(void);
power_ctrl_loop_detail_t* get_s_ctrl_loop(void);

power_loop_context_t* get_s_pvt_telem_loop_context(void);
power_loop_context_t* get_s_vr_telem_loop_context(void);
power_telem_loop_detail_t* get_s_telem_loop(void);


#ifdef __cplusplus
}
#endif
