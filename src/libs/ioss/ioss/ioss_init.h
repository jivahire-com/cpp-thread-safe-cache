//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ioss_init.h
 *  Initializes IOSS
 */

#pragma once

/*------------- Includes -----------------*/
#include <stdint.h>             // for uint8_t

/*-- Symbolic Constant Macros (defines) --*/
#define USBSS_NUM_USB_CTRLS (0x2)

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------- Function Prototypes -----------*/

/**
 *  @brief Inits the IOSS
 * 
 *  @param[in] die_num
 *      Current die number
 *
 *  @return
 *      None
 */
void ioss_init(uint8_t die_num);


