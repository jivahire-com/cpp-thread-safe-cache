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
static void update_local_error_record(uint32_t sec_idx,
                                      uint16_t error_domain_idx,
                                      acpi_error_severity_t err_severity,
                                      acpi_cper_section_t* err_record_section,
                                      uint32_t err_record_section_size);

static void create_full_cper_record(acpi_error_domain_t err_domain_idx,
                                    acpi_error_severity_t severity,
                                    acpi_cper_section_t* err_record_section,
                                    uint32_t err_record_section_size,
                                    acpi_cper_record_t* cper_record);

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
        // if this is MSCP error domain, generate full CPER record so that HSP can process it
        if (error_domain_idx == ACPI_ERROR_DOMAIN_MCP_PROC || error_domain_idx == ACPI_ERROR_DOMAIN_SCP_PROC)
        {
            acpi_cper_record_t cper_record = {0};
            create_full_cper_record(error_domain_idx, err_severity, err_record_section, err_record_section_size, &cper_record);

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
            HM_ET_ERROR_PARAM(HM_ET_TYPE_CPER_CACHE_ERROR, error_domain_idx);

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
        HM_ET_ERROR_PARAM(HM_ET_TYPE_CPER_INVALID_PARAMS, error_domain_idx);
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

static void create_full_cper_record(acpi_error_domain_t err_domain_idx,
                                    acpi_error_severity_t severity,
                                    acpi_cper_section_t* err_record_section,
                                    uint32_t err_record_section_size,
                                    acpi_cper_record_t* cper_record)
{
    uint8_t CPER_NAME[] = {'C', 'P', 'E', 'R'};
    guid_t PLATFORM_ID = {0x1BF602DA, 0x3F0D, 0x4DA3, {0xBC, 0x01, 0xC7, 0x6D, 0xB6, 0xA9, 0x57, 0x18}};
    guid_t CREATOR_ID = {0xCF07C4BD, 0xB789, 0x4E18, {0xB3, 0xC4, 0x1F, 0x73, 0x2C, 0xB5, 0x71, 0x31}};
    guid_t NOTIFICATION_TYPE = {0x4B5664D5, 0x79B1, 0x4C3C, {0x92, 0x6D, 0x91, 0xD0, 0x86, 0xDD, 0xA0, 0xF7}};

    if (err_domain_idx >= ACPI_ERROR_DOMAIN_COUNT || cper_record == NULL || err_record_section == NULL ||
        err_record_section_size == 0)
    {
        BUG_ASSERT_PARAM(false, err_domain_idx, err_record_section);
    }

    memset(cper_record, 0, sizeof(acpi_cper_record_t));

    // CPER Record Header
    memcpy(&cper_record->record_header.signature_start, CPER_NAME, sizeof(CPER_NAME));
    cper_record->record_header.revision = 0x0101; // Revision 1.1
    cper_record->record_header.signature_end = 0xFFFFFFFF;
    cper_record->record_header.section_count = 1;
    cper_record->record_header.error_severity = severity;
    cper_record->record_header.valid_platform_id = 1;
    cper_record->record_header.valid_time_stamp = 0;
    cper_record->record_header.valid_partition_id = 0;
    cper_record->record_header.valid_reserved1 = 0;
    cper_record->record_header.record_length =
        sizeof(acpi_cper_record_header_t) + sizeof(acpi_cper_sec_desc_t) + err_record_section_size;
    cper_record->record_header.time_stamp = 0;
    memcpy(&cper_record->record_header.platform_id, &PLATFORM_ID, sizeof(guid_t));
    memset(&cper_record->record_header.partition_id, 0, sizeof(guid_t));
    memcpy(&cper_record->record_header.creator_id, &CREATOR_ID, sizeof(guid_t));
    memcpy(&cper_record->record_header.notification_type, &NOTIFICATION_TYPE, sizeof(guid_t));
    cper_record->record_header.record_id = 0;
    cper_record->record_header.flags = 0;
    cper_record->record_header.persistence_info = 0;
    memset(&cper_record->record_header.reserved1, 0, sizeof(cper_record->record_header.reserved1));

    // CPER Section Descriptor
    cper_record->section_descriptor[0].section_offset = sizeof(acpi_cper_record_header_t) + sizeof(acpi_cper_sec_desc_t);
    cper_record->section_descriptor[0].section_length = err_record_section_size;
    cper_record->section_descriptor[0].revision = 0x0300;
    cper_record->section_descriptor[0].valid_fru_id = 0;
    cper_record->section_descriptor[0].valid_fru_string = 0;
    cper_record->section_descriptor[0].valid_reserved1 = 0;
    cper_record->section_descriptor[0].reserved1 = 0;
    cper_record->section_descriptor[0].flags = 0;
    cper_record->section_descriptor[0].section_type = ACPI_ERROR_DOMAIN_SEC_INFO[err_domain_idx].section_type;
    memset(&cper_record->section_descriptor[0].fru_id, 0, sizeof(guid_t));
    cper_record->section_descriptor[0].section_severity = severity;
    memset(cper_record->section_descriptor[0].fru_text, 0, ACPI_FRU_TEXT_LENGTH);

    // FRU Info update from GHES
    hm_config_t* hm_config = get_hm_config();
    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += err_domain_idx;

    uint32_t error_record_base = (uint32_t)(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(*(uint32_t*)error_record_base + hm_config->mscp_ghes_base_apcore_offset);

    if (current_ghes_error_record_base->data[0].valid_fru)
    {
        cper_record->section_descriptor[0].valid_fru_id = 1;
        memcpy(&cper_record->section_descriptor[0].fru_id, &current_ghes_error_record_base->data[0].fru_id, sizeof(guid_t));
    }

    if (current_ghes_error_record_base->data[0].valid_fru_str)
    {
        cper_record->section_descriptor[0].valid_fru_string = 1;
        memcpy(cper_record->section_descriptor[0].fru_text, current_ghes_error_record_base->data[0].fru_text, ACPI_FRU_TEXT_LENGTH);
    }

    // CPER section
    memcpy(&cper_record->section_data[0], err_record_section, err_record_section_size);
}
