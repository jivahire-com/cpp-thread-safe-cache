//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hw_semaphore.h
 * Header file of HW semaphore
 */
#pragma once

/*----------- Nested includes ------------*/

#include <kng_error.h>
#include <semaphore_lib.h>

/*-- Symbolic Constant Macros (defines) --*/

/**
 * Local semaphores in use between MCP and SCP.
 */
#define CRASH_DUMP_RMSS_EXP_SEMAPHORE_ID (SEM_ID_MSCP_EXP_0)
#define HEALTH_MON_RMSS_EXP_SEMAPHORE_ID (SEM_ID_MSCP_EXP_1)
#define STARTUP_SHUTDOWN_RMSS_EXP_SEMAPHORE_ID (SEM_ID_MSCP_EXP_2)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize HW semaphore
 * 
 * @return KNG_STATUS KNG_SUCCESS if successful, otherwise matched error code.
 */
KNG_STATUS init_hw_semaphore();