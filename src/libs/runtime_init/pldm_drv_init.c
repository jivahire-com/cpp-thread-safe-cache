//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pldm_drv_init.c
 * Initialization of the PLDM driver.
 */

/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h>    // for DFWK_THREADX_HOST
#include <bug_check.h>          // for BUG_ASSERT_PARAM
#include <fpfw_init.h>          // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <idsw.h>               // for idsw_get_cpu_type, idsw_get_die_id
#include <idsw_kng.h>           // for DIE_0, DIE_1
#include <kng_error.h>          // for KNG_SUCCESS
#include <pldm_drv.h>           // for pldm_platform_event_ready_notification
#include <stddef.h>             // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(pldm_drv, FPFW_INIT_DEPENDENCIES("dfwk", "pldm"))
{
    if (idsw_get_die_id() == DIE_0)
    {
        // PLDM driver is only initialized on DIE 0
        DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");

        pldm_device_initialize(&(dfwk_host->Schedule));
        pldm_interface_initialize();

        fpfw_status_t status = pldm_driver_initialize();
        BUG_ASSERT_PARAM(FPFW_STATUS_SUCCESS == status, status, 0);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
