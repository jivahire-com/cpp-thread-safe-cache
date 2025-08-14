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