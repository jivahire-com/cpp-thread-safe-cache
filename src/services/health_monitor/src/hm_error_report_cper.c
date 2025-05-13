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
#include <health_monitor_i.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void update_local_error_record(uint32_t sec_idx,
                                      uint16_t error_domain_idx,
                                      acpi_error_severity_t err_severity,
                                      acpi_cper_section_t* err_record_section,
                                      uint32_t err_record_section_size);

/*-- Declarations (Statics and globals) --*/
static hm_error_record_t error_record_sections[MAX_CPER_CACHE] = {0};
static uint32_t local_cper_count = 0;
/*------------- Functions ----------------*/

void hm_submit_cper(uint16_t error_domain_idx,
                    acpi_error_severity_t err_severity,
                    acpi_cper_section_t* err_record_section,
                    uint32_t err_record_section_size)
{
    if ((error_domain_idx < ACPI_ERROR_DOMAIN_COUNT) && (err_record_section_size <= sizeof(acpi_cper_section_t)) &&
        err_record_section != NULL && err_record_section_size > 0)
    {
        if (ddr_subsystem_enabled())
        {
            update_error_record_section(error_domain_idx, err_severity, err_record_section, err_record_section_size);

            if (err_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
            {
                hm_report_error_event(HM_ERROR_REPORT_GPIO, true);
            }
        }
        else if (local_cper_count < MAX_CPER_CACHE)
        {
            // cache current error record section
            update_local_error_record(local_cper_count++, error_domain_idx, err_severity, err_record_section, err_record_section_size);
        }
        else
        {
            HM_LOG_CRIT("CPER cache full (%s)", get_error_domain_name(error_domain_idx));

            // cache is full, report uncorrectable error first
            for (uint32_t non_fatal_cper_idx = 0; non_fatal_cper_idx < local_cper_count; non_fatal_cper_idx++)
            {
                if (error_record_sections[non_fatal_cper_idx].err_severity != ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
                {
                    update_local_error_record(non_fatal_cper_idx, error_domain_idx, err_severity, err_record_section, err_record_section_size);
                    break;
                }
            }
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

void hm_submit_cached_cper()
{
    for (uint32_t local_cper_idx = 0; local_cper_idx < local_cper_count; local_cper_idx++)
    {
        hm_submit_cper(error_record_sections[local_cper_idx].error_domain_idx,
                       error_record_sections[local_cper_idx].err_severity,
                       &error_record_sections[local_cper_idx].cper_section,
                       error_record_sections[local_cper_idx].section_size);
    }
}

void update_local_error_record(uint32_t sec_idx,
                               uint16_t error_domain_idx,
                               acpi_error_severity_t err_severity,
                               acpi_cper_section_t* err_record_section,
                               uint32_t err_record_section_size)
{
    error_record_sections[sec_idx].error_domain_idx = error_domain_idx;
    error_record_sections[sec_idx].err_severity = err_severity;
    memcpy(&error_record_sections[sec_idx].cper_section, err_record_section, err_record_section_size);
    error_record_sections[sec_idx].section_size = err_record_section_size;
}
