//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_setup.cpp
 * Tests health monitor setup
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <mscp_exp_rmss_memory_map.h>
/*-- Symbolic Constant Macros (defines) --*/
hm_config_t hm_config_test = {0};

static acpi_ghes_t ghes_local[ACPI_ERROR_DOMAIN_COUNT + 1];
static acpi_ghes_error_record_dual_die_t ghes_error_record_local[ACPI_ERROR_DOMAIN_COUNT + 1];
static uint64_t ghes_record_addr_table_local[ACPI_ERROR_DOMAIN_COUNT] = {0};
static uint64_t ghes_ack_addr_table_local[ACPI_ERROR_DOMAIN_COUNT] = {0};
static uint8_t hsp_ras_payload[SCP_EXP_HSP_RAS_PAYLOAD_SIZE] = {0};
static uint8_t mscp_cper_record[SCP_EXP_MSCP_CPER_REPORT_SIZE] = {0};

/*-- Declarations (Statics and globals) --*/
uint32_t __wrap_AP_GHES_ADDR(uint32_t mscp_addr)
{
    return mscp_addr;
}

uint32_t __wrap_MSCP_GHES_ADDR(uint32_t ap_addr)
{
    return ap_addr;
}

void construct_mscp_ghes_table()
{

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

    for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
    {
        // GHES Structure update for current error domain
        default_ghes.source_id = error_domain_idx;
        default_ghes.enabled = true;
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
            ((volatile uint8_t*)current_ghes_error_record_base)[i] = ((const uint8_t*)&default_ghes_error_record)[i];
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
}

int pre_ddr_setup(void** state)
{
    FPFW_UNUSED(state);

    hm_config_test.mscp_ghes_base = (acpi_ghes_t*)ghes_local;
    hm_config_test.mscp_ghes_error_record_addr_table_base = (uint32_t*)ghes_record_addr_table_local;
    hm_config_test.mscp_ghes_ack_addr_table_base = (uint64_t*)ghes_ack_addr_table_local;
    hm_config_test.mscp_ghes_error_record_addr_base = (uint32_t*)ghes_error_record_local;
    hm_config_test.mscp_ghes_base_apcore_offset = 0;
    hm_config_test.mscp_hsp_ras_payload_base = (uint8_t*)hsp_ras_payload;
    hm_config_test.mscp_full_cper_record_base = (uint8_t*)mscp_cper_record;
    hm_config_test.semaphore_id = SEM_ID_MSCP_EXP_1;
    hm_config_test.semaphore_key = 0;
    hm_config_test.is_primary = 1;

    hm_pre_ddr_init(&hm_config_test);

    hm_config_t* hm_config = get_hm_config();

    assert_true(hm_config->mscp_ghes_base == hm_config_test.mscp_ghes_base);
    assert_true(hm_config->mscp_ghes_error_record_addr_table_base == hm_config_test.mscp_ghes_error_record_addr_table_base);
    assert_true(hm_config->mscp_ghes_ack_addr_table_base == hm_config_test.mscp_ghes_ack_addr_table_base);
    assert_true(hm_config->mscp_ghes_error_record_addr_base == hm_config_test.mscp_ghes_error_record_addr_base);
    assert_true(hm_config->mscp_ghes_base_apcore_offset == hm_config_test.mscp_ghes_base_apcore_offset);

    construct_mscp_ghes_table();

    return 0;
}
}
