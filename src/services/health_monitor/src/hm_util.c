//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_util.c
 * Implements utility of health monitor module.
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <gicd_regs.h>
#include <gpio_lib.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <memory_map/ddrss_reserved_regions.h>
#include <ras.h>
#include <silibs_platform.h>
#include <time.h>
#include <utc_sync_client_service.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MSCP_ATU_AP_WINDOW_ERROR_INJECTION_SIZE \
    (ERROR_INJECTION_RESERVATION_END - ERROR_INJECTION_RESERVATION_BASE)
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static atu_map_entry_t error_injection_atu_entry = {
    .ap_base_address = ERROR_INJECTION_RESERVATION_BASE,
    .mscp_start_address = 0,
    .mscp_end_address = ALIGN_UP(MSCP_ATU_AP_WINDOW_ERROR_INJECTION_SIZE, ATU_PAGE_SIZE) - 1,
    .attribute = {ATU_BUS_ATTR_NS},
};
/*------------- Functions ----------------*/

static uint8_t decimal_to_bcd(uint8_t v)
{
    return (uint8_t)(((v / 10) << 4) | (v % 10));
}

static int gmtime_utc(const time_t* seconds, struct tm* utc_time)
{
#if defined(_WIN32)
    return gmtime_s(utc_time, seconds);
#else
    return (gmtime_r(seconds, utc_time) != NULL) ? 0 : -1;
#endif
}

const char* get_error_domain_name(acpi_error_domain_t domain)
{
    static const char* enum_names[ACPI_ERROR_DOMAIN_COUNT] = {
        "DDR",           "MESH",       "SECURE_RAM", "NON_SECURE_RAM", "MCP_PROC", "SCP_PROC",
        "HSP_PROC",      "AP_PROC",    "SDM",        "CDED_SDM",       "SMMU",     "PCIE",
        "GIC",           "PEX",        "VAB",        "NITOWER",        "RHTLM",    "AP_WDT",
        "STD_PROCESSOR", "STD_MEMORY", "STD_PCIE",   "STD_PLATFORM"};

    // Check if the domain is within the valid range
    if (domain < ACPI_ERROR_DOMAIN_COUNT)
    {
        return enum_names[domain];
    }

    return "NA";
}

uint64_t AP_GHES_ADDR(uint32_t mscp_addr)
{
    return (mscp_addr - MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_BASE_ADDR) + (uint64_t)D1_MSCP_ARSM_SRAM_NS_BASE;
}

uint32_t MSCP_GHES_ADDR(uint64_t ap_addr)
{
    if (ap_addr >= ((uint64_t)D1_MSCP_ARSM_SRAM_BASE + (uint64_t)D1_MSCP_ARSM_SRAM_SIZE))
    {
        BUG_ASSERT_PARAM(false, ap_addr, 0);
    }

    return (uint32_t)MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_BASE_ADDR + (uint32_t)(ap_addr - (uint64_t)D1_MSCP_ARSM_SRAM_NS_BASE);
}

void hm_map_error_injection_payload()
{
    hm_config_t* hm_config = get_hm_config();

    if (hm_config->mscp_error_injection_addr_base == NULL)
    {
        int status = atu_map(ATU_ID_MSCP, &error_injection_atu_entry);
        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

        hm_config->mscp_error_injection_addr_base =
            (ras_einj_info_t*)(error_injection_atu_entry.mscp_start_address + RAS_EINJ_VENDOR_DEFINED_STRUCT_OFFSET);
    }
}

void hm_unmap_error_injection_payload()
{
    hm_config_t* hm_config = get_hm_config();

    if (hm_config->mscp_error_injection_addr_base != NULL)
    {
        int status = atu_unmap(ATU_ID_MSCP, &error_injection_atu_entry);
        BUG_ASSERT(status == SILIBS_SUCCESS);

        hm_config->mscp_error_injection_addr_base = NULL;
    }
}

bool hm_is_fatal_error(uint32_t err_severity)
{
    return (err_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL || err_severity == ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL);
}

void hm_copy_cper_record(volatile uint8_t* dest, const uint8_t* src, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        dest[i] = src[i];
    }
}

bool is_standard_error_section_used(int error_domain_idx)
{
    return (error_domain_idx == ACPI_ERROR_DOMAIN_STD_PROCESSOR ||
            error_domain_idx == ACPI_ERROR_DOMAIN_STD_MEMORY || error_domain_idx == ACPI_ERROR_DOMAIN_STD_PCIE ||
            error_domain_idx == ACPI_ERROR_DOMAIN_STD_PLATFORM || error_domain_idx == ACPI_ERROR_DOMAIN_DDR);
}

uint64_t get_cper_timestamp(void)
{
    if (crash_dump_is_utc_ready() == false)
    {
        return 0;
    }

    uint64_t current_utc = utc_sync_client_get_current_time_epoch_ms();
    time_t seconds = (time_t)(current_utc / 1000);

    struct tm utc_time = {0};
    if (gmtime_utc(&seconds, &utc_time) != 0)
    {
        return 0;
    }

    acpi_ghes_timestamp_t cper_ts = {0};
    cper_ts.sec = decimal_to_bcd(utc_time.tm_sec);
    cper_ts.minute = decimal_to_bcd(utc_time.tm_min);
    cper_ts.hour = decimal_to_bcd(utc_time.tm_hour);
    cper_ts.precise = 0x01;
    cper_ts.day = decimal_to_bcd(utc_time.tm_mday);
    cper_ts.month = decimal_to_bcd(utc_time.tm_mon + 1);
    int year = utc_time.tm_year + 1900;
    cper_ts.year = decimal_to_bcd(year % 100);
    cper_ts.century = decimal_to_bcd(year / 100);

    return cper_ts.as_uint64;
}