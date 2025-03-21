//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_service_public.c
 * Implements the shared  data/functions that are used for signalling PHY FW load
 */

/*------------- Includes -----------------*/
#include <pcie_phy_load_events.h> // Phy FW load event and related functions

/*--------- Function Prototypes ----------*/
/**
 *  @brief      Used to create the shared event to indicate PCIE Phy FW load
 *
 *  @param[in]  pcie_phyfw_load_complete  ThreadX primitive event
 *
 *  @retval     TX_SUCCESS on no error
 *
 *
 */
UINT pcie_phyfw_create_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete)
{
    UINT txStatus;
    // Create an event flags group.
    txStatus = tx_event_flags_create(pcie_phyfw_load_complete, "pcie_phyfw_load_event");
    FPFW_RUNTIME_ASSERT(txStatus == TX_SUCCESS);

    return txStatus;
}

/**
 *  @brief      Wait for Pci PhyFW load event to be set
 *
 *  @param[in]  pcie_phyfw_load_complete  ThreadX primitive event
 *
 *  @retval     TX_SUCCESS on no error
 *
 *
 */
UINT pcie_phyfw_wait_load_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete)
{
    ULONG event;
    UINT txStatus;
    txStatus = tx_event_flags_get(pcie_phyfw_load_complete, 0x1, TX_AND, &event, TX_WAIT_FOREVER);
    FPFW_RUNTIME_ASSERT(txStatus == TX_SUCCESS);

    return txStatus;
}

/**
 *  @brief      Set Pci PhyFW load event
 *
 *  @param[in]  pcie_phyfw_load_complete  ThreadX primitive event
 *
 *  @retval     TX_SUCCESS on no error
 *
 *
 */
UINT pcie_phyfw_set_load_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete)
{
    UINT txStatus = tx_event_flags_set(pcie_phyfw_load_complete, 0x1, TX_OR);
    FPFW_RUNTIME_ASSERT(txStatus == TX_SUCCESS);

    return txStatus;
}
