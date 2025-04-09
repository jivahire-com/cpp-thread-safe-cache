//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_service_public.h
 * Implements the shared data/functions that are used for signalling PHY FW load
 */

#pragma once
/*------------- Includes -----------------*/
#include <tx_api.h>

/*
 * Global event to signal PCIe phy firmware load completion.
 * This is done from the ap_core service which manages all the loading from flash.
 */
extern TX_EVENT_FLAGS_GROUP pcie_phyfw_load_event;

/**
 *  @brief      Used to create a shared ThreadX event to indicate PCIe phy
 *              firmware load completion.
 *
 *  @param[in]  pcie_phyfw_load_complete  ThreadX primitive event
 *
 *  @retval     TX_SUCCESS on no error
 *              in case of errors, FpFwErrorRaise is called to crash the system
 */
UINT pcie_phyfw_create_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete);

/**
 *  @brief      Wait indefinitely for the PCIe phy firmware load event to be set.
 *
 *  @param[in]  pcie_phyfw_load_complete  ThreadX primitive event to wait on
 *
 *  @retval     TX_SUCCESS on no error
 *              in case of errors, FpFwErrorRaise is called to crash the system
 */
UINT pcie_phyfw_wait_load_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete);

/**
 *  @brief      Set PCIe phy firmware load event to indicate that the phy
 *              firmware has been loaded from flash.
 *
 *  @param[in]  pcie_phyfw_load_complete  ThreadX primitive event to set
 *
 *  @retval     TX_SUCCESS on no error
 *              in case of errors, FpFwErrorRaise is called to crash the system
 */
UINT pcie_phyfw_set_load_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete);
