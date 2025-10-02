//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_injection_pex.c
 * This file contains pex ras injection related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <atu_api.h>
#include <bug_check.h>
#include <core_cluster_top_regs.h> // for CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS
#include <cper.h>
#include <error_domain_i.h>
#include <error_domain_pex.h>
#include <rng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

acpi_einj_cmd_status_t pex_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);

    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_PEX)
    {
        FPFW_DBGPRINT_ERROR("Invalid PEX error domain(%d)\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }
    FPFW_DBGPRINT_INFO("Injecting PEX rng error\n");
    pex_rng_config_t rng_config;
    rng_config.cluster_pex_base = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR;
    rng_config.cluster_stride = (CORE_CLUSTER_TOP_CORE_CLUSTER1_ADDRESS - CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS);
    const uintptr_t cluster_pex_base_addr = (rng_config.cluster_pex_base + (rng_config.cluster_stride * 1));
    uint32_t pex_rng_base_addr = cluster_pex_base_addr + PEX_RNG_ADDRESS;
    rng_disable_r(pex_rng_base_addr);
    rng_enable_r(pex_rng_base_addr, 0x1);
    return ACPI_EINJ_SUCCESS;
}

static void rng_write_ctrl(uint32_t val)
{
    /* dummy function, do nothing */
    FPFW_UNUSED(val);
}

static uint32_t rng_read_ctrl()
{
    /* dummy function, do nothing */
    return 0;
}
void unused_functions()
{
    FPFW_UNUSED(rng_write_ctrl);
    FPFW_UNUSED(rng_read_ctrl);
}