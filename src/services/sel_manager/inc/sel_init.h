//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sel_init.h
 * Header containing definitions for the SEL initialization.
 */
#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>      // for DFWK_DEVICE_HEADER, DFWK_INTERFACE_HEADER
#include <fpfw_icc_base.h>  // for fpfw_icc_base_ctx_t
#include <kng_error.h> // for KNG_STATUS
#include <sel.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/


/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize SEL manager queue to accept SEL events
 *
 */
void sel_init();

/**
 * @brief Register ICC channel for SEL manager
 * @param type SEL ICC type
 * @param icc_ctx ICC context
 * @return KNG_STATUS KNG_SUCCESS if succeeded, otherwise error code
 */
KNG_STATUS sel_register_icc(sel_icc_type_t type, fpfw_icc_base_ctx_t *icc_ctx);

/**
 * @brief Register PLDM platform event ready notification for SEL manager
 * @return KNG_STATUS KNG_SUCCESS if succeeded, otherwise error code
 */
KNG_STATUS sel_register_pldm();

/**
 * @brief Initialize SEL manager DFWK device
 * @param schedule DFWK schedule
 */
void sel_init_dfwk_device(PDFWK_SCHEDULE schedule);

/**
 * @brief Initialize SEL manager DFWK interface
 */
void sel_init_dfwk_interface();

/**
 * @brief Initialize SEL manager CLI commands
 */
void sel_cli_init(void);