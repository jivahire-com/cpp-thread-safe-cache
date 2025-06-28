//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_lt_events.h
 * Implements the shared data/functions that are used for signalling that
 * link training sequence has begun.
 */

#pragma once
/*------------- Includes -----------------*/
#include <tx_api.h>

/*
 * Global event to signal PCIe link training sequence has begun.
 * This is done from the ap_core service which manages all the loading from flash.
 */
extern TX_EVENT_FLAGS_GROUP pcie_lt_event;

/**
 *  @brief      Used to create a shared ThreadX event to indicate PCIe link
 *              training has been issued.
 *
 *  @param[in]  pcie_lt_event  ThreadX primitive event
 *
 *  @retval     TX_SUCCESS on no error
 *              in case of errors, FpFwErrorRaise is called to crash the system
 */
UINT pcie_link_training_create_event(TX_EVENT_FLAGS_GROUP* pcie_lt_event);

/**
 *  @brief      Wait for PCIe init sequences to complete till link training.
 *
 *  @param[in]  pcie_lt_event  ThreadX primitive event to wait on
 *
 *  @retval     TX_SUCCESS on no error
 *              in case of errors, FpFwErrorRaise is called to crash the system
 */
UINT pcie_link_training_wait_event(TX_EVENT_FLAGS_GROUP* pcie_lt_event);

/**
 *  @brief      Set link training event to indicate that link training has
 *              been initiated.
 *
 *  @param[in]  pcie_lt_event  ThreadX primitive event to set
 *
 *  @retval     TX_SUCCESS on no error
 *              in case of errors, FpFwErrorRaise is called to crash the system
 */
UINT pcie_link_training_set_event(TX_EVENT_FLAGS_GROUP* pcie_lt_event);