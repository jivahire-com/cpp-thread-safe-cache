//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_providers.h
 * Defines event trace provider ids for providers on every core.
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

// Provider IDs are used for:
//   - Associating events with a Provider
//   - Associating an event level filter with a Provider

typedef enum
{
    EVENT_TRACE_PROVIDER_ID_COMMON_SENSOR_FIFO_SVC = 0x0400,
    EVENT_TRACE_PROVIDER_ID_COMMON_MAX
} EVENT_TRACE_PROVIDER_ID_COMMON;

typedef enum
{
    EVENT_TRACE_PROVIDER_ID_SCP_MAIN = 0x0100,
    EVENT_TRACE_PROVIDER_ID_SCP_FUSE = 0x0101,
    EVENT_TRACE_PROVIDER_ID_SCP_POWER = 0x0109,
    EVENT_TRACE_PROVIDER_ID_SCP_ACCEL_INTR = 0x010A,
    EVENT_TRACE_PROVIDER_ID_SCP_CLI_ACCEL_INT = 0x010B,

    EVENT_TRACE_PROVIDER_ID_SCP_SENSOR_FIFO_SVC = EVENT_TRACE_PROVIDER_ID_COMMON_SENSOR_FIFO_SVC,

    EVENT_TRACE_PROVIDER_ID_SCP_MAX
} EVENT_TRACE_PROVIDER_ID_SCP;

typedef enum
{
    EVENT_TRACE_PROVIDER_ID_MCP_MAIN = 0x0200,
    EVENT_TRACE_PROVIDER_ID_MCP_SENSOR_FIFO_SVC = EVENT_TRACE_PROVIDER_ID_COMMON_SENSOR_FIFO_SVC,

    EVENT_TRACE_PROVIDER_ID_MCP_MAX
} EVENT_TRACE_PROVIDER_ID_MCP;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
