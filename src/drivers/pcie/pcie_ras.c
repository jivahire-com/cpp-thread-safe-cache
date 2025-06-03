//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_ras.c
 * Implements functionality to probe the various RAS nodes that are part
 * of the PCIe RPSS and up level any errors found back to the service.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkPtrTypes.h>
#include <pcie_dfwk.h>
#include <pcie_rp_common.h>
#include <pcie_rp_dtim.h>
#include <pcie_rp_ltim.h>
#include <pcie_rp_vsecras.h>
#include <pciess.h>
#include <ras_common.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
silibs_status_t handle_pcie_vsecras_probe_request(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    pcie_ss_entity_t* rp = pciess_get_entity(r->rpss_index);
    uint8_t rp_index = r->rp_index;
    ras_agent_entity_t* agent = (ras_agent_entity_t*)(&(rp->rps[rp_index].vsecras_pagent));

    if (r->p_requested_data == NULL)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: p_requested_data is NULL!\n", r->rpss_index, r->rp_index);
        return SILIBS_E_PARAM;
    }

    bool ret = ras_pcie_vsecras_agent_probe(agent, (ras_error_record_t*)r->p_requested_data);

    return (ret == true) ? SILIBS_E_DEVICE : SILIBS_SUCCESS;
}

silibs_status_t handle_pcie_dtim_probe_request(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    pcie_ss_entity_t* rp = pciess_get_entity(r->rpss_index);
    uint8_t rp_index = r->rp_index;
    ras_agent_entity_t* agent = (ras_agent_entity_t*)(&(rp->rps[rp_index].psra));

    if (r->p_requested_data == NULL)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: p_requested_data is NULL!\n", r->rpss_index, r->rp_index);
        return SILIBS_E_PARAM;
    }

    bool ret = ras_pcie_dtim_agent_probe(agent, (ras_error_record_t*)r->p_requested_data);

    return (ret == true) ? SILIBS_E_DEVICE : SILIBS_SUCCESS;
}

silibs_status_t handle_pcie_ltim_probe_request(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    pcie_ss_entity_t* rp = pciess_get_entity(r->rpss_index);
    uint8_t rp_index = r->rp_index;
    ras_agent_entity_t* agent = (ras_agent_entity_t*)(&(rp->rps[rp_index].psra));

    if (r->p_requested_data == NULL)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: p_requested_data is NULL!\n", r->rpss_index, r->rp_index);
        return SILIBS_E_PARAM;
    }

    bool ret = ras_pcie_ltim_agent_probe(agent, (ras_error_record_t*)r->p_requested_data);

    return (ret == true) ? SILIBS_E_DEVICE : SILIBS_SUCCESS;
}
