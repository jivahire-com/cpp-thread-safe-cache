//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_report_ghes.c
 * Implements GHES table related functionality.
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <idhw.h>
#include <idsw.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void update_error_record_section(uint16_t error_domain_idx,
                                 acpi_error_severity_t err_severity,
                                 acpi_cper_section_t* err_record_section,
                                 uint32_t err_record_section_size)
{
    if ((error_domain_idx < ACPI_ERROR_DOMAIN_COUNT) && (err_record_section_size <= sizeof(acpi_cper_section_t)))
    {
        hm_config_t* hm_config = get_hm_config();
        BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

        // locate the GHES base of current error domain
        volatile acpi_ghes_t* current_error_domain_ghes_base = hm_config->mscp_ghes_base;
        current_error_domain_ghes_base += error_domain_idx;

        // locate error record base of current error domain
        uint32_t error_record_base_addr = MSCP_GHES_ADDR((uint32_t)current_error_domain_ghes_base->address.address);
        uint32_t error_record_base = MSCP_GHES_ADDR(*(uint32_t*)error_record_base_addr);

        volatile acpi_ghes_error_record_dual_die_t* current_domain_error_record_base =
            (acpi_ghes_error_record_dual_die_t*)(error_record_base + hm_config->mscp_ghes_base_apcore_offset);

        // locate error record section
        uint32_t current_section_idx = current_domain_error_record_base->block_status_entry_count;
        if (current_section_idx > 1)
        {
            current_section_idx = 1;
        }

        // lock before updating error record information
        wait_for_semaphore(hm_config->semaphore_id, hm_config->semaphore_key);

        // update error record section
        current_domain_error_record_base->data[current_section_idx].error_severity = err_severity;
        for (size_t i = 0; i < err_record_section_size; ++i)
        {
            ((volatile uint8_t*)&current_domain_error_record_base->data[current_section_idx].section)[i] =
                ((const uint8_t*)err_record_section)[i];
        }

        // update error record
        // override error severity if it is recoverable
        // As OS tries to recover but will end up with OS BUG_CHECK as don't know how to recover
        current_domain_error_record_base->error_severity =
            ((err_severity == ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL) ? ACPI_ERROR_SEVERITY_CORRECTED : err_severity);

        if ((err_severity == ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL) ||
            (err_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL))
        {
            if (current_domain_error_record_base->block_status_ue)
            {
                current_domain_error_record_base->block_status_multi_ue = true;
            }
            current_domain_error_record_base->block_status_ue = true;
        }

        if (err_severity == ACPI_ERROR_SEVERITY_CORRECTED)
        {
            if (current_domain_error_record_base->block_status_ce)
            {
                current_domain_error_record_base->block_status_multi_ce = true;
            }
            current_domain_error_record_base->block_status_ce = true;
        }

        if (current_domain_error_record_base->block_status_entry_count < 2)
        {
            current_domain_error_record_base->block_status_entry_count++;
        }

        bool error_domain_enabled = current_error_domain_ghes_base->enabled;
        release_semaphore(hm_config->semaphore_id);

        // report AP for new CPER arrival
        if (error_domain_enabled)
        {
            hm_report_error_event(HM_ERROR_REPORT_INTERRUPT, true);
        }

        HM_LOG_INFO("%s CPER record updated, severity(%d)", get_error_domain_name(error_domain_idx), err_severity);
    }
    else
    {
        BUG_ASSERT_PARAM(false, error_domain_idx, err_record_section_size);
    }
}