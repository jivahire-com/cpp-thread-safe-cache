//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Private APIs implemented by this driver for PCIe root port subsystem
 * initialization
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pcie_dfwk.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief Begin rpss initialization. This will call silibs APIs to
 *         setup internal pciess configuration structures and configures
 *         SS PCR, SS Tower APU & SAM, SS 1P Regs in SS, SS 3P Regs.
 *
 *  @param[in] req  Synchronous request packet received by the pciess
 *                  driver which contains information on how to
 *                  configure this pciess instance.
 *
 *  @return silibs_status_t returned by the silibs API
 */
int begin_rpss_init(PDFWK_SYNC_REQUEST_HEADER req);

/**
 *  @brief This will call silibs APIs to setup INTU, FIPS on a per RP basis
 *         before the RP sub-blocks are up. Also responsible for ensuring PHY
 *         SRAMs are initialized as expected.
 *
 *  @param[in] req  Synchronous request packet received by the pciess
 *                  driver which contains information on how to
 *                  configure this pciess instance.
 *
 *  @return silibs_status_t returned by the silibs API
 */
int begin_rpss_pre_rp_ready_init(PDFWK_SYNC_REQUEST_HEADER req);

/**
 *  @brief This routine ensures that RP sub-blocks are ready for
 *         configuration and programs each sub-block by calling the
 *         top-level silibs API responsible.
 *
 *  @param[in] req  Synchronous request packet received by the pciess
 *                  driver which contains information on how to
 *                  configure this pciess instance.
 *
 *  @return silibs_status_t returned by the silibs API
 */
int begin_rpss_post_rp_ready_init(PDFWK_SYNC_REQUEST_HEADER req);

/**
 *  @brief This routine that the Link Training is started for a 
 *         enabled RP. This calls into underlying SILIBS functions 
 *         to enable Link Training.
 *
 *  @param[in] req  Synchronous request packet received by the pciess
 *                  driver which contains information on how to
 *                  configure this pciess instance.
 *
 *  @return silibs_status_t returned by the silibs API
 */
int begin_link_training(PDFWK_SYNC_REQUEST_HEADER req);