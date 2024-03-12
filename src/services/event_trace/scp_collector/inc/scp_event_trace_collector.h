//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file scp_event_trace_collector.h
 *  This modules initializes and configures event tracing for the SCP Core.
 */

#pragma once

/*--------------- Includes ---------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 *  @brief      Used to initialize the Event Tracing Collector for the SCP Core. Handles SCP Specific collector needs.
 * 
 *  @retval     None. Errors raised if failure in initialization.
*/
void scp_etc_initialize();
