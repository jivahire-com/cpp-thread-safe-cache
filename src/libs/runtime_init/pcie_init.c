//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Initializes and starts up PCIe subsystem management drivers and service.
 */

/*------------- Includes -----------------*/
#include <DfwkPtrTypes.h>
#include <DfwkThreadXHost.h>
#include <fpfw_init.h>
#include <scp_pcie_manager.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(pcie, FPFW_INIT_DEPENDENCIES("mesh", "dfwk"))
{
    fpfw_init_component_id_t dfwk_id = "dfwk";
    PDFWK_THREADX_HOST host = fpfw_init_get_handle(dfwk_id);
    scp_pcie_initialize(&(host->Schedule));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
