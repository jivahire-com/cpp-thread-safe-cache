//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file usb.h
 *  Initializes USBSS
 */

#pragma once

/*------------- Includes -----------------*/
#include <silibs_platform.h>

/*-- Symbolic Constant Macros (defines) --*/

/**
 * @brief USB subsystem init flags controlling which blocks get initialized in the
 *        usbss_init api
 */
#define USBSS_INIT_USB2_0 (BIT0)
#define USBSS_INIT_USB2_1 (BIT1)

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------- Function Prototypes -----------*/

/**
 *  @brief Inits the USBSS
 * 
 *  @param[in] usb_init_block
 *      Bitmask representing which USB blocks to init
 *
 *  @return
 *      None
 */
void usb_init(uint32_t usb_init_block);