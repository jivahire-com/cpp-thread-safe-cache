//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_cores_init.h
 * Header file to bring AP Core(s) out of RESET by SCP core.
 */

#pragma once

/*------------- Includes -----------------*/
#include <stdint.h>

/*------------- Typedefs -----------------*/
typedef enum {
    E_AP_CORES_INIT_STATUS_ATU_MAP_FAIL = (-2), // Total number of error types
    E_AP_CORES_INIT_STATUS_ATU_UNMAP_FAIL,
    E_AP_CORES_INIT_STATUS_SUCCESS = 0
} E_AP_CORES_INIT_STATUS_CODES;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/**
 * @brief Power ON the AP Core(s)
 *
 * \b Description:
 * Bring the AP Core(s) out of RESET i.e. Power ON
 *
 * @param[in] void
 *
 * @retval
 *  On success, E_AP_CORES_INIT_STATUS_SUCCESS
 *  On failure, relevant failure code
 */
int32_t scp_ap_cores_init(void);
