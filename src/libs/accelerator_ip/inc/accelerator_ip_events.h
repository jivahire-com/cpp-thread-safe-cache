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

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
