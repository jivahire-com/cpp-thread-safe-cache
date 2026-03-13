//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Call into VAB initialization APIs provided by the VAB library.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <boot_status.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <stdint.h>
#include <stdio.h>
#include <utils.h>
#include <vab.h>
#include <vab_irq.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static const guid_t guid_vab = ACPI_ERROR_TYPE_VAB;

/*------------- Functions ----------------*/
static PLACED_CODE uint16_t vab_instances_to_be_enabled(uint8_t die_num)
{
    uint16_t vab_instances_to_init = 0;
    KNG_PLAT_ID plat = idsw_get_platform_sdv();
    switch (plat)
    {
    case PLATFORM_SVP_SIM:
        if (die_num == 0)
        {
            vab_instances_to_init = ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) |
                                     (1 << D0_VAB3_RPSS3) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS));
        }
        else
        {
            vab_instances_to_init = ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) |
                                     (1 << D1_VAB3_RPSS3) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS));
        }
        break;

    case PLATFORM_SVP_MIN_CONFIG_SIM:
        if (die_num == 0)
        {
            vab_instances_to_init = ((1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS));
        }
        else
        {
            vab_instances_to_init = 0x00;
        }
        break;

    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        if (die_num == 0)
        {
            vab_instances_to_init =
                ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS));
        }
        else
        {
            vab_instances_to_init = ((1 << D1_VAB1_RPSS1) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS));
        }
        break;

    case PLATFORM_RVP_EVT_SILICON:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_2D_8C:
        if (die_num == 0)
        {
            vab_instances_to_init = ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) |
                                     (1 << D0_VAB3_RPSS3) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS));
        }
        else
        {
            vab_instances_to_init = ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) |
                                     (1 << D1_VAB3_RPSS3) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS));
        }
        break;

    default:
        FPFW_DBGPRINT_WARNING("Skip VAB init on unknown platform id: %d\n", plat);
        break;
    }

    return vab_instances_to_init;
}

PLACED_CODE FPFW_INIT_COMPONENT(vab, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "atu_svc", "css_pome"))
{
    uint8_t die_num = (uint8_t)idsw_get_die_id();
    FPFW_DBGPRINT_INFO("VAB Initialization: Begin for die:0x%x\n", die_num);
    uint16_t vab_instances_enabled = vab_instances_to_be_enabled(die_num);
    FPFW_DBGPRINT_INFO("Bit mask of VAB instances to be enabled: 0x%x\n", vab_instances_enabled);
    vab_common_init(vab_instances_enabled);

    /* Register the error domain for VABs in Health Monitor */
    hm_register_error_domain(ACPI_ERROR_DOMAIN_VAB, &guid_vab, "VAB Error Domain", vab_error_injection_cb, NULL);

    FPFW_DBGPRINT_INFO("VAB Initialization: End\n");

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_VAB_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_VAB_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(accel_vab_irq, FPFW_INIT_DEPENDENCIES("vab", "accel", "nvic"))
{
    uint16_t vab_instances_to_init = 0;
    uint8_t die_id = (uint8_t)idsw_get_die_id();

    /*
     * Only enable accelerator VABs here.
     * RPSS VABs will be enabled from within the PCIe driver after RPSS intus
     * are programmed.
     */
    switch (die_id)
    {
    case 0:
        vab_instances_to_init = ((1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS));
        break;
    case 1:
        vab_instances_to_init = ((1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS));
        break;
    default:
        FPFW_DBGPRINT_WARNING("accel_vab_irq init skipped! Got invalid die id: %d\n", die_id);
        break;
    }

    enable_vab_isrs(vab_instances_to_init);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
