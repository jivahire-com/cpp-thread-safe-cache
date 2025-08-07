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
static bool ghes_table_init = false;

/*------------- Functions ----------------*/
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
    volatile uint32_t* current_ghes_error_record_base = hm_config->mscp_ghes_error_record_addr_base;
    volatile uint32_t* current_ghes_error_record_location = hm_config->mscp_ghes_error_record_addr_table_base;
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
    default_ghes.ack.address.access_size = 2;
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
            default_ghes.address.address = (uint32_t)AP_GHES_ADDR((uint32_t)current_ghes_error_record_location);
            default_ghes.ack.address.address = (uint32_t)AP_GHES_ADDR((uint32_t)current_ghes_ack_base++);

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
            *current_ghes_error_record_location++ = (uint32_t)AP_GHES_ADDR(
                (uint32_t)(current_ghes_error_record_base - hm_config->mscp_ghes_base_apcore_offset));
            *current_ghes_error_record_location++ = 0; // 64 bit address

            // get ghes error record base for next error domain
            current_ghes_error_record_base = (uint32_t*)FPFW_ALIGN_BY(
                sizeof(uint32_t),
                ((uint32_t)current_ghes_error_record_base + default_ghes.error_status_block_length));
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
    uint32_t error_record_base_addr = MSCP_GHES_ADDR((uint32_t)current_error_domain_ghes_base->address.address);
    uint32_t error_record_base = MSCP_GHES_ADDR(*(uint32_t*)error_record_base_addr);

    volatile acpi_ghes_error_record_dual_die_t* current_error_domain_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(error_record_base + hm_config->mscp_ghes_base_apcore_offset);

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
                                 uint32_t err_record_section_size)
{
    if ((error_domain_idx < ACPI_ERROR_DOMAIN_COUNT) && (err_record_section_size <= sizeof(acpi_cper_section_t)))
    {
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
        uint32_t error_record_base_addr = MSCP_GHES_ADDR((uint32_t)current_error_domain_ghes_base->address.address);
        uint32_t error_record_base = MSCP_GHES_ADDR(*(uint32_t*)error_record_base_addr);

        volatile acpi_ghes_error_record_dual_die_t* current_domain_error_record_base =
            (acpi_ghes_error_record_dual_die_t*)(error_record_base + hm_config->mscp_ghes_base_apcore_offset);

        // lock before updating error record information
        wait_for_semaphore(hm_config->semaphore_id, hm_config->semaphore_key);

        if (current_domain_error_record_base->block_status_entry_count != 0)
        {
            // move the first section to the next one
            volatile uint8_t* dst = (volatile uint8_t*)&current_domain_error_record_base->data[1].section;
            volatile uint8_t* src = (volatile uint8_t*)&current_domain_error_record_base->data[0].section;
            for (size_t i = 0; i < sizeof(acpi_cper_section_t); ++i)
            {
                dst[i] = src[i];
            }
        }

        // copy current section to the first section
        for (size_t i = 0; i < err_record_section_size; ++i)
        {
            ((volatile uint8_t*)&current_domain_error_record_base->data[0].section)[i] =
                ((const uint8_t*)err_record_section)[i];
        }

        // update error record section
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
        HM_ET_ERROR_PARAM(HM_ET_TYPE_GHES_INVALID_PARAMS, error_domain_idx);
        BUG_ASSERT_PARAM(false, error_domain_idx, err_record_section_size);
    }
}