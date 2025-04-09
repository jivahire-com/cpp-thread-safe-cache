//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_service_public.c
 * Implements the shared  data/functions that are used for signalling PHY FW load
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <bug_check.h>
#include <pcie_phy_load_events.h>

/*--------- Function Prototypes ----------*/
UINT pcie_phyfw_create_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete)
{
    UINT tx_sts = tx_event_flags_create(pcie_phyfw_load_complete, "pcie_phyfw_load_event");
    if (tx_sts != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[PCIe PHY]: Failed to create PCIe phyfw load event! TX_STATUS: %d\n", tx_sts);
        BUG_ASSERT_PARAM((tx_sts == TX_SUCCESS), tx_sts, 0);
    }

    return tx_sts;
}

UINT pcie_phyfw_wait_load_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete)
{
    ULONG event;
    UINT tx_sts = tx_event_flags_get(pcie_phyfw_load_complete, 0x1, TX_AND, &event, TX_WAIT_FOREVER);
    if (tx_sts != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[PCIe PHY]: Failed to wait for PCIe phyfw load event! TX_STATUS: %d\n", tx_sts);
        BUG_ASSERT_PARAM((tx_sts == TX_SUCCESS), tx_sts, 0);
    }

    return tx_sts;
}

UINT pcie_phyfw_set_load_event(TX_EVENT_FLAGS_GROUP* pcie_phyfw_load_complete)
{
    UINT tx_sts = tx_event_flags_set(pcie_phyfw_load_complete, 0x1, TX_OR);
    if (tx_sts != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[PCIe PHY]: Failed to set PCIe phyfw load event! TX_STATUS: %d\n", tx_sts);
        BUG_ASSERT_PARAM((tx_sts == TX_SUCCESS), tx_sts, 0);
    }

    return tx_sts;
}
