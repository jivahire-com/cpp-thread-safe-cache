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
#include <DfwkDriver.h>
#include <bug_check.h>
#include <kng_soc_constants.h>
#include <pcie_lt_events.h>
#include <startup_shutdown_ssi.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pssi_startup_notification_request_t req;
static bool req_pending = false;
static bool ltssm_en_set = false;

/*------------- Functions ----------------*/
void cache_ssi_ltssm_startup_request(pssi_startup_notification_request_t ltssm_req)
{
    req = ltssm_req;
    req_pending = true;
}

void complete_ssi_ltssm_startup_req(RPSS_INSTANCE rpss_idx)
{
    if (ltssm_en_set == false)
    {
        ltssm_en_set = true;
    }

    if (req_pending == true)
    {
        FPFW_DBGPRINT_INFO("RPSS[%d]: Completing link training startup request\n",
                           rpss_idx); // Print kept as this is invoked only on PCIESS Initialization.
        DfwkAsyncRequestComplete(&(req->header));
        req_pending = false;
    }
}

bool scp_is_pcie_ltssm_en_set()
{
    return ltssm_en_set;
}
