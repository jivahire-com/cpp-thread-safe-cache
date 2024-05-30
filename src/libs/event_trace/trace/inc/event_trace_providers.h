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

typedef enum {
    EVENT_TRACE_PROVIDER_ID_SCP_MAIN = 0x0100,
    EVENT_TRACE_PROVIDER_ID_SCP_ETC = 0x0101,
    EVENT_TRACE_PROVIDER_ID_SCP_POWER = 0x0109,

    EVENT_TRACE_PROVIDER_ID_SCP_MAX
} EVENT_TRACE_PROVIDER_ID_SCP;

typedef enum {
    EVENT_TRACE_PROVIDER_ID_MCP_MAIN = 0x0200,
    EVENT_TRACE_PROVIDER_ID_MCP_ETC = 0x0201,

    EVENT_TRACE_PROVIDER_ID_MCP_MAX
} EVENT_TRACE_PROVIDER_ID_MCP;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/