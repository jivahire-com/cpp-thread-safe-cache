//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Call into VAB initialization APIs provided by the VAB library.
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <stdint.h>
#include <stdio.h>
#include <vab.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static uint16_t vab_instances_to_be_enabled(uint8_t die_num)
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
            vab_instances_to_init = ((1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS));
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
        printf("Skip VAB init on unknown platform id: %d\n", plat);
        break;
    }

    return vab_instances_to_init;
}

FPFW_INIT_COMPONENT(vab, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "atu_svc", "ddr"))
{
    uint8_t die_num = (uint8_t)idsw_get_die_id();
    printf("VAB Initialization: Begin for die:0x%x\n", die_num);
    uint16_t vab_instances_enabled = vab_instances_to_be_enabled(die_num);
    printf("Bit mask of VAB instances to be enabled: 0x%x\n", vab_instances_enabled);
    vab_common_init(vab_instances_enabled);
    printf("VAB Initialization: End\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(accel_vab_irq, FPFW_INIT_DEPENDENCIES("vab", "accel", "nvic"))
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
        printf("accel_vab_irq init skipped! Got invalid die id: %d\n", die_id);
        break;
    }

    enable_vab_isrs(vab_instances_to_init);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
