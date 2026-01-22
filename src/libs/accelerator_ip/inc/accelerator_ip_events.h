//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_events.h
 * This file defines the event trace events for Accel Lib and client
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

#include "accelerator_ip_priv.h"

#include <event_trace.h>
#include <event_trace_providers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_IP,  // ID
    SCP_ACCEL_IP,                          // Name
    ET_LEVEL_MASK_ALL                      // Logging Level Mask
)

/**
 * Define Event Trace events for the ACCEL IP Provider
*/
typedef enum {
    ACCEL_IP_EVENT_ID_BOOT_TIME = 0,                           // Event for Accel boot time in microseconds
    ACCEL_IP_EVENT_ID_IP_INIT_ERROR,                           // Printed when an error occurs in file accelerator_ip_init.c
    ACCEL_IP_EVENT_ID_IP_MCP_ERROR,                            // Printed when an error occurs in file accelerator_ip_mcp.c
    ACCEL_IP_EVENT_ID_IP_KNOBS_ERROR,                          // Printed when an error occurs in file accelerator_ip_knobs.c
    ACCEL_IP_EVENT_ID_KNOBS_TX_FAILED,
} ACCEL_IP_EVENT_ID;

/**
 * This prints:
 * Boot time in microseconds
 * Accel type (SDM / CDED)
 * Die ID
 * Valid flag (1: boot time within threshold, 0: boot time exceeded threshold)
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_IP,
    ACCEL_IP_EVENT_ID_BOOT_TIME,
    AccelBootTime,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, boot_time_us),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, die_id),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, valid_flag)
)

/**
 * Event indicating an error in the accel IP init file
 * accel_id - Accel type (SDM / CDED)
 * status - error code
 * line - Line number where the error occurred in this file
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_IP,
    ACCEL_IP_EVENT_ID_IP_INIT_ERROR,
    AccelIPInitError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

/**
 * Event indicating an error in the accel IP MCP file
 * accel_id - Accel type (SDM / CDED)
 * status - error code
 * line - Line number where the error occurred in this file
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_IP,
    ACCEL_IP_EVENT_ID_IP_MCP_ERROR,
    AccelIPMCPError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

/**
 * Event indicating an error in the accel IP knobs file
 * accel_id - Accel type (SDM / CDED)
 * status - error code
 * line - Line number where the error occurred in this file
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_IP,
    ACCEL_IP_EVENT_ID_IP_KNOBS_ERROR,
    AccelIPKnobsError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

/**
 * Event indicating an error in the knobs transfer
 * accel_id - Accel type (SDM / CDED)
 * status - error code
 * knob_index - Index of the knob where the error occurred while transfer
 * line - Line number where the error occurred in this file
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_IP,
    ACCEL_IP_EVENT_ID_KNOBS_TX_FAILED,
    AccelIPKnobsTXError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, knob_index),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
