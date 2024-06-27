//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file interrupts.h
*/

#pragma once

/*--------------- Includes ---------------*/

/*------------- Typedefs -----------------*/
 typedef enum _MCP_IRQn_t {
    #include <kng_mcp_ints.h>
} MCP_IRQn_t;
