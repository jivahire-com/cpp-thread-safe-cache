//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_error_injection_i.h
 * Header file for PCIe error injection functions.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <cper.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief PCIe error injection callback function. Invoked by the health monitor
 *        to handle error injection requests for PCIe components.
 *
 * @param info Pointer to the PCIe error injection payload.
 * @param error_domain_context Pointer to the error domain context (not used).
 *
 * @return ACPI_EINJ_SUCCESS Error injected successfully.
 * @return ACPI_EINJ_INVALID_ACCESS Invalid structure metadata passed.
 */
acpi_einj_cmd_status_t pcie_error_injection_cb(ras_einj_info_t* einj_payload, void* error_domain_context);
