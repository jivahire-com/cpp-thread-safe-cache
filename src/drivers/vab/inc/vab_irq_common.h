//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Definitions and interfaces to handle certain interrupt domain components common
 * to all VABs.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <kng_soc_constants.h>
#include <ras_arm.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef silibs_status_t (*vab_generic_handler_t)(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 *  @brief Probe, print, and clear all error records for the VAB RAS Node
 *
 *  @note This function will not follow a priority scheme and will flush out all errors in
 *        index order.
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_ras_node_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/**
 *  @brief Probe, print, and clear all error records for the VAB TBU0 RAS Node
 *
 *  @note This function will not follow a priority scheme and will flush out all errors in
 *        index order.
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_tbu0_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/**
 *  @brief Probe, print, and clear all error records for the VAB TBU1 RAS Node
 *
 *  @note This function will not follow a priority scheme and will flush out all errors in
 *        index order.
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_tbu1_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/**
 *  @brief Probe, print, and clear all error records for the VAB TCU RAS Node
 *
 *  @note This function will not follow a priority scheme and will flush out all errors in
 *        index order.
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_tcu_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/**
 *  @brief Fetch parity status(es) and produce a CPER record. This event is considered fatal
 *         and a bugcheck will be invoked.
 *
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_fabric_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/**
 *  @brief Fetch parity status(es) and produce a CPER record. This event is considered fatal
 *         and a bugcheck will be invoked.
 *
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_csr_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/**
 *  @brief Fetch parity status(es) and produce a CPER record. This event is considered fatal
 *         and a bugcheck will be invoked.
 *
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_pcr_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/**
 *  @brief Produce a CPER record for VAB Integration-layer PCR parity fault. This event is considered fatal
 *         and a bugcheck will be invoked.
 *
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_integ_pcr_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/**
 *  @brief Produce a CPER record for VAB Integration-layer CSR parity fault. This event is considered fatal
 *         and a bugcheck will be invoked.
 *
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_integ_csr_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/**
 *  @brief Probe, print, and clear all error records for the VAB Tower FMU Nodes
 *
 *  @note This function will not follow a priority scheme and will flush out all errors in
 *        index order.
 *
 *  @param[in] vab_id SUBSYSTEM_WITH_VAB_ID id for the targeted VAB
 *  @param[in] base UINTPTR_T base pointer for the entire VAB subsystem
 *
 *  @return SILIBS_STATUS_T
 */
silibs_status_t vab_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);
