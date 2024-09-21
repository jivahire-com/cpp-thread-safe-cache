//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file usb.c
 *  This module initializes the USBSS
 */

/*------------- Includes -----------------*/
#include "usb_init.h" // for dwc3_usb_t, usbss_cfg_t, scaledown_config_t

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <atu_lib.h>    // for atu_map_entry_t, atu_map, atu_unmap
#include <ioss.h>
#include <kng_soc_constants.h>  // for ATU_PAGE_SIZE, NUM_IOSS_INSTANCES
#include <silibs_ap_top_regs.h> // for AP_TOP_D0_VAB_CDED_IOSS_ADDRESS
#include <silibs_common.h>      // for ALIGN_UP
#include <stdbool.h>            // for false
#include <stdint.h>             // for uint32_t
#include <stdio.h>
#include <vab_cded_ioss_top_regs.h> // for VAB_CDED_IOSS_TOP_IOSS_ADDRESS

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static atu_map_entry_t atu_ioss_map[NUM_IOSS_INSTANCES] = {
    {
        // D0-IOSS0
        .ap_base_address = AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(VAB_CDED_IOSS_TOP_IOSS_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {{.axprot0 = 0x3, .axprot1 = 0x2, .axnse = 0x3}},
    },
};

/*------------- Functions ----------------*/

void usb_init(uint32_t usb_init_block)
{
    if (!(usb_init_block & (USBSS_INIT_USB2_0 | USBSS_INIT_USB2_1)))
    {
        printf("Skip usb_init! Invalid init flags \n!");
        FPFW_RUNTIME_ASSERT(false);
    }

    int sts = atu_map(ATU_ID_MSCP, &atu_ioss_map[0]);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    uint32_t resolved_ioss_base_addr = atu_ioss_map[0].mscp_start_address;

    sts = set_mscp_ioss_base_addr(resolved_ioss_base_addr);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    usbss_cfg_t usbss_cfg = {};
    // unused in silibs, atu mapped address in set_mscp_ioss_base_addr used as base address
    usbss_cfg.usbss_base_address = 0x01;

    // USB_0 settings
    usbss_cfg.usb_cfg[0].sd_config.dis_u1u2timerscale = true;
    usbss_cfg.usb_cfg[0].sd_config.sd_mode = DISABLE_ALL_SD;
    usbss_cfg.usb_cfg[0].sd_config.frmscldwn = FRM_SCLDWN_1024;
    usbss_cfg.usb_cfg[0].sd_config.pwrdnscale = 0x07;
    usbss_cfg.usb_cfg[0].configure_phys = 0x0;
    usbss_cfg.usb_cfg[0].scrambling_disable = 0x0;

    // USB_1 settings
    usbss_cfg.usb_cfg[1].sd_config.dis_u1u2timerscale = true;
    usbss_cfg.usb_cfg[1].sd_config.sd_mode = DISABLE_ALL_SD;
    usbss_cfg.usb_cfg[1].sd_config.frmscldwn = FRM_SCLDWN_1024;
    usbss_cfg.usb_cfg[1].sd_config.pwrdnscale = 0x07;
    usbss_cfg.usb_cfg[1].configure_phys = 0x0;
    usbss_cfg.usb_cfg[1].scrambling_disable = 0x0;

    usbss_init(usb_init_block, &usbss_cfg);
    sts = atu_unmap(ATU_ID_MSCP, &atu_ioss_map[0]);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
}