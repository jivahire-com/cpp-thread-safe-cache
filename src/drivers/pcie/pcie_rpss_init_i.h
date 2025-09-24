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
#define COHERENCY_READ_WRITE_DOMAIN_MODE  (0x3) /* 2’b11 */
#define COHERENCY_READ_WRITE_CACHE_MODE   (0xF) /* 4’b1111 */
#define COHERENCY_READ_WRITE_DOMAIN_VALUE (0x2) /* 2’b10  Sharable Domain */
#define COHERENCY_READ_WRITE_CACHE_VALUE  (0xF) /* 4’b1111  Write-Back Read/Write*/

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
 *  @brief Get ready status for all rpss on a given RPSS.
 *         Used to ascertain if we are ready to start post RP-ready init.
 *
 *  @param[in] req  Synchronous request packet received by the pciess
 *                  driver which contains information about this pciess
 *                  instance.
 *
 *  @return silibs_success: The RPSS is ready
 *          silibs_e_busy:  The RPSS is not ready yet.
 */
int get_rpss_ready(PDFWK_SYNC_REQUEST_HEADER req);

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
 *  @brief This routine starts link training for an enabled RP by
 *         calling into underlying silibs APIs for it.
 *
 *  @param[in] req  Synchronous request packet received by the pciess
 *                  driver which contains information on how to
 *                  configure this pciess instance.
 *
 *  @return None
 */
void begin_link_training(PDFWK_SYNC_REQUEST_HEADER req);

/**
 * @brief This routine will do post link up programming for an RP.
 *        Sets up the RP's slot capabilities register set
 *        for now and initializes IDE if enabled.
 *
 * @param[in] req  Synchronous request packet received by the pciess
 *                 driver which contains information on how to
 *                 configure this pciess instance.
 *
 * @return silibs_status_t returned by the silibs API
 */
int begin_rp_post_link_up_init(PDFWK_SYNC_REQUEST_HEADER req);

/**
 *  @brief Get ready status for a given RP.
 *         Used to ascertain if we are ready to start link training by checking
 *         the cdm_in_reset bit on an enabled RP. To be used when responding to
 *         a link down event.
 *
 *  @param[in] req  Synchronous request packet received by the pciess
 *                  driver which contains information about this pciess
 *                  instance.
 *
 *  @return silibs_success: The RP is ready
 *          silibs_e_busy:  The RP is not ready yet.
 */
int get_rp_ready(PDFWK_SYNC_REQUEST_HEADER req);

/**
 *  @brief Get link status for a given RP.
 *
 *  @param[in] req  Synchronous request packet received by the pciess
 *                  driver which contains information about this pciess
 *                  instance.
 *
 * @retval silibs_success: The link is up and has trained to the desired speed
 *                         and width.
 *
 * @retval silibs_e_overwritten:  The link trained but the speed and width
 *                                do not match the expected configuration.
 *
 * @retval any other silibs_errors:  The link failed to train.
 */
int get_rp_link_status(PDFWK_SYNC_REQUEST_HEADER req);
