//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_events.h
 * This file defines the event trace events for Accel Interrupt service and client
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

#include <event_trace.h>
#include <event_trace_providers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR, // ID
    SCP_ACCEL,                              // Name
    ET_LEVEL_MASK_ALL                       // Logging Level Mask
)

/**
 * Define Event Trace events for the SCP Main Provider
*/
typedef enum {
    SCP_ACCEL_EVENT_ID_INTERRUPT = 0,                           // Basic interrupt event. Prints Level1 bit index
    SCP_ACCEL_EVENT_ID_INTERRUPT_LEVEL2,                        // Basic interrupt event. Prints Level1 bit index and Level 2 status
    SCP_ACCEL_EVENT_ID_INTERRUPT_CATEGORY_AND_LEVEL2,           // Basic interrupt event. Prints Level1 bit index, Category and Level 2 status (Used for UE_ECC)
    SCP_ACCEL_EVENT_ID_INTERRUPT_SOC_RESET,                     // Printed when going for SoC Reset
    SCP_ACCEL_EVENT_ID_INTERRUPT_EMCPU_RESET,                   // Printed when going for emCPU Reset
    SCP_ACCEL_EVENT_ID_INTERRUPT_CRASHDUMP_COLLECTION_TIMEOUT   // Printed when timeout for Crash Dump Collection happens
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
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, IRQnum),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, irq_bit)
)

/**
 * This prints:
 * IRQnum (SDM / CDED)
 * Level 1 register bit index : level1
 * Level 2 register value : level2
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_LEVEL2,
    AccelIntrWithLevel2Bits,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, IRQnum),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, irq_bit),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, intr_status)
)

/**
 * This prints:
 * IRQnum (SDM / CDED)
 * Level 1 register bit index : level1
 * Interrupt category : category (Used for UE_ECC (FAB vs TEL))
 * Level 2 register value : level2
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_CATEGORY_AND_LEVEL2,
    AccelIntrWithCategoryLevel2Bits,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, IRQnum),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, level1),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, category),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, level2)
)

/**
 * This prints:
 * IRQ Number (SDM / CDED)
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_SOC_RESET,
    AccelIntrSoCReset,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, IRQnum)
)

/**
 * This prints:
 * IRQ Number
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_EMCPU_RESET,
    AccelIntremCPUReset,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, IRQnum)
)

/**
 * This prints:
 * IRQ Number
 * Timeout Count
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_CRASHDUMP_COLLECTION_TIMEOUT,
    AccelIntrCrashdumpCollectTimeout,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, IRQnum),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, timeout_count)
)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
