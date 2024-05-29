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
#include <stdbool.h>    // for false
#include <stdint.h>     // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void usb_init(uint32_t usb_init_block)
{
    if (usb_init_block != USBSS_INIT_USB2_0 && usb_init_block != USBSS_INIT_USB2_1)
    {
        FPFW_RUNTIME_ASSERT(false);
    }

    usbss_cfg_t usbss_cfg = {};
    usbss_cfg.usbss_base_address = 0x01;

    // USB_0 settings
    usbss_cfg.usb_cfg[0].sd_config.dis_u1u2timerscale = false;
    usbss_cfg.usb_cfg[0].sd_config.sd_mode = DISABLE_ALL_SD;
    usbss_cfg.usb_cfg[0].sd_config.frmscldwn = FRM_SCLDWN_1024;
    usbss_cfg.usb_cfg[0].sd_config.pwrdnscale = 0x07;
    usbss_cfg.usb_cfg[0].configure_phys = 0x0;
    usbss_cfg.usb_cfg[0].scrambling_disable = 0x0;

    // USB_1 settings
    usbss_cfg.usb_cfg[1].sd_config.dis_u1u2timerscale = false;
    usbss_cfg.usb_cfg[1].sd_config.sd_mode = DISABLE_ALL_SD;
    usbss_cfg.usb_cfg[1].sd_config.frmscldwn = FRM_SCLDWN_1024;
    usbss_cfg.usb_cfg[1].sd_config.pwrdnscale = 0x07;
    usbss_cfg.usb_cfg[1].configure_phys = 0x0;
    usbss_cfg.usb_cfg[1].scrambling_disable = 0x0;

    usbss_init(usb_init_block, &usbss_cfg);
}