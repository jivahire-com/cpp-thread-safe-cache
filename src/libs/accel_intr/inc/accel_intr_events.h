//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_events.h
 * This file defines the event trace events for Accel Interrupt service and client
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

#include "accel_intr_priv.h"

#include <event_trace.h>
#include <event_trace_providers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// clang-format off

/*  TODO ADO 3041451: See if the events can be optimized, club multiples events into one
*   with the event string also as parameter using FPFW_ET_ASCII_STRING
*/

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR, // ID
    SCP_ACCEL_INTR,                         // Name
    FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS              // Logging Level Mask
)

/**
 * Define Event Trace events for the SCP Main Provider
*/
typedef enum {
    SCP_ACCEL_EVENT_ID_INTERRUPT = 0,                           // Basic interrupt event. Prints Level1 bit index
    SCP_ACCEL_EVENT_ID_INTERRUPT_SOC_RESET,                     // Printed when going for SoC Reset
    SCP_ACCEL_EVENT_ID_INTERRUPT_EMCPU_RESET,                   // Printed when going for emCPU Reset
    SCP_ACCEL_EVENT_ID_INTERRUPT_CRASHDUMP_COLLECTION_TIMEOUT,  // Printed when timeout for Crash Dump Collection happens
    SCP_ACCEL_EVENT_ID_CD_COMPLETION_RETRY,                     // Printed when giving more time Crash Dump Collection of accel core
    SCP_ACCEL_EVENT_ID_CD_COMPLETION_FAILED,                    // Printed when Crash Dump Collection of accel core failed
    SCP_ACCEL_EVENT_ID_INTERRUPT_INVALID,                       // Printed when accel device receives an invalid interrupt
    SCP_ACCEL_EVENT_ID_CPER_COMPLETION_RETRY,                   // Printed when giving more time for CPER submission from accel core
    SCP_ACCEL_EVENT_ID_CPER_COMPLETION_FAILED                   // Printed when CPER submission from accel core failed
} SCP_ACCEL_EVENT_ID;


/**
 * This prints:
 * IRQnum (SDM / CDED)
 * Level 1 register bit index : level1
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT,
    AccelIntr,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, ISR_VAL),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, ISR_W_MASK_VAL)
)

/**
 * This prints:
 * Accel type (SDM / CDED)
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_SOC_RESET,
    AccelIntrSoCReset,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id)
)

/**
 * This prints:
 * Accel type (SDM / CDED)
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_EMCPU_RESET,
    AccelIntremCPUReset,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id)
)

/**
 * This prints:
 * Accel type (SDM / CDED)
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_CRASHDUMP_COLLECTION_TIMEOUT,
    AccelIntrCrashdumpCollectTimeout,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id)
)

/**
 * This prints:
 * Accel type (SDM / CDED)
 * Retry count for crash dump collection
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_CD_COMPLETION_RETRY,
    AccelIntrCrashdumpCollectRetry,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, retry_count)
)

/**
 * This prints:
 * Accel type (SDM / CDED)
 * Crash dump collection failure log
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_CD_COMPLETION_FAILED,
    AccelIntrCrashdumpCollectFailed,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id)
)

/**
 * This prints:
 * Accel type (SDM / CDED)
 * Retry count for crash dump collection
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_CPER_COMPLETION_RETRY,
    AccelIntrCPERCollectTimeout,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, retry_count)
)

/**
 * This prints:
 * Accel type (SDM / CDED)
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_CPER_COMPLETION_FAILED,
    AccelIntrCPERCollectFailed,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, accel_id)
)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
