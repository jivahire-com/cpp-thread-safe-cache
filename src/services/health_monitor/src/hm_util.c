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
    .attribute = {ATU_BUS_ATTR_ROOT},
};
/*------------- Functions ----------------*/

const char* get_error_domain_name(acpi_error_domain_t domain)
{
    static const char* enum_names[ACPI_ERROR_DOMAIN_COUNT] = {
        "DDR",     "MESH",    "SECURE_RAM", "NON_SECURE_RAM", "MCP_PROC",   "SCP_PROC", "HSP_PROC",
        "AP_PROC", "SDM",     "CDED_SDM",   "SMMU",           "PCIE",       "GIC",      "PEX",
        "VAB",     "NITOWER", "RHTLM",      "STD_PROCESSOR",  "STD_MEMORY", "STD_PCIE", "STD_PLATFORM"};

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
