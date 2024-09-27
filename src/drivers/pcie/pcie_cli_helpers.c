//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Helper functions and utilities to support the PCIe cli.
 */

/*------------- Includes -----------------*/
#include <pcie_dfwk.h>
#include <pcie_rp_common.h>
#include <pcie_rp_dbi.h>
#include <pcie_rp_sii.h>
#include <pcie_ss_common.h>
#include <pciess.h>
#include <rc4sx16_pf0_type1_hdr_regs.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void get_rp_link_state(pcie_sync_request_t* r)
{
    if (r->p_requested_data == NULL)
    {
        r->status = SILIBS_E_PARAM;
        return;
    }
    pcie_ss_entity_t* ss = pciess_get_entity(r->rpss_index);
    pcie_link_state_t** link_state = (pcie_link_state_t**)(r->p_requested_data);
    pcie_rp_dbi_check_link_state(&(ss->rps[r->rp_index]), false);
    *link_state = &(ss->rps[r->rp_index].current_state);
    r->status = SILIBS_SUCCESS;
}

static void get_rp_ltssm_state(pcie_sync_request_t* r)
{
    if (r->p_requested_data == NULL)
    {
        r->status = SILIBS_E_PARAM;
        return;
    }
    pcie_ss_entity_t* ss = pciess_get_entity(r->rpss_index);
    PCIE_LTSSM_STATE* ltssm_val = (PCIE_LTSSM_STATE*)(r->p_requested_data);
    *(ltssm_val) = (PCIE_LTSSM_STATE)pcie_rp_sii_get_link_state(&(ss->rps[r->rp_index]));
    r->status = SILIBS_SUCCESS;
}

static void get_rp_dbi_cfg_hdr(pcie_sync_request_t* r)
{
    if (r->p_requested_data == NULL)
    {
        r->status = SILIBS_E_PARAM;
        return;
    }
    pcie_ss_entity_t* ss = pciess_get_entity(r->rpss_index);
    pcie_rp_entity_t* rp = &(ss->rps[r->rp_index]);
    rc4sx16_pf0_type1_hdr_reg** t1 = (rc4sx16_pf0_type1_hdr_reg**)(r->p_requested_data);
    *t1 = (rc4sx16_pf0_type1_hdr_reg*)(rp->bases.dbi_base_addr + rp->offsets.type1_hdr);
    r->status = SILIBS_SUCCESS;
}

void handle_cli_request(pcie_sync_request_t* r)
{
    pcie_cli_req_op_t* p_cli_req = (pcie_cli_req_op_t*)(r->p_sent_data);
    if (p_cli_req == NULL)
    {
        printf("%s: p_cli_req is NULL - cannot process cli request!\n", __FUNCTION__);
        return;
    }

    switch (*p_cli_req)
    {
    case GET_RP_LTSSM_STATE:
        get_rp_ltssm_state(r);
        break;
    case GET_RP_LINK_STATE:
        get_rp_link_state(r);
        break;
    case GET_RP_DBI_CONFIG_HDR:
        get_rp_dbi_cfg_hdr(r);
        break;
    default:
        printf("%s: Unknown CLI_REQUEST OP: %d\n", __FUNCTION__, *(p_cli_req));
        break;
    }
}
