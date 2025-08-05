//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkClient.h>
#include <DfwkStatus.h>
#include <bug_check.h>
#include <kng_soc_constants.h>
#include <pcie_common.h>
#include <pcie_dfwk.h>
#include <pcie_einj_structs.h>
#include <pcie_ss_common.h>
#include <pcie_sync_requests_i.h>
#include <pciess.h>
#include <ras_common.h>
#include <silibs_status.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/*---------- Root port subsystem (RPSS) level synchronous requests- ----------*/
silibs_status_t send_generic_ss_sync_req(PDFWK_INTERFACE_HEADER iface,
                                         RPSS_INSTANCE rpss_idx,
                                         pcie_rp_sync_request_t req,
                                         void* data,
                                         bool no_silibs_check)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if (rpss_idx >= NUM_RPSS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Invalid RPSS index!\n", rpss_idx);
        BUG_ASSERT_PARAM((rpss_idx < NUM_RPSS), rpss_idx, 0);
    }

    pcie_sync_request_t sync_req = {.header = {.RequestType = req}, .rpss_index = rpss_idx, .req_type = req, .p_requested_data = data};

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status) || ((sync_req.status != SILIBS_SUCCESS) && !no_silibs_check))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Failed to send SS request [%d]! dfwk_status: %d, silibs_status: %d\n",
                            rpss_idx,
                            req,
                            dfwk_status,
                            sync_req.status);
        BUG_ASSERT_PARAM((((sync_req.status == SILIBS_SUCCESS) || no_silibs_check) && (dfwk_status == DFWK_SUCCESS)),
                         sync_req.status,
                         dfwk_status);
    }

    return sync_req.status;
}

pcie_ss_entity_t* send_sync_rpss_get_entity(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx)
{
    pcie_ss_entity_t* rpss_ent = NULL;
    silibs_status_t status = send_generic_ss_sync_req(iface, rpss_idx, GET_RPSS_ENTITY_REQUEST, &rpss_ent, false);
    BUG_ASSERT_PARAM((rpss_ent != NULL), status, rpss_idx);
    return rpss_ent;
}

silibs_status_t send_sync_rpss_initial_config(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx)
{
    return send_generic_ss_sync_req(iface, rpss_idx, INITIAL_CONFIG_REQUEST, NULL, false);
}

silibs_status_t send_sync_rpss_pre_rp_init_request(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx)
{
    return send_generic_ss_sync_req(iface, rpss_idx, PRE_RP_INIT_REQUEST, NULL, false);
}

silibs_status_t send_sync_rpss_post_rp_init_request(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx)
{
    return send_generic_ss_sync_req(iface, rpss_idx, POST_RP_INIT_REQUEST, NULL, false);
}

silibs_status_t send_sync_rpss_inject_pcie_error(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx, ras_einj_info_t* einj_payload)
{
    return send_generic_ss_sync_req(iface, rpss_idx, INJECT_PCIE_ERROR, einj_payload, true);
}

/*--------------- Root port (RP) level synchronous requests -----------------*/
silibs_status_t send_generic_rp_sync_req(PDFWK_INTERFACE_HEADER iface,
                                         RPSS_INSTANCE rpss_idx,
                                         uint8_t rp_idx,
                                         pcie_rp_sync_request_t req,
                                         void* data,
                                         bool no_silibs_check)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if ((rpss_idx >= NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Invalid RPSS/RP index!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM(((rpss_idx < NUM_RPSS) && (rp_idx < PCIESS_NUM_PORTS)), rpss_idx, rp_idx);
    }

    pcie_sync_request_t sync_req = {.header = {.RequestType = req},
                                    .rpss_index = rpss_idx,
                                    .rp_index = rp_idx,
                                    .req_type = req,
                                    .p_requested_data = data};

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status) || ((sync_req.status != SILIBS_SUCCESS) && !no_silibs_check))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Failed to send RP request [%d]! dfwk_status: %d, silibs_status: %d\n",
                            rpss_idx,
                            req,
                            dfwk_status,
                            sync_req.status);
        BUG_ASSERT_PARAM((((sync_req.status == SILIBS_SUCCESS) || no_silibs_check) && (dfwk_status == DFWK_SUCCESS)),
                         sync_req.status,
                         dfwk_status);
    }

    return sync_req.status;
}

bool send_sync_rp_is_ready(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx, uint8_t rp_idx)
{
    silibs_status_t status = send_generic_rp_sync_req(iface, rpss_idx, rp_idx, GET_RP_READY_REQUEST, NULL, true);
    return status == SILIBS_SUCCESS;
}

silibs_status_t send_sync_rp_initiate_link_training(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx, uint8_t rp_idx)
{
    return send_generic_rp_sync_req(iface, rpss_idx, rp_idx, INITIATE_LINK_TRAINING, NULL, false);
}

silibs_status_t send_sync_rp_post_link_up_init(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx)
{
    return send_generic_rp_sync_req(iface, rpss_idx, rp_idx, POST_RP_LINK_UP_INIT, NULL, false);
}

silibs_status_t send_sync_rp_get_link_status(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx)
{
    silibs_status_t status = send_generic_rp_sync_req(iface, rpss_idx, rp_idx, GET_LINK_STATUS, NULL, true);
    if (status == SILIBS_E_OVERWRITTEN)
    {
        FPFW_DBGPRINT_WARNING("RPSS[%d] RP[%d]: Link Width mismatch!\n", rpss_idx, rp_idx);
    }
    else if (status == SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d]: Link Trained to Correct Width/Speed!\n", rpss_idx, rp_idx);
    }
    else
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Link Training failure!\n", rpss_idx, rp_idx);
    }
    return status;
}

silibs_status_t send_sync_rp_set_secondary_bus_reset(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx)
{
    return send_generic_rp_sync_req(iface, rpss_idx, rp_idx, SET_SECONDARY_BUS_RESET_REQUEST, NULL, false);
}

silibs_status_t send_sync_rp_clear_secondary_bus_reset(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx)
{
    return send_generic_rp_sync_req(iface, rpss_idx, rp_idx, CLEAR_SECONDARY_BUS_RESET_REQUEST, NULL, false);
}

silibs_status_t send_sync_rp_probe_vsecras(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx, ras_error_record_t* error_record)
{
    BUG_ASSERT(error_record != NULL);
    return send_generic_rp_sync_req(iface, rpss_idx, rp_idx, PROBE_VSECRAS_NODE, error_record, true);
}

silibs_status_t send_sync_rp_probe_dtim(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx, ras_error_record_t* error_record)
{
    BUG_ASSERT(error_record != NULL);
    return send_generic_rp_sync_req(iface, rpss_idx, rp_idx, PROBE_DTIM_NODE, error_record, true);
}

silibs_status_t send_sync_rp_probe_ltim(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx, ras_error_record_t* error_record)
{
    BUG_ASSERT(error_record != NULL);
    return send_generic_rp_sync_req(iface, rpss_idx, rp_idx, PROBE_LTIM_NODE, error_record, true);
}
