//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file in_band_tlm_cmpnt.c
 * Primary in band telemetry processing
 */

/*------------- Includes -----------------*/
#include "in_band_tlm_cmpnt.h"

#include "in_band_tlm_cmpnt_i.h"
#include "telemetry_package_defs.h"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void in_band_tlm_cmpnt_init(void)
{
}

void in_band_tlm_cmpnt_generate_perf_report(void)
{
    // TODOs will be handled by tasks
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2023646
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2025876

    uintptr_t pkg_location;
    size_t pkg_available_size;
    fpfw_status_t status = ddr_manager_allocate_mem_for_perf_report(&pkg_location, &pkg_available_size);

    if (FPFW_STATUS_SUCCEEDED(status))
    {
        status = package_create_perf_report(pkg_location, pkg_available_size);
        if (status == FPFW_STATUS_BUFFER_TOO_SMALL)
        {
            // TODO: send this package allocate a new package and call again
        }
    }
    else
    {
        // TODO:handle error
    }
}

void in_band_tlm_cmpnt_generate_pwr_report(void)
{
    // TODOs will be handled by tasks
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2023646
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2025876

    uintptr_t pkg_location;
    size_t pkg_available_size;
    fpfw_status_t status = ddr_manager_allocate_mem_for_pwr_report(&pkg_location, &pkg_available_size);

    if (FPFW_STATUS_SUCCEEDED(status))
    {
        status = package_create_power_report(pkg_location, pkg_available_size);
        if (status == FPFW_STATUS_BUFFER_TOO_SMALL)
        {
            // TODO: send this package allocate a new package and call again
        }
    }
    else
    {
        // TODO:handle error
    }
}