//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcs_manager_i.h
 * Manage data collection service interaction.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "ddr_manager_i.h"

#include <FpFwAssert.h>
#include <fpfw_status.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct
{
   uintptr_t pkg_location;
   size_t pkg_size;
} tlm_package_t;

/*-- Declarations (Statics and globals) --*/
extern tlm_package_t dcs_pkg_pending_buffer[MAX_PENDING_PACKAGES];
extern TX_QUEUE dcs_pkg_pending_queue;

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the data collection service manager.
 */
void dcs_manager_init(void);

/**
 * @brief Queue a telemetry package for eventual transmission to the host.
 *
 * @param[in] pkg_location - location of the package
 * @param[in] pkg_size - size of the package
 */
void dcs_manager_queue_tlm_package(uintptr_t pkg_location, size_t pkg_size);