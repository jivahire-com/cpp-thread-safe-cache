//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ioss_ini.c
 *  This modules initializes IOSS
 */

/*------------- Includes -----------------*/
#include "ioss_ini.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <atu_lib.h>    // for atu_map_entry_t, atu_map, atu_unmap
#include <idsw.h>
#include <idsw_kng.h>
#include <ioss_init.h>
#include <ioss_top_regs.h>      // for IOSS_TOP_IOSS_PCR_ADDRESS, IOSS_...
#include <kng_soc_constants.h>  // for ATU_PAGE_SIZE, NUM_IOSS_INSTANCES
#include <silibs_ap_top_regs.h> // for AP_TOP_D0_VAB_CDED_IOSS_ADDRESS
#include <silibs_common.h>      // for ALIGN_UP
#include <silibs_status.h>      // for SILIBS_SUCCESS
#include <stdint.h>             // for uint64_t, uint32_t, uint8_t
#include <stdio.h>
#include <usb_struct_defaults.h>
#include <vab_cded_ioss_top_regs.h> // for VAB_CDED_IOSS_TOP_IOSS_ADDRESS

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
static atu_map_entry_t atu_ioss_map = {
    // D0-IOSS0
    .ap_base_address = AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
    .mscp_start_address = 0,
    .mscp_end_address = ALIGN_UP(VAB_CDED_IOSS_TOP_IOSS_SIZE, ATU_PAGE_SIZE) - 1,
    .attribute = {{.axprot0 = 0x3, .axprot1 = 0x2, .axnse = 0x3}},
};

static usb_cfg_t usb_cfg_knobs = {
    .power_on_port0 = true,
    .power_on_port1 = true,
    .usb_cfg =
        {
            {
                .sd_config = {.dis_u1u2timerscale = true, .sd_mode = 2},
            },
            {
                .sd_config =
                    {
                        .dis_u1u2timerscale = true,
                        .sd_mode = 2,
                    },
            },
        },
};

static usb_cfg_t usb_fpga_cfg_knobs = {
    .power_on_port0 = true,
    .power_on_port1 = false,
    .usb_cfg =
        {
            {
                .sd_config = {.dis_u1u2timerscale = true, .sd_mode = 2},
            },
        },
};

static usb_cfg_t usb_disable_cfg_knobs = {
    .power_on_port0 = false,
    .power_on_port1 = false,
};

static gpio_cfg_t gpio_cfg_knobs = {
    .gpio_init = true,
    .gpio_afm_init = true,
};

static gpio_cfg_t gpio_disable_cfg_knobs = {
    .gpio_init = false,
    .gpio_afm_init = false,
};

static ioss_cfg_t ioss_init_knobs = {
    .ioss_init = true,
};

/*------------- Functions ----------------*/
void ioss_ini()
{
    int sts = atu_map(ATU_ID_MSCP, &atu_ioss_map);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    uint32_t resolved_ioss_base_addr = atu_ioss_map.mscp_start_address;

    // default is USB disabled
    ioss_init_t init = {.ioss_knobs = &ioss_init_knobs,
                        .gpio_knobs = &gpio_cfg_knobs,
                        .usb_knobs = &usb_disable_cfg_knobs,
                        .ioss_base_addr = resolved_ioss_base_addr};

    /* TODO:
     * Obtain USB configuration information from the configuration manager
     * instead of relying on SDV. ADO - 1508440
     * https://azurecsi.visualstudio.com/Dev/_workitems/edit/1508440
     */
    KNG_PLAT_ID plat = idsw_get_platform_sdv();
    switch (plat)
    {
    // Note: FPGAs only support one controller subsytem - USB2_0
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        init.usb_knobs = &usb_fpga_cfg_knobs;
        init.gpio_knobs = &gpio_disable_cfg_knobs;
        break;

    case PLATFORM_SVP_SIM:
    case PLATFORM_SVP_MIN_CONFIG_SIM:
    case PLATFORM_RVP_EVT_SILICON:
        init.usb_knobs = &usb_cfg_knobs;
        init.gpio_knobs = &gpio_disable_cfg_knobs;
        break;

    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_2D_8C:
    default:
        printf("Skip USB init on platform id: %d\n", plat);
        break;
    }

    // Enable IOSS IPs
    ioss_init(D0_IOSS, &init);

    sts = atu_unmap(ATU_ID_MSCP, &atu_ioss_map);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
}