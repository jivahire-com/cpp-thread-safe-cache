//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_report_cper.c
 * Implements CPER related functionality.
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <cper.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <mscp_exp_rmss_memory_map.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void hm_submit_cper(uint16_t error_domain_idx,
                    acpi_error_severity_t err_severity,
                    acpi_cper_section_t* err_record_section,
                    uint32_t err_record_section_size)
{
    if ((error_domain_idx < ACPI_ERROR_DOMAIN_COUNT) && (err_record_section_size <= sizeof(acpi_cper_section_t)) &&
        err_record_section != NULL && err_record_section_size > 0)
    {
        // if this is MSCP error domain, generate full CPER record so that HSP can process it
        if (error_domain_idx == ACPI_ERROR_DOMAIN_MCP_PROC || error_domain_idx == ACPI_ERROR_DOMAIN_SCP_PROC)
        {
            acpi_cper_record_t cper_record = {0};
            create_full_mscp_cper_record(error_domain_idx, err_severity, err_record_section, err_record_section_size, &cper_record);

            volatile uint8_t* rmss_cper_record_base = (volatile uint8_t*)(uintptr_t)get_hm_config()->mscp_full_cper_record_base;
            BUG_ASSERT_PARAM(rmss_cper_record_base != NULL, rmss_cper_record_base, 0);

            static_assert(sizeof(acpi_cper_record_t) <= SCP_EXP_MSCP_CPER_REPORT_SIZE,
                          "acpi_cper_record_t > SCP_EXP_MSCP_CPER_REPORT_SIZE");

            for (uint32_t i = 0; i < sizeof(acpi_cper_record_t); ++i)
            {
                rmss_cper_record_base[i] = ((uint8_t*)&cper_record)[i];
            }

            HM_LOG_INFO("Full CPER record created for (%s)", get_error_domain_name(error_domain_idx));
        }

        update_error_record_section(error_domain_idx, err_severity, err_record_section, err_record_section_size);

        if (err_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
        {
            hm_report_error_event(HM_ERROR_REPORT_GPIO, true);
        }

        if ((err_severity == ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL) ||
            (err_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL))
        {
            hm_report_error_event(HM_ERROR_REPORT_VARSVC, true);
        }
    }
    else
    {
        BUG_ASSERT_PARAM(false, error_domain_idx, err_record_section_size);
    }
}
