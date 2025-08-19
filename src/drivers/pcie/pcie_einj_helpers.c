//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_einj_helpers.c
 * Wrappers around silibs root port subsystem error injection APIs.
 * These are invoked through synchronous requests from the service layer.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <bug_check.h>
#include <cper.h>
#include <pcie_dfwk.h>
#include <pcie_einj_helpers_i.h>
#include <pcie_einj_structs.h>
#include <pcie_phy.h>
#include <pcie_rp_common.h>
#include <pcie_rp_rasdes.h>
#include <pcie_ss.h>
#include <pcie_ss_common.h>
#include <pciess.h>
#include <silibs_status.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static uint8_t map_sbdf_to_root_port(pcie_ss_entity_t* ss, pcie_sbdf_t* sbdf);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static uint8_t map_sbdf_to_root_port(pcie_ss_entity_t* ss, pcie_sbdf_t* sbdf)
{
    uint8_t rp_index = 0xFF;

    for (uint8_t i = 0; i < PCIESS_NUM_PORTS; i++)
    {
        pcie_rp_entity_t* rp = &(ss->rps[i]);
        if (!(rp->valid))
        {
            continue;
        }

        if ((rp->bus_cfg.primary_bus <= sbdf->bus) && (sbdf->bus <= rp->bus_cfg.subordinate_bus))
        {
            rp_index = i;
            break;
        }
    }

    return rp_index;
}

silibs_status_t handle_pcie_error_injection(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;
    ras_einj_info_t* einj_info = (ras_einj_info_t*)(r->p_requested_data);
    pcie_einj_operation_t* op = (pcie_einj_operation_t*)&(einj_info->status_operation);
    pcie_einj_params_t* pcie_params = (pcie_einj_params_t*)&(einj_info->param_type.error_parameters[0]);

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    pcie_rp_entity_t* rp;
    BUG_ASSERT_PARAM((rpss != NULL), r->rpss_index, 0);

    uint8_t rp_index = map_sbdf_to_root_port(rpss, &(pcie_params->sbdf));
    if (rp_index == 0xFF)
    {
        /* No matching (and enabled) root port found for the given SBDF */
        sts = SILIBS_E_PARAM;
        goto done;
    }
    rp = &rpss->rps[rp_index];
    if (op->error_count == 0)
    {
        /* If error count is zero, warn and inject one error */
        FPFW_DBGPRINT_WARNING("RPSS[%d] RP[%d]: Error count is zero, overriding to inject one error.\n", r->rpss_index, rp_index);
        op->error_count = 1;
    }

    switch (op->error_type)
    {
    case PCIE_ERROR_TYPE_AER:
        sts = pcie_ss_rp_aer_spoof(rpss, rp_index, pcie_params->error_data.aer);
        break;
    case PCIE_ERROR_TYPE_RP_INTERNAL_CRC:
        sts = pcie_rp_rasdes_inject_crc_error(rp, pcie_params->error_data.crc, op->error_count);
        break;
    case PCIE_ERROR_TYPE_RP_INTERNAL_SEQNUM:
        sts = pcie_rp_rasdes_inject_seqnum_error(rp, pcie_params->error_data.seqnum, op->error_count);
        break;
    case PCIE_ERROR_TYPE_RP_INTERNAL_DLLP:
        sts = pcie_rp_rasdes_inject_dllp_error(rp, pcie_params->error_data.dllp, op->error_count);
        break;
    case PCIE_ERROR_TYPE_RP_INTERNAL_SYMBOL:
        sts = pcie_rp_rasdes_inject_symbol_error(rp, pcie_params->error_data.symbol, op->error_count);
        break;
    case PCIE_ERROR_TYPE_RP_INTERNAL_FC:
        sts = pcie_rp_rasdes_inject_fc_error(rp, pcie_params->error_data.fc, op->error_count);
        break;
    case PCIE_ERROR_TYPE_RP_INTERNAL_RETRY_TLP:
        sts = pcie_rp_rasdes_inject_retry_tlp_error(rp, pcie_params->error_data.retry_tlp, op->error_count);
        break;
    case PCIE_ERROR_TYPE_RP_INTERNAL_PHY_SET_SRAM_ERROR:
        sts = pcie_phy_setup_error_injection(rp->bases._base,
                                             pcie_params->error_data.phy_sram.phy_inst,
                                             pcie_params->error_data.phy_sram.inj_mask,
                                             pcie_params->error_data.phy_sram.addr_low,
                                             pcie_params->error_data.phy_sram.addr_high);
        break;
    case PCIE_ERROR_TYPE_RP_INTERNAL_PHY_CLEAR_SRAM_ERROR:
        sts = pcie_phy_clear_injection(rp->bases._base, pcie_params->error_data.phy_sram.phy_inst);
        break;
    case PCIE_ERROR_TYPE_VSECRAS:
        sts = ras_pcie_vsecras_agent_trigger_by_type(&rp->vsecras_pagent, pcie_params->error_data.vsecras);
        break;
    case PCIE_ERROR_TYPE_IDE:
        sts = ras_pcie_ide_agent_trigger_by_type(&rp->ide.ide_pagent, pcie_params->error_data.ide);
        break;
    default:
        sts = SILIBS_E_PARAM;
        goto done;
    }

done:
    return sts;
}
