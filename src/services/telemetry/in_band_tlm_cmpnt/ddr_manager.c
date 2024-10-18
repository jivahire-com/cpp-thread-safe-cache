//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager.c
 * Manages DDR buffer allocation and deallocation
 * for in-band telemetry component.
 */

/*------------- Includes -----------------*/

#include "in_band_tlm_cmpnt.h"
#include "in_band_tlm_cmpnt_i.h"
#include "telemetry_package_defs.h"

#include <fpfw_status.h>
#include <in_band_telemetry_ddr.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// NOTE: this is just a placeholder for the actual data locations.
// Data management will be handled by https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2025876

static power_report_full_package_t s_power_report_package;
static inst_report_full_package_t s_inst_report_package;

/*------------- Functions ----------------*/

fpfw_status_t ddr_manager_allocate_mem_for_pwr_report(uintptr_t* pkg_location, size_t* available_size)
{
    *pkg_location = (uintptr_t)&s_power_report_package;
    *available_size = sizeof(s_power_report_package);
    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t ddr_manager_allocate_mem_for_inst_report(uintptr_t* pkg_location, size_t* available_size)
{
    *pkg_location = (uintptr_t)&s_inst_report_package;
    *available_size = sizeof(s_inst_report_package);
    return FPFW_STATUS_SUCCESS;
}