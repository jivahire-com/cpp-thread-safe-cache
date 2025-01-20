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
    SCP_ACCEL_EVENT_ID_INTERRUPT_LEVEL2_CDED_CP_FATAL,          // Basic interrupt event. Prints Level1 bit index and Level 2 status
    SCP_ACCEL_EVENT_ID_INTERRUPT_LEVEL2_WARN,                   // Basic interrupt event. Prints Level1 bit index and Level 2 status
    SCP_ACCEL_EVENT_ID_INTERRUPT_CATEGORY_AND_LEVEL2,           // Basic interrupt event. Prints Level1 bit index, Category and Level 2 status (Used for UE_ECC)
    SCP_ACCEL_EVENT_ID_INTERRUPT_CCMP_LEVEL3_FATAL,             // Compression engine fatal interrupt event. Prints Level2 bit index and Level 3 ISR
    SCP_ACCEL_EVENT_ID_INTERRUPT_CCMP_LEVEL3_WARN,              // Compression engine spurious interrupt event. Prints Level2 bit index and Level 3 ISR
    SCP_ACCEL_EVENT_ID_INTERRUPT_DCMP_LEVEL3_FATAL,             // Decompression engine fatal interrupt event. Prints Level2 bit index and Level 3 ISR
    SCP_ACCEL_EVENT_ID_INTERRUPT_DCMP_LEVEL3_WARN,              // Compression engine spurious interrupt event. Prints Level2 bit index and Level 3 ISR
    SCP_ACCEL_EVENT_ID_INTERRUPT_AES_LEVEL3_FATAL,              // AES interrupt engine fatal event. Prints Level2 bit index and Level 3 ISR
    SCP_ACCEL_EVENT_ID_INTERRUPT_AES_LEVEL3_WARN,               // Compression engine spurious interrupt event. Prints Level2 bit index and Level 3 ISR
    SCP_ACCEL_EVENT_ID_INTERRUPT_SOC_RESET,                     // Printed when going for SoC Reset
    SCP_ACCEL_EVENT_ID_INTERRUPT_EMCPU_RESET,                   // Printed when going for emCPU Reset
    SCP_ACCEL_EVENT_ID_INTERRUPT_CRASHDUMP_COLLECTION_TIMEOUT,  // Printed when timeout for Crash Dump Collection happens
    SCP_ACCEL_EVENT_ID_INTERRUPT_INVALID,                       // Printed when accel device receives an invalid interrupt
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
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_INVALID,
    AccelIntrInvalid,
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
 * Level 2 register value : level2
 * LEVEL2_INT : level2 reason for interrupt
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_LEVEL2_CDED_CP_FATAL,
    AccelIntrWithLevel2CPCDED,
    FPFW_ET_LEVEL_FATAL,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, IRQnum),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, intr_status),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(CDED_CP_INT_TRACE_STR_LEN), LEVEL2_INT)
)

/**
 * This prints:
 * IRQnum (SDM / CDED)
 * Level 1 register bit index : level1
 * Level 2 register value : level2
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_LEVEL2_WARN,
    AccelSpurIntrWithLevel2CPCDED,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, IRQnum),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, intr_status)
)

/**
 * This prints:
 * Compression engine level 3 ISR
 * Level 2 register bit index : level2
 * Level 3 register value : Compression engine [0-1] ISR value
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_CCMP_LEVEL3_FATAL,
    AccelIntrWithLevel3CCMP,
    FPFW_ET_LEVEL_FATAL,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, irq_bit),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, ccmp0_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, ccmp1_isr)
)

/**
 * This prints:
 * Compression engine level 3 ISR
 * Level 2 register bit index : level2
 * Level 3 register value : Compression engine [0-1] ISR value
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_CCMP_LEVEL3_WARN,
    AccelSpurIntrWithLevel3CCMP,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, irq_bit),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, ccmp0_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, ccmp1_isr)
)

/**
 * This prints:
 * Decompression engine level 3 ISR
 * Level 2 register bit index : level2
 * Level 3 register value : Decompression engine [0-7] ISR value
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_DCMP_LEVEL3_FATAL,
    AccelIntrWithLevel3DCMP,
    FPFW_ET_LEVEL_FATAL,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, irq_bit),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp0_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp1_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp2_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp3_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp4_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp5_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp6_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp7_isr)
)

/**
 * This prints:
 * Decompression engine level 3 ISR
 * Level 2 register bit index : level2
 * Level 3 register value : Decompression engine [0-7] ISR value
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_DCMP_LEVEL3_WARN,
    AccelSpurIntrWithLevel3DCMP,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, irq_bit),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp0_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp1_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp2_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp3_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp4_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp5_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp6_isr),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, dcmp7_isr)
)

/**
 * This prints:
 * AES engine level 3 ISR
 * Level 2 register bit index : level2
 * Level 3 register value : AES engine ISR value
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_AES_LEVEL3_FATAL,
    AccelIntrWithLevel3AES,
    FPFW_ET_LEVEL_FATAL,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, irq_bit),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, aes_isr)
)

/**
 * This prints:
 * AES engine level 3 ISR
 * Level 2 register bit index : level2
 * Level 3 register value : AES engine ISR value
 */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR,
    SCP_ACCEL_EVENT_ID_INTERRUPT_AES_LEVEL3_WARN,
    AccelSpurIntrWithLevel3AES,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, irq_bit),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, aes_isr)
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
