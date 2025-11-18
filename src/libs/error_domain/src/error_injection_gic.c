//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_injection_gic.c
 * This file contains gic ras injection related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <atu_api.h>
#include <bug_check.h>
#include <cper.h>
#include <error_domain_i.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

PLACED_CODE acpi_einj_cmd_status_t gic_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);

    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_GIC)
    {
        FPFW_DBGPRINT_ERROR("Invalid GIC error domain(%d)\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // check address is in valid GIC range
    if ((0 == einj_payload->param_type.address_32) || (AP_TOP_D0_GIC_SIZE <= einj_payload->param_type.address_32))
    {
        FPFW_DBGPRINT_ERROR("Invalid GIC Err Inject Offset (%d)\n", einj_payload->param_type.address_32);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Check if the address is in ATU mapped space
    uint32_t translated_addr_gic_base = 0x0;
    int status = atu_translate_address(ATU_ID_MSCP, AP_TOP_D0_GIC_ADDRESS, &translated_addr_gic_base);

    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, translated_addr_gic_base);
    BUG_ASSERT_PARAM(MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR == translated_addr_gic_base,
                     MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR,
                     translated_addr_gic_base);

    FPFW_DBGPRINT_INFO("Injecting GIC Err at (%p), value(0x%llx)\n",
                       (void*)(AP_TOP_D0_GIC_ADDRESS + einj_payload->param_type.address_32),
                       einj_payload->value_type.data_64);

    // Write the value to the GIC register
    MMIO_WRITE64(MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + einj_payload->param_type.address_32,
                 einj_payload->value_type.data_64);

    return ACPI_EINJ_SUCCESS;
}