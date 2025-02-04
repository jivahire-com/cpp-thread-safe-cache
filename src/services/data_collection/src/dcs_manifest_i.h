//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcs_manifest_i.h
 * Api's and data related to manifest used to decode client data.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_status.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/


/**
 * @brief Get the manifest information.
 *
 * @param[out] manifest_offset Pointer to the manifest offset, from the telemetry base physical address.
 * @param[out] manifest_size Pointer to the total manifest size.
 *
 * @return FPFW_STATUS_SUCCESS on success, error code otherwise.
 */
fpfw_status_t dcs_get_manifest_info(uint64_t* manifest_offset, uint64_t* manifest_size);
