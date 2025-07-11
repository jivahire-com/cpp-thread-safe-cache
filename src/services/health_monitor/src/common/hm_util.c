//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_util.c
 * Implements utility of health monitor module.
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <bug_check.h>
#include <gicd_regs.h>
#include <gpio_lib.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <ras.h>
#include <silibs_platform.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
hm_config_t* health_monitor_config = NULL;

/*------------- Functions ----------------*/
void create_full_mscp_cper_record(acpi_error_domain_t err_domain_idx,
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

    uint32_t error_record_base_addr = MSCP_GHES_ADDR((uint32_t)current_ghes_base->address.address);
    uint32_t error_record_base = MSCP_GHES_ADDR(*(uint32_t*)error_record_base_addr);

    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(error_record_base + hm_config->mscp_ghes_base_apcore_offset);

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

hm_config_t* get_hm_config()
{
    return health_monitor_config;
}

void hm_pre_ddr_init(hm_config_t* hm_config)
{
    health_monitor_config = hm_config;
}

const char* get_error_domain_name(acpi_error_domain_t domain)
{
    static const char* enum_names[] = {"DDR",
                                       "MESH",
                                       "SECURE_RAM",
                                       "NON_SECURE_RAM",
                                       "MCP_PROC",
                                       "SCP_PROC",
                                       "HSP_PROC",
                                       "AP_PROC",
                                       "SDM",
                                       "CDED_SDM",
                                       "SMMU",
                                       "PCIE",
                                       "GIC",
                                       "STD_PROCESSOR",
                                       "STD_MEMORY",
                                       "STD_PCIE",
                                       "STD_PLATFORM"};

    // Check if the domain is within the valid range
    if (domain < ACPI_ERROR_DOMAIN_COUNT)
    {
        return enum_names[domain];
    }

    return "NA";
}

uint32_t AP_GHES_ADDR(uint32_t mscp_addr)
{
    return mscp_addr - MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR;
}

uint32_t MSCP_GHES_ADDR(uint32_t ap_addr)
{
    return MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ap_addr;
}