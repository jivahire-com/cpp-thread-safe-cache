/*
 * Copyright (c) Microsoft Corporation.
 *
 * Description:
 *     PID-based resource control implementation
 */

/*------------- Includes -----------------*/
#include "pid_resource.h"

#include "power_i.h"
#include "power_log.h" // for power_log_cores

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for FPFW_RUNTIME_ASSERT
#include <stdbool.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*------------- Globals -----------------*/
static pid_config_t pidcfg = {0};  // PID configuration
static pid_context_t pidctx = {0}; // PID error, accumulated integral, current resource count

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

void pid_reset()
{
    pidctx.prev_error = 0;
    pidctx.integral = 0;
}

void pid_init(const pid_config_t* config)
{
    FPFW_RUNTIME_ASSERT(config != NULL);
    memcpy(&pidcfg, config, sizeof(pidcfg));
}

void pid_get_config(pid_config_t* config)
{
    FPFW_RUNTIME_ASSERT(config != NULL);
    memcpy(config, &pidcfg, sizeof(pidcfg));
}

void pid_set_context(const pid_context_t* context)
{
    FPFW_RUNTIME_ASSERT(context != NULL);
    memcpy(&pidctx, context, sizeof(pidctx));
}

void pid_get_context(pid_context_t* context)
{
    FPFW_RUNTIME_ASSERT(context != NULL);
    memcpy(context, &pidctx, sizeof(pidctx));
}

void pid_update_setpoint(float setpoint)
{
    pidcfg.setpoint = setpoint;
}

void pid_set_resources(uint32_t resources)
{
    // adjust to internal pid max
    pidctx.available_resources = FPFW_MIN(resources, pidcfg.max);
}

uint32_t pid_calculate_resources(float delta_time, float measured_value)
{
    // subtract the offset
    const float setpoint = pidcfg.setpoint - pidcfg.setpoint_offset;
    // basically, we only want to calculate/accumulate error if there's a
    // change we can accomplish.
    const bool accumulate_error = (((setpoint > measured_value) && (pidcfg.max > pidctx.available_resources)) ||
                                   ((setpoint < measured_value) && (0 < pidctx.available_resources)));

    // calculate error
    const float error = accumulate_error ? (setpoint - measured_value) : 0;
    // then proportional, integral and derivative parts
    const float proportional = pidcfg.kp * error;
    pidctx.integral += pidcfg.ki * error * delta_time;
    const float derivative = pidcfg.kd * ((error - pidctx.prev_error) / delta_time);
    // store prev_error for next iteration
    pidctx.prev_error = error;

    // adjust "available resources", bound by 0 and pidcfg.max
    const float temp = (float)pidctx.available_resources + proportional + pidctx.integral + derivative;
    pidctx.available_resources = (temp > 0) ? (uint32_t)temp : 0;
    if (pidctx.available_resources > pidcfg.max)
    {
        pidctx.available_resources = pidcfg.max;
    }

    POWER_LOG_TRACE("PID: %d  %d  %d  %d  %d  %d\n",
                    (int)pidctx.available_resources,
                    (int)pidcfg.max,
                    (int)error,
                    (int)proportional,
                    (int)pidctx.integral,
                    (int)derivative);
    //! enable power log, log current PID state
    power_log_cores(&ALLCORES,
                    POWER_LOG_DATA(PID,
                                   {.error = error,
                                    .p = proportional,
                                    .i = pidctx.integral,
                                    .d = derivative,
                                    .available = ((uint32_t)pidctx.available_resources),
                                    .scaled = (uint32_t)temp}));

    // output current value
    return pidctx.available_resources;
}
