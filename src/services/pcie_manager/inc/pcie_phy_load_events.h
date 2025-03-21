//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_service_public.h
 * Implements the shared  data/functions that are used for signalling PHY FW load
 */

#pragma once
/*------------- Includes -----------------*/
#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <assert.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send, fpfw_icc_base...
#include <fpfw_init.h>     // for fpfw_init_get_handle, FPFW_INIT_S...
#include <fpfw_status.h>   // for fpfw_init_get_handle, FPFW_INIT_S...
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

// Shared event to indicate Pcie PhyFW load complete.
// This is done from ap_core service which manages all the loading from flash.
extern TX_EVENT_FLAGS_GROUP pcie_phyfw_load_event;

/**
 *  @brief      Used to create the shared event to indicate PCIE Phy FW load
 *
 *  @param[in]  pcie_phyfw_load_complete  ThreadX primitive event
 *
 *  @retval     TX_SUCCESS on no error
 *
 *
 */
UINT pcie_phyfw_create_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete);

/**
 *  @brief      Wait for Pci PhyFW load event to be set
 *
 *  @param[in]  pcie_phyfw_load_complete  ThreadX primitive event
 *
 *  @retval     TX_SUCCESS on no error
 *
 *
 */
UINT pcie_phyfw_wait_load_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete);

/**
 *  @brief      Set Pci PhyFW load event
 *
 *  @param[in]  pcie_phyfw_load_complete  ThreadX primitive event
 *
 *  @retval     TX_SUCCESS on no error
 *
 *
 */
UINT pcie_phyfw_set_load_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete);
