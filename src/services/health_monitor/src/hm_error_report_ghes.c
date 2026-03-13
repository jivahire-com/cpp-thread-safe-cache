//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_report_ghes.c
 * Implements GHES table related functionality.
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <cortex_m7_atomics.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <idhw.h>
#include <idsw.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bool ghes_table_init = false;

/*------------- Functions ----------------*/
static int cper_severity_rank(acpi_error_severity_t severity)
{
    switch (severity)
    {
    case ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL:
        return 0;
    case ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL:
        return 1;
    case ACPI_ERROR_SEVERITY_CORRECTED:
        return 2;
    default:
        return 3;
    }
}
static acpi_error_severity_t get_highest_severity(acpi_ghes_error_record_dual_die_t* record)
{
    acpi_error_severity_t adjusted_severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;

    if (record->block_status_entry_count == 0)
    {
        return adjusted_severity;
    }

    acpi_error_severity_t s0 = record->data[0].error_severity;
    acpi_error_severity_t s1 = (record->block_status_entry_count == 2) ? record->data[1].error_severity : s0;

    s0 = (s0 == ACPI_ERROR_SEVERITY_INFORMATIONAL) ? ACPI_ERROR_SEVERITY_CORRECTED : s0;
    s1 = (s1 == ACPI_ERROR_SEVERITY_INFORMATIONAL) ? ACPI_ERROR_SEVERITY_CORRECTED : s1;

    if (cper_severity_rank(s0) < cper_severity_rank(s1))
    {
        adjusted_severity = s0;
    }
    else
    {
        adjusted_severity = s1;
    }

    return adjusted_severity;
}

void set_ghes_table_ready()
{
    ghes_table_init = true;
}

void construct_mscp_ghes_table()
{
    HM_ET_INFO_PARAM(HM_ET_TYPE_GHES_CONSTRUCT_TBL, ACPI_ERROR_DOMAIN_COUNT * sizeof(acpi_ghes_error_record_dual_die_t));
    static_assert((ACPI_ERROR_DOMAIN_COUNT * sizeof(acpi_ghes_error_record_dual_die_t) < RAS_GHES_ERROR_RECORD_SIZE),
                  "GHES error record size is too small");

    hm_config_t* hm_config = get_hm_config();

    volatile acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    volatile uint64_t* current_ghes_error_record_base = hm_config->mscp_ghes_error_record_addr_base;
    volatile uint64_t* current_ghes_error_record_location = hm_config->mscp_ghes_error_record_addr_table_base;
    volatile uint64_t* current_ghes_ack_base = hm_config->mscp_ghes_ack_addr_table_base;

    // default ghes values
    acpi_ghes_t default_ghes;
    memset(&default_ghes, 0, sizeof(acpi_ghes_t));

    // Explicitly initialize the required members
    default_ghes.type = HM_GHES_VERSION_V2;
    default_ghes.source_id = 0;
    default_ghes.related_source_id = 0xFFFF;
    default_ghes.flags = 0;
    default_ghes.enabled = 0;
    default_ghes.num_of_records = HM_ERROR_RECORD_COUNT;
    default_ghes.max_sections_per_record = HM_ERROR_SECTION_COUNT;
    default_ghes.max_raw_data_length = 0;
    default_ghes.error_status_block_length = sizeof(acpi_ghes_error_record_dual_die_t);

    // Error Status Address
    default_ghes.address.space_id = 0;
    default_ghes.address.reg_bit_width = 0x40;
    default_ghes.address.reg_bit_offset = 0;
    default_ghes.address.access_size = 4;

    // Hardware Error Notification Structure Values
    default_ghes.notification.type = 3;
    default_ghes.notification.length = 28;
    default_ghes.notification.cfg_wr_enable_type = 0;
    default_ghes.notification.cfg_wr_enable_poll_interval = 0;
    default_ghes.notification.cfg_wr_enable_sw_poll_thrs_val = 0;
    default_ghes.notification.cfg_wr_enable_sw_poll_thrs_win = 0;
    default_ghes.notification.cfg_wr_enable_err_thrs_val = 0;
    default_ghes.notification.cfg_wr_enable_err_thrs_win = 0;
    default_ghes.notification.poll_interval = 0;
    default_ghes.notification.vector = 0;
    default_ghes.notification.sw_to_polling_threshold_value = 0;
    default_ghes.notification.sw_to_polling_threshold_window = 0;
    default_ghes.notification.error_threshold_value = 0;
    default_ghes.notification.error_threashold_window = 0;

    // Read Ack Structure values, GHES v2
    default_ghes.ack.address.space_id = 0;
    default_ghes.ack.address.reg_bit_width = 0x40;
    default_ghes.ack.address.reg_bit_offset = 0;
    default_ghes.ack.address.access_size = 4;
    default_ghes.ack.rd_ack_preserve = 0;
    default_ghes.ack.rd_ack_wr = 0;

    // default error record values
    acpi_ghes_error_record_dual_die_t default_ghes_error_record;
    memset(&default_ghes_error_record, 0, sizeof(default_ghes_error_record));
    default_ghes_error_record.data_length = sizeof(default_ghes_error_record.data);
    default_ghes_error_record.error_severity = 3;

    for (uint32_t i = 0; i < default_ghes.max_sections_per_record; i++)
    {
        default_ghes_error_record.data[i].error_severity = 3;
        default_ghes_error_record.data[i].revision = 0x300;
        default_ghes_error_record.data[i].error_data_length = sizeof(default_ghes_error_record.data[i].section);
    }

    if (ghes_table_init == false)
    {
        for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
        {
            // GHES Structure update for current error domain
            default_ghes.source_id = error_domain_idx;
            default_ghes.address.address = AP_GHES_ADDR((uint32_t)current_ghes_error_record_location);
            default_ghes.ack.address.address = AP_GHES_ADDR((uint32_t)current_ghes_ack_base);
            *current_ghes_ack_base++ = GHES_ACK_UNSET_VALUE;

            for (size_t i = 0; i < sizeof(acpi_ghes_t); i++)
            {
                ((volatile uint8_t*)current_ghes_base)[i] = ((const uint8_t*)&default_ghes)[i];
            }

            current_ghes_base++;

            // Error status block update for current error domain
            for (uint32_t i = 0; i < default_ghes.max_sections_per_record; i++)
            {
                memcpy(&default_ghes_error_record.data[i].section_type,
                       &ACPI_ERROR_DOMAIN_SEC_INFO[error_domain_idx].section_type,
                       sizeof(guid_t));
            }

            // init as 0. We will update this once we have actual error data.
            default_ghes_error_record.block_status_entry_count = 0;

            // Error Record update for current error domain
            for (size_t i = 0; i < sizeof(acpi_ghes_error_record_dual_die_t); i++)
            {
                ((volatile uint8_t*)current_ghes_error_record_base)[i] =
                    ((const uint8_t*)&default_ghes_error_record)[i];
            }

            // update error record location table for current error domain
            *current_ghes_error_record_location++ = AP_GHES_ADDR((uint32_t)current_ghes_error_record_base);

            // get ghes error record base for next error domain
            current_ghes_error_record_base = (uint64_t*)FPFW_ALIGN_BY(
                sizeof(uint64_t),
                ((uintptr_t)current_ghes_error_record_base + default_ghes.error_status_block_length));
        }

        set_ghes_table_ready();
        HM_LOG_INFO("GHES created(%p)", (void*)hm_config->mscp_ghes_base);
    }
}

void activate_error_domain(uint16_t error_domain_idx, const guid_t* error_domain_guid, const char* error_domain_txt)
{
    bool skip = false;

    if (ghes_table_init == false)
    {
        HM_ET_ERROR_PARAM(HM_ET_TYPE_GHES_INVALID_PARAMS, ghes_table_init);
        BUG_ASSERT_PARAM(false, ghes_table_init, 0);
    }

    hm_config_t* hm_config = get_hm_config();
    HM_ET_INFO(HM_ET_TYPE_GHES_GET_CONFIG);
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

    // locate the GHES base of current error domain
    volatile acpi_ghes_t* current_error_domain_ghes_base = hm_config->mscp_ghes_base;
    current_error_domain_ghes_base += error_domain_idx;

    // locate error record base of current error domain
    uint32_t error_record_base_addr = MSCP_GHES_ADDR(current_error_domain_ghes_base->address.address);
    uint32_t error_record_base = MSCP_GHES_ADDR((uint64_t)(*(uint32_t*)error_record_base_addr));

    volatile acpi_ghes_error_record_dual_die_t* current_error_domain_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(error_record_base);

    // lock before updating error domain information
    wait_for_semaphore(hm_config->semaphore_id, hm_config->semaphore_key);

    if (current_error_domain_ghes_base->enabled == true)
    {
        skip = true;
    }
    else
    {
        // current record block not contains any error data yet.
        current_error_domain_error_record_base->block_status_entry_count = 0;

        // update error domain information for each Generic Error Data Entry
        for (uint32_t i = 0; i < current_error_domain_ghes_base->max_sections_per_record; i++)
        {
            if (error_domain_guid != NULL)
            {
                for (size_t idx = 0; idx < sizeof(current_error_domain_error_record_base->data[i].fru_id); idx++)
                {
                    ((volatile uint8_t*)&current_error_domain_error_record_base->data[i].fru_id)[idx] =
                        ((const uint8_t*)error_domain_guid)[idx];
                }

                current_error_domain_error_record_base->data[i].valid_fru = 1;
            }
            else
            {
                current_error_domain_error_record_base->data[i].valid_fru = 0;
            }

            if (error_domain_txt != NULL)
            {
                for (size_t idx = 0; idx < sizeof(current_error_domain_error_record_base->data[i].fru_text) - 1 &&
                                     idx < strlen(error_domain_txt);
                     idx++)
                {
                    ((volatile uint8_t*)&current_error_domain_error_record_base->data[i].fru_text)[idx] =
                        ((const uint8_t*)error_domain_txt)[idx];
                }

                // Null-terminate the string
                ((volatile uint8_t*)&current_error_domain_error_record_base->data[i]
                     .fru_text)[sizeof(current_error_domain_error_record_base->data[i].fru_text) - 1] = '\0';

                current_error_domain_error_record_base->data[i].valid_fru_str = 1;
            }
            else
            {
                current_error_domain_error_record_base->data[i].valid_fru_str = 0;
            }
        }

        // activate error domain
        current_error_domain_ghes_base->enabled = 1;
    }

    release_semaphore(hm_config->semaphore_id);

    HM_LOG_INFO("%s error domain %s", get_error_domain_name(error_domain_idx), skip ? "already registered" : "registered");
}

void update_error_record_section(uint16_t error_domain_idx,
                                 acpi_error_severity_t err_severity,
                                 acpi_cper_section_t* err_record_section,
                                 uint32_t err_record_section_size,
                                 uint64_t cper_timestamp)
{
    if ((error_domain_idx < ACPI_ERROR_DOMAIN_COUNT) && (err_record_section_size <= sizeof(acpi_cper_section_t)))
    {
        acpi_error_severity_t adjusted_severity = err_severity;

        if (ghes_table_init == false)
        {
            HM_ET_ERROR_PARAM(HM_ET_TYPE_GHES_INVALID_PARAMS, ghes_table_init);
            BUG_ASSERT_PARAM(false, ghes_table_init, 0);
        }

        hm_config_t* hm_config = get_hm_config();
        HM_ET_INFO(HM_ET_TYPE_GHES_GET_CONFIG);
        BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

        // update common heder part of error record section if vendor specific
        if (is_standard_error_section_used(error_domain_idx) == false)
        {
            // inband path only
            // override recoverable error severity to corrected for vendor specific error domains
            adjusted_severity = ((err_severity == ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL) ? ACPI_ERROR_SEVERITY_CORRECTED
                                                                                             : err_severity);
        }

        // locate the GHES base of current error domain
        volatile acpi_ghes_t* current_error_domain_ghes_base = hm_config->mscp_ghes_base;
        current_error_domain_ghes_base += error_domain_idx;

        // locate error record base of current error domain
        uint32_t error_record_base_addr = MSCP_GHES_ADDR(current_error_domain_ghes_base->address.address);
        uint32_t error_record_base = MSCP_GHES_ADDR((uint64_t)(*(uint32_t*)error_record_base_addr));

        volatile acpi_ghes_error_record_dual_die_t* current_domain_error_record_base =
            (acpi_ghes_error_record_dual_die_t*)(error_record_base);

        // lock before updating error record information
        wait_for_semaphore(hm_config->semaphore_id, hm_config->semaphore_key);

        // if we have a room to hold section entry then
        if (current_domain_error_record_base->block_status_entry_count < 2)
        {
            uint32_t idx = current_domain_error_record_base->block_status_entry_count;

            // update severity on error data entry
            current_domain_error_record_base->data[idx].error_severity = adjusted_severity;

            // update utc if possible
            current_domain_error_record_base->data[idx].valid_fru_timestamp = (cper_timestamp != 0U);
            current_domain_error_record_base->data[idx].timestamp = cper_timestamp;

            // clear the section data
            memset((void*)&current_domain_error_record_base->data[idx].section, 0, sizeof(acpi_cper_section_t));

            // copy cper section into error data entry
            hm_copy_cper_record((volatile uint8_t*)&current_domain_error_record_base->data[idx].section,
                                (const uint8_t*)err_record_section,
                                err_record_section_size);

            current_domain_error_record_base->block_status_entry_count++;
        }
        else
        {
            // if section entry is full, override the non-fatal one if current error is fatal
            if (hm_is_fatal_error(err_severity))
            {
                for (int i = 0; i < 2; i++)
                {
                    if (current_domain_error_record_base->data[i].error_severity != ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
                    {
                        current_domain_error_record_base->data[i].error_severity = adjusted_severity;

                        // update utc if possible
                        current_domain_error_record_base->data[i].valid_fru_timestamp = (cper_timestamp != 0U);
                        current_domain_error_record_base->data[i].timestamp = cper_timestamp;

                        hm_copy_cper_record((volatile uint8_t*)&current_domain_error_record_base->data[i].section,
                                            (const uint8_t*)err_record_section,
                                            err_record_section_size);
                        break;
                    }
                }
            }
        }

        adjusted_severity = get_highest_severity((acpi_ghes_error_record_dual_die_t*)current_domain_error_record_base);
        // Update Error Status Block severity to reflect the most severe entry
        current_domain_error_record_base->error_severity = adjusted_severity;

        if ((adjusted_severity == ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL) ||
            (adjusted_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL))
        {
            if (current_domain_error_record_base->block_status_ue)
            {
                current_domain_error_record_base->block_status_multi_ue = true;
            }
            current_domain_error_record_base->block_status_ue = true;
        }

        if (adjusted_severity == ACPI_ERROR_SEVERITY_CORRECTED)
        {
            if (current_domain_error_record_base->block_status_ce)
            {
                current_domain_error_record_base->block_status_multi_ce = true;
            }
            current_domain_error_record_base->block_status_ce = true;
        }

        bool error_domain_enabled = current_error_domain_ghes_base->enabled;
        release_semaphore(hm_config->semaphore_id);

        // report AP for new CPER arrival
        if (error_domain_enabled)
        {
            // Request ACK for the error to GHES
            uint64_t* ack_base_addr = (uint64_t*)MSCP_GHES_ADDR(current_error_domain_ghes_base->ack.address.address);
            *ack_base_addr = GHES_ACK_SET_VALUE;

            cortex_m7_atomic_call_data_synchronization_barrier();

            hm_report_error_event(HM_ERROR_REPORT_INTERRUPT, true);
            HM_LOG_INFO("CPER reported, inband(domain=%s, sev=%d)", get_error_domain_name(error_domain_idx), err_severity);
        }
    }
    else
    {
        HM_ET_ERROR_PARAM(HM_ET_TYPE_GHES_INVALID_PARAMS, error_domain_idx);
        BUG_ASSERT_PARAM(false, error_domain_idx, err_record_section_size);
    }
}