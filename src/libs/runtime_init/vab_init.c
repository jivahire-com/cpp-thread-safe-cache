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
#include <kng_soc_constants.h>
#include <stdint.h>
#include <stdio.h>
#include <vab.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(vab, FPFW_INIT_DEPENDENCIES("css_pome"))
{
    printf("VAB Initialization: Begin\n");

    /* Choose which VAB instances are to be initialized based on SDV type */
    uint16_t vab_instances_to_init = 0;
    PLAT_ID plat = idsw_get_platform_sdv();
    switch (plat)
    {
    case PLATFORM_SVP_SIM:
        printf("Skip RPSS VAB init on SVP!\n");
        break;

    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        vab_instances_to_init = ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2));
        break;

    case PLATFORM_RVP_EVT_SILICON:
        vab_instances_to_init =
            ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3));
        break;

    default:
        printf("Skip RPSS VAB init on unknown platform id: %d\n", plat);
        break;
    }

    vab_init(vab_instances_to_init);
    printf("VAB Initialization: End\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
