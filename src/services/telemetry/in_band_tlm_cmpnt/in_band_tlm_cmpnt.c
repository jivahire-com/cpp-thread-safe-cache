//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file in_band_tlm_cmpnt.c
 * Primary in band telemetry processing
 */

/*------------- Includes -----------------*/
#include "in_band_tlm_cmpnt.h"

#include "dcs_manager_i.h"
#include "ddr_manager_i.h"
#include "element_schema.h"
#include "in_band_tlm_cmpnt_i.h"
#include "package_creation_i.h"
#include "telemetry_package_defs.h"

#include <FpFwAssert.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
uint8_t inband_die_id;
uint16_t inband_inst_samples_per_pkg;

/*------------- Functions ----------------*/

void in_band_tlm_cmpnt_init(uint8_t die_id, uint16_t inst_samples_per_pkg)
{
    inband_die_id = die_id;
    inband_inst_samples_per_pkg = inst_samples_per_pkg;

    FPFW_RUNTIME_ASSERT_EXT(inband_inst_samples_per_pkg <= MAX_INST_SAMPLES_PER_PACKAGE, inband_inst_samples_per_pkg, 0, 0, 0);

    ddr_manager_init();
    dcs_manager_init();

    // TODO: temporary until DCS is supported
    package_create_enable_disable_pwr_record(POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE, true);
    package_create_enable_disable_pwr_record(POWER_TELEMETRY_ELEMENT_CORE_CURRENT, true);

    // since the telemetry schema events are not called within code, the linker will optimize out
    // a single call is enough to anchor them
    FPFW_ET_LOG(pwr_core_element_voltage, 1, 2, 3, 4);
}

void in_band_tlm_cmpnt_add_inst_sample(void)
{
    static uintptr_t pkg_location = 0;
    static size_t pkg_used_size = 0;
    static size_t pkg_available_size = 0;
    static uint16_t sample_count = 0;
    static uint32_t inst_package_number = 1;

    if (sample_count++ == 0)
    {
        fpfw_status_t status = ddr_manager_allocate_mem_for_inst_pkg(&pkg_location, &pkg_available_size);

        if (FPFW_STATUS_FAILED(status))
        {
            // error already traced
            return;
        }
        p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)pkg_location;
        package_create_populate_hdr(package_hdr);
        package_hdr->payload_header.package_number = inst_package_number++;

        pkg_used_size = sizeof(telemetry_package_hdr_t);
    }

    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)pkg_location;

    pkg_used_size +=
        package_create_append_to_inst_pkg(pkg_location + pkg_used_size, pkg_available_size - pkg_used_size, package_hdr);

    if (sample_count == inband_inst_samples_per_pkg)
    {
        sample_count = 0;
        package_hdr->payload_header.package_payload_size = pkg_used_size - sizeof(telemetry_package_hdr_t);

        if (pkg_used_size > sizeof(telemetry_package_hdr_t))
        {
            dcs_manager_queue_tlm_package(pkg_location, pkg_used_size);
        }
        else
        {
            // no enabled elements
            ddr_manager_deallocate_mem(&pkg_location);
        }
    }
}

void in_band_tlm_cmpnt_generate_pwr_pkg(void)
{
    uintptr_t pkg_location;
    size_t pkg_available_size;
    uint32_t pkg_used_size;
    fpfw_status_t status = ddr_manager_allocate_mem_for_pwr_pkg(&pkg_location, &pkg_available_size);

    if (FPFW_STATUS_SUCCEEDED(status)) // failure already traced
    {
        pkg_used_size = package_create_power_pkg(pkg_location, pkg_available_size);
        if (pkg_used_size > 0)
        {
            dcs_manager_queue_tlm_package(pkg_location, pkg_used_size);
        }
        else
        {
            ddr_manager_deallocate_mem(&pkg_location);
        }
    }
}