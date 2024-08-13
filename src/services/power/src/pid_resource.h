//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pid_resource.h
 * Header file for PID-based resource control
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct _pid_config_t {
    float kp; /* proportional coefficient */
    float ki; /* integral coefficient */
    float kd; /* derivative coefficient */
    float setpoint;
    float setpoint_offset; /* used to reduce the setpoint to give a buffer, if
                              necessary */
    uint32_t max;          /* PID-managed resource count/value will be between 0 and max */
} pid_config_t;

typedef struct _pid_context_t {
    float prev_error;              /* error from previous PID iteration */
    float integral;                /* accumulated integral value */
    uint32_t available_resources;  /* resource count managed by PID */
} pid_context_t;

/*--------- Function Prototypes ----------*/

/**
 * @brief Configure the PID details
 *
 * \b Description:
 *      Configures/initializes the PID control
 *
 * @param[in] config - Pointer to pid_config structure
 *
 */
void pid_init(const pid_config_t *config);

/**
 * @brief Get the PID configuration
 *
 * \b Description:
 *      Retrieves the configuration of the PID control - primarily for test
 * purposes
 *
 * @param[in] config - Pointer to pid_config structure to fill with config data
 *
 */
void pid_get_config(pid_config_t *config);

/**
 * @brief Set the PID context
 *
 * \b Description:
 *      Sets the context of the PID control - necessary to ensure synchronization 
 * of PID state across dies
 *
 * @param[in] context - Pointer to pid_context structure to replace internal context
 *
 */
void pid_set_context(const pid_context_t *context);

/**
 * @brief Get the PID context
 *
 * \b Description:
 *      Retrieves the context of the PID control - necessary to ensure synchronization 
 * of PID state across dies
 *
 * @param[in] context - Pointer to pid_config structure to fill with config data
 *
 */
void pid_get_context(pid_context_t *context);

/**
 * @brief Reset PID integral, error.  Useful on setpoint changes.
 *
 * \b Description:
 *      Resets components of the PID
 *
 */
void pid_reset();

/**
 * @brief Initialize the managed resource count
 *
 * \b Description:
 *      Provide a starting value for the count of resources the PID is managing
 *
 * @param[in] resources - Update/initialize resource count
 */
void pid_set_resources(uint32_t resources);

/**
 * @brief Update the setpoint the PID is managing to
 *
 * \b Description:
 *      This updates the PID setpoint
 *
 * @param[in] setpoint - new setpoint
 *
 */
void pid_update_setpoint(float setpoint);

/**
 * @brief Runs a PID iteration
 *
 * \b Description:
 *      Runs one iteration of the PID and returns an updated count of resources
 * scaled to scaled_max config parameter
 *
 * @param[in] delta_time - Time that has passed in seconds since last iteration
 * @param[in] measured_value - The value against which error from the setpoint
 * is calculated
 *
 * @return Returns an @c uint32_t corresponding to the updated count of resource
 * the PID is managing (between 0 and max)
 *
 */
uint32_t pid_calculate_resources(float delta_time, float measured_value);