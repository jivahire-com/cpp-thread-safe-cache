//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_sync_requests_i.h
 * Implements common routines to send synchronous requests to the PCIe driver.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkPtrTypes.h>
#include <pcie_einj_structs.h>
#include <pcie_ss_common.h>
#include <silibs_status.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/*---------- Root port subsystem (RPSS) level synchronous requests- ----------*/
/**
 * @brief Send a synchronous request to get the RPSS entity for the specified RPSS instance.
 *
 * @param[in] iface     - Pointer to the driver interface header for this RPSS.
 * @param[in] rpss_idx  - The RPSS instance index.
 *
 * @retval pcie_ss_entity_t*: Pointer to the PCIe subsystem entity structure.
 */
pcie_ss_entity_t* send_sync_rpss_get_entity(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx);

/**
 * @brief Send a synchronous request to initiate the initial configuration of the RPSS.
 *
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] iface     - The interface header for the PCIe subsystem.
 *
 * @retval SILIBS_SUCCESS: Initial configuration sent successfully.
 * @retval Other:          Error occurred while sending initial configuration.
 */
silibs_status_t send_sync_rpss_initial_config(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx);

/**
 * @brief Send a synchronous request to initiate the RPSS pre-RP initialization
 *        sequence.
 *
 * @param[in] iface     - Pointer to the driver interface header for this RPSS.
 * @param[in] rpss_idx  - The RPSS instance index.
 *
 * @retval SILIBS_SUCCESS: Pre-RP initialization sent successfully.
 * @retval Other:          Error occurred while sending pre-RP initialization.
 */
silibs_status_t send_sync_rpss_pre_rp_init_request(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx);

/**
 * @brief Send a synchronous request to initiate the RPSS post-RP initialization.
 *
 * @param[in] iface     - Pointer to the driver interface header for this RPSS.
 * @param[in] rpss_idx  - The RPSS instance index.
 *
 * @retval SILIBS_SUCCESS: Post-RP initialization sent successfully.
 * @retval Other:          Error occurred while sending post-RP initialization.
 */
silibs_status_t send_sync_rpss_post_rp_init_request(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx);

/**
 * @brief Send a synchronous request to inject a PCIe error for the specified RPSS instance.
 *
 * @param[in] iface     - Pointer to the driver interface header for this RPSS.
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] einj_payload - Pointer to the EINJ payload structure.
 *
 * @retval SILIBS_SUCCESS: Error injection request sent successfully.
 * @retval Other:          Error occurred while sending error injection request.
 */
silibs_status_t send_sync_rpss_inject_pcie_error(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx, ras_einj_info_t* einj_payload);

/**
 * @brief Send a synchronous request to check if the RPSS is ready.
 *
 * @param[in] iface     - Pointer to the driver interface header for this RPSS.
 * @param[in] rpss_idx  - The RPSS instance index.
 *
 * @retval SILIBS_SUCCESS: RPSS is ready.
 * @retval SILIBS_E_BUSY:  RPSS is not ready or an error occurred.
 */
silibs_status_t send_sync_rpss_get_ready_request(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx);

/*--------------- Root port (RP) level synchronous requests -----------------*/
/**
 * @brief Send a synchronous request to check if the RP is ready.
 *
 * @param[in] iface     - Pointer to the driver interface header for this RPSS.
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] rp_index  - The root port index.
 *
 * @retval true:  RP is ready.
 * @retval false: RP is not ready.
 */
bool send_sync_rp_is_ready(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx, uint8_t rp_idx);

/**
 * @brief Send a synchronous request to initiate link training on the specified RP.
 *
 * @param[in] iface     - Pointer to the driver interface header for this RPSS.
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] rp_index  - The root port index.
 *
 * @retval SILIBS_SUCCESS: Link training initiated successfully.
 * @retval Other:          Error occurred while initiating link training.
 */
silibs_status_t send_sync_rp_initiate_link_training(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx, uint8_t rp_idx);

/**
 * @brief Send a synchronous request to get the link status of the specified RP.
 *
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] rp_index  - The root port index.
 * @param[in] lt_retry_count - pointer to retry count for the RP 
 *
 * @retval SILIBS_SUCCESS: Link status obtained successfully.
 * @retval Other:          Error occurred while obtaining link status.
 */
silibs_status_t send_sync_rp_get_link_status(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx, uint8_t *lt_retry_count);

/**
 * @brief Send a synchronous request to carry out any initialization required on the RP after link-up.
 *
 * @param[in] iface     - Pointer to the driver interface header for this RPSS.
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] rp_index  - The root port index.
 *
 * @retval SILIBS_SUCCESS: Initialization after link-up sent successfully.
 * @retval Other:          Error occurred while sending initialization after link-up.
 */
silibs_status_t send_sync_rp_post_link_up_init(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx);

/**
 * @brief Send a synchronous request to set the secondary bus reset for the specified RP.
 *
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] rp_index  - The root port index.
 *
 * @retval SILIBS_SUCCESS: SBR bit set successfully.
 * @retval Other:          Error occurred while setting SBR bit.
 */
silibs_status_t send_sync_rp_set_secondary_bus_reset(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx);

/**
 * @brief Send a synchronous request to clear the secondary bus reset for the specified RP.
 *
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] rp_index  - The root port index.
 *
 * @retval SILIBS_SUCCESS: SBR bit cleared successfully.
 * @retval Other:          Error occurred while clearing SBR bit.
 */
silibs_status_t send_sync_rp_clear_secondary_bus_reset(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx);

/**
 * @brief Send a synchronous request to spoof an AER Uncorrectable Internal Error to bring the link down.
 *
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] rp_index  - The root port index.
 *
 * @retval SILIBS_SUCCESS: SBR bit cleared successfully.
 * @retval Other:          Error occurred while clearing SBR bit.
 */
silibs_status_t send_sync_rp_force_aer_internal_uncorr_error(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx);

/**
 * @brief Send a synchronous request to notify the RMM to perform stream rekeying for any TX stream that needs a key.
 *
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] rp_index  - The root port index.
 *
 * @retval SILIBS_SUCCESS: SBR bit cleared successfully.
 * @retval Other:          Error occurred while clearing SBR bit.
 */
silibs_status_t send_sync_rp_tx_rekey(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx);

/**
 * @brief Send a synchronous request to notify the RMM to perform stream rekeying for any RX stream that needs a key.
 *
 * @param[in] rpss_idx  - The RPSS instance index.
 * @param[in] rp_index  - The root port index.
 *
 * @retval SILIBS_SUCCESS: SBR bit cleared successfully.
 * @retval Other:          Error occurred while clearing SBR bit.
 */
silibs_status_t send_sync_rp_rx_rekey(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx);
