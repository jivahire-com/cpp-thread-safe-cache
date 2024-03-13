//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mcp_event_trace_collector.h
 *  This modules initializes and configures event tracing for the mcp Core.
 */

#pragma once

/*--------------- Includes ---------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 *  @brief      Used to initialize the Event Tracing Collector for the mcp Core. Handles mcp Specific collector needs.
 * 
 *  @retval     None. Errors raised if failure in initialization.
*/
void mcp_etc_initialize();
