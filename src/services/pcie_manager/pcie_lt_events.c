//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_lt_events.c
 * Implements the shared data/functions that are used for signalling link training
 * completion.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <bug_check.h>
#include <pcie_lt_events.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PCI_LT_EVENT_TIMEOUT_TICKS (60000)

/*--------- Function Prototypes ----------*/
UINT pcie_link_training_create_event(TX_EVENT_FLAGS_GROUP* pcie_lt_event)
{
    UINT tx_sts = tx_event_flags_create(pcie_lt_event, "pcie_lt_event");
    if (tx_sts != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[PCIe]: Failed to create PCIe link training event! TX_STATUS: %d\n", tx_sts);
        BUG_ASSERT_PARAM((tx_sts == TX_SUCCESS), tx_sts, 0);
    }

    return tx_sts;
}

UINT pcie_link_training_wait_event(TX_EVENT_FLAGS_GROUP* pcie_lt_event)
{
    ULONG event;
    UINT tx_sts = tx_event_flags_get(pcie_lt_event, 0x1, TX_AND, &event, PCI_LT_EVENT_TIMEOUT_TICKS);
    if (tx_sts != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR(
            "[PCIe]: PCIe link training event was not set in time - booting without PCIe! TX_STATUS: %d\n",
            tx_sts);
    }

    return tx_sts;
}

UINT pcie_link_training_set_event(TX_EVENT_FLAGS_GROUP* pcie_lt_event)
{
    UINT tx_sts = tx_event_flags_set(pcie_lt_event, 0x1, TX_OR);
    if (tx_sts != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[PCIe]: Failed to set PCIe link training done event! TX_STATUS: %d\n", tx_sts);
        BUG_ASSERT_PARAM((tx_sts == TX_SUCCESS), tx_sts, 0);
    }

    return tx_sts;
}
