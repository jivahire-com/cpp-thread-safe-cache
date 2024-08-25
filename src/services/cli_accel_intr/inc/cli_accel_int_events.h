//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_accel_int_events.h
 * Defines ACCEL App Trace Provider and events for Accelerator Interrupt Handler CLI
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

#include <event_trace.h>
#include <event_trace_providers.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_CLI_ACCEL_INT,  // ID
    CLI_ACCEL_INT,                              // Name
    ET_LEVEL_MASK_ALL                           // Logging Level Mask
)

/**
 * Define Event Trace events for the Accelerator Interrupt Provider. 
 * This provider is used to trace everything related to Accelerator Interrupt Handler
*/
typedef enum {
    E_CLI_ACCEL_INT_SUCCESS = 0x0,    // When given command is successful, print reg address and value
    E_CLI_ACCEL_INT_INVALID_ADDR,     // Address given by user is invalid
    E_CLI_ACCEL_INT_INVALID_VALUE,    // Value given by user when executing wr/set/clr is invalid
    E_CLI_ACCEL_INT_FAILED            // Command execution has failed, this generally happens when register is not from available reg-set
} e_accel_interrupt_event_id_t;

/**
 * @brief When given command is successful, print reg address and value
 * */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_CLI_ACCEL_INT,              // Provider ID
    E_CLI_ACCEL_INT_SUCCESS,                                // Event ID
    REG_OPER_SUCCESS,                                       // Event Name
    FPFW_ET_LEVEL_INFO,                                     // Log Level
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, REG_ADDR),     // Variables to be printed
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, REG_VALUE)
)

/**
 * @brief Address given by user is invalid
 * */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_CLI_ACCEL_INT,      // Provider ID
    E_CLI_ACCEL_INT_INVALID_ADDR,                   // Event ID
    REG_OPER_INVALID_ADDR,                          // Event Name
    FPFW_ET_LEVEL_ERROR,                            // Log Level
    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, ERRNO) // Variables to be printed
)

/**
 * @brief Value given by user is invalid
 * */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_CLI_ACCEL_INT,      // Provider ID
    E_CLI_ACCEL_INT_INVALID_VALUE,                  // Event ID
    REG_OPER_INVALID_VALUE,                         // Event Name
    FPFW_ET_LEVEL_ERROR,                            // Log Level
    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, ERRNO) // Variables to be printed
)

/**
 * @brief Command execution has failed, this generally happens when register is not from available reg-set
 * */
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_CLI_ACCEL_INT,            // Provider ID
    E_CLI_ACCEL_INT_FAILED,                               // Event ID
    REG_OPER_FAILED,                                      // Event Name
    FPFW_ET_LEVEL_ERROR,                                  // Log Level
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, OPERATION),  // Variables to be printed
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, REG_ADDR),     
    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, ERRNO)         
)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
