//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * SCP service responsible for PCIe root port subsystem management.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkThreadXHost.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PCIE_OUTSTANDING_REQ_QUOTA (4) /* This allows up to 4 outstanding async. requests per rpss */

/*-------------- Typedefs ----------------*/

/* PCIe completion request format - this is what is used by the service for internal communication */
typedef struct _pciess_completion_request_t
{
    pcie_rp_async_request_t op;
    uint8_t rp_index;
    silibs_status_t status;
} pciess_completion_request_t;

/* This is the thread context block for each RPSS management thread */
typedef struct _pcie_manager_context_t
{
    RPSS_INSTANCE rpss_idx;
    pciess_device_t* dev;
    pciess_device_interface_t* iface;
    pcie_async_request_t async_req[ROOT_PORTS_PER_RPSS];
    pciess_completion_request_t cmpl_req[PCIE_OUTSTANDING_REQ_QUOTA];
    TX_THREAD worker;
    TX_QUEUE work_queue;
} pcie_manager_context_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief      Used to initialize the PCIe subsystem management service and
 *              drivers on SCP.
 *
 *  @param[in]  schedule  Driver framework scheduler being used by the host.
 *
 *  @retval     None. Errors raised if failure in initialization.
 */
void scp_pcie_initialize(PDFWK_SCHEDULE schedule);
