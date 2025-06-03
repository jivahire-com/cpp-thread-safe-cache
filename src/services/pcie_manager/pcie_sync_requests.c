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
pcie_ss_entity_t* send_sync_rpss_get_entity(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx)
{
    pcie_ss_entity_t* rpss_ent = NULL;
    pcie_sync_request_t sync_req = {0};
    int32_t dfwk_status = DFWK_SUCCESS;

    if (rpss_idx >= NUM_RPSS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Invalid RPSS index!\n", rpss_idx);
        BUG_ASSERT_PARAM((rpss_idx < NUM_RPSS), rpss_idx, 0);
    }

    sync_req.p_requested_data = (void*)(&rpss_ent);
    sync_req.rpss_index = rpss_idx;
    sync_req.req_type = GET_RPSS_ENTITY_REQUEST;
    sync_req.header.RequestType = GET_RPSS_ENTITY_REQUEST;
    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status) || (sync_req.status != SILIBS_SUCCESS) || (rpss_ent == NULL))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Error getting RPSS entity! dfwk_status: %d, silibs_status: %d\n",
                            rpss_idx,
                            dfwk_status,
                            sync_req.status);
        BUG_ASSERT_PARAM(((sync_req.status == SILIBS_SUCCESS) && (dfwk_status == DFWK_SUCCESS)), sync_req.status, rpss_idx);
    }

    return rpss_ent;
}

silibs_status_t send_sync_rpss_initial_config(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if (rpss_idx >= NUM_RPSS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Invalid RPSS index!\n", rpss_idx);
        BUG_ASSERT_PARAM((rpss_idx < NUM_RPSS), rpss_idx, 0);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = INITIAL_CONFIG_REQUEST},
        .rpss_index = rpss_idx,
        .req_type = INITIAL_CONFIG_REQUEST,
    };

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status) || (sync_req.status != SILIBS_SUCCESS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Failed to send initial config! dfwk_status: %d, silibs_status: %d\n",
                            rpss_idx,
                            dfwk_status,
                            sync_req.status);
        BUG_ASSERT_PARAM(((sync_req.status == SILIBS_SUCCESS) && (dfwk_status == DFWK_SUCCESS)), sync_req.status, 0);
    }

    return sync_req.status;
}

silibs_status_t send_sync_rpss_pre_rp_init_request(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if (rpss_idx >= NUM_RPSS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Invalid RPSS index!\n", rpss_idx);
        BUG_ASSERT_PARAM((rpss_idx < NUM_RPSS), rpss_idx, 0);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = PRE_RP_INIT_REQUEST},
        .rpss_index = rpss_idx,
        .req_type = PRE_RP_INIT_REQUEST,
    };

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status) || (sync_req.status != SILIBS_SUCCESS))
    {
        FPFW_DBGPRINT_ERROR(
            "RPSS[%d]: Failed to send pre RP init request! dfwk_status: %d, silibs_status: %d\n",
            rpss_idx,
            dfwk_status,
            sync_req.status);
        BUG_ASSERT_PARAM(((sync_req.status == SILIBS_SUCCESS) && (dfwk_status == DFWK_SUCCESS)), sync_req.status, 0);
    }

    return sync_req.status;
}

silibs_status_t send_sync_rpss_post_rp_init_request(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if (rpss_idx >= NUM_RPSS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Invalid RPSS index!\n", rpss_idx);
        BUG_ASSERT_PARAM((rpss_idx < NUM_RPSS), rpss_idx, 0);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = POST_RP_INIT_REQUEST},
        .rpss_index = rpss_idx,
        .req_type = POST_RP_INIT_REQUEST,
    };

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status) || (sync_req.status != SILIBS_SUCCESS))
    {
        FPFW_DBGPRINT_ERROR(
            "RPSS[%d]: Failed to send post RP init request! dfwk_status: %d, silibs_status: %d\n",
            rpss_idx,
            dfwk_status,
            sync_req.status);
        BUG_ASSERT_PARAM(((sync_req.status == SILIBS_SUCCESS) && (dfwk_status == DFWK_SUCCESS)), sync_req.status, 0);
    }

    return sync_req.status;
}

/*--------------- Root port (RP) level synchronous requests -----------------*/
bool send_sync_rp_is_ready(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx, uint8_t rp_idx)
{
    bool rp_ready = false;
    int32_t dfwk_status = DFWK_SUCCESS;

    if ((rpss_idx >= NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Invalid RPSS/RP index!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM(((rpss_idx < NUM_RPSS) && (rp_idx < PCIESS_NUM_PORTS)), rpss_idx, rp_idx);
    }

    /* Send a sync. request to the driver to check if the root port is ready */
    pcie_sync_request_t rp_ready_req = {
        .header = {.RequestType = GET_RP_READY_REQUEST},
        .rpss_index = rpss_idx,
        .rp_index = rp_idx,
        .req_type = GET_RP_READY_REQUEST,
    };

    dfwk_status = DfwkInterfaceSendSync(iface, &rp_ready_req.header);
    if (DFWK_FAILED(dfwk_status))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Failed to send GET_RP_READY_REQUEST! dfwk_status: %d\n", rpss_idx, rp_idx, dfwk_status);
        BUG_ASSERT_PARAM((dfwk_status == DFWK_SUCCESS), dfwk_status, 0);
    }

    /* Check return status and return true/false */
    if (rp_ready_req.status == SILIBS_SUCCESS)
    {
        rp_ready = true;
    }
    else
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Not in ready state! silibs_status: %d\n",
                            rpss_idx,
                            rp_idx,
                            rp_ready_req.status);
        rp_ready = false;
    }

    return rp_ready;
}

silibs_status_t send_sync_rp_initiate_link_training(PDFWK_INTERFACE_HEADER iface, RPSS_INSTANCE rpss_idx, uint8_t rp_idx)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if ((rpss_idx >= NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Invalid RPSS/RP index!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM(((rpss_idx < NUM_RPSS) && (rp_idx < PCIESS_NUM_PORTS)), rpss_idx, rp_idx);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = INITIATE_LINK_TRAINING},
        .rpss_index = rpss_idx,
        .req_type = INITIATE_LINK_TRAINING,
        .rp_index = rp_idx,
    };

    /*
     * This step shouldn't fail as we are only setting the ltssm_en bit on
     * already enabled root ports. We should crash in case of failures here as
     * it mostly implies a logical error in the driver or service.
     */
    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status) || (sync_req.status != SILIBS_SUCCESS))
    {
        FPFW_DBGPRINT_ERROR(
            "RPSS[%d] RP[%d]: Failed to initiate link training! dfwk_status: %d, silibs_status: %d\n",
            rpss_idx,
            rp_idx,
            dfwk_status,
            sync_req.status);
        BUG_ASSERT_PARAM(((sync_req.status == SILIBS_SUCCESS) && (dfwk_status == DFWK_SUCCESS)), sync_req.status, 0);
    }

    return sync_req.status;
}

silibs_status_t send_sync_rp_get_link_status(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if ((rpss_idx >= NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Invalid RPSS/RP index!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM(((rpss_idx < NUM_RPSS) && (rp_idx < PCIESS_NUM_PORTS)), rpss_idx, rp_idx);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = GET_LINK_STATUS},
        .rpss_index = rpss_idx,
        .req_type = GET_LINK_STATUS,
        .rp_index = rp_idx,
    };

    /*
     * OK to encounter a silibs failure here, as there are multiple reasons why
     * link training could fail on an enabled root port.
     * We will just log an error and continue execution.
     */
    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Failed to send GET_RP_READY_REQUEST! dfwk_status: %d\n", rpss_idx, rp_idx, dfwk_status);
        BUG_ASSERT_PARAM((dfwk_status == DFWK_SUCCESS), dfwk_status, 0);
    }

    /* Log link status based on the silibs status returned */
    if (sync_req.status == SILIBS_E_OVERWRITTEN)
    {
        FPFW_DBGPRINT_WARNING("RPSS[%d] RP[%d]: Link Width mismatch!\n", rpss_idx, rp_idx);
    }
    else if (sync_req.status == SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d]: Link Trained to Correct Width/Speed!\n", rpss_idx, rp_idx);
    }
    else
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Link Training failure!\n", rpss_idx, rp_idx);
    }

    return sync_req.status;
}

silibs_status_t send_sync_rp_set_secondary_bus_reset(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if ((rpss_idx >= NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Invalid RPSS/RP index!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM(((rpss_idx < NUM_RPSS) && (rp_idx < PCIESS_NUM_PORTS)), rpss_idx, rp_idx);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = SET_SECONDARY_BUS_RESET_REQUEST},
        .rpss_index = rpss_idx,
        .req_type = SET_SECONDARY_BUS_RESET_REQUEST,
        .rp_index = rp_idx,
    };

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status))
    {
        FPFW_DBGPRINT_ERROR(
            "RPSS[%d] RP[%d]: Failed to send SET_SECONDARY_BUS_RESET_REQUEST! dfwk_status: %d\n",
            rpss_idx,
            rp_idx,
            dfwk_status);
        BUG_ASSERT_PARAM((dfwk_status == DFWK_SUCCESS), dfwk_status, 0);
    }

    return sync_req.status;
}

silibs_status_t send_sync_rp_clear_secondary_bus_reset(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if ((rpss_idx >= NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Invalid RPSS/RP index!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM(((rpss_idx < NUM_RPSS) && (rp_idx < PCIESS_NUM_PORTS)), rpss_idx, rp_idx);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = CLEAR_SECONDARY_BUS_RESET_REQUEST},
        .rpss_index = rpss_idx,
        .req_type = CLEAR_SECONDARY_BUS_RESET_REQUEST,
        .rp_index = rp_idx,
    };

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status))
    {
        FPFW_DBGPRINT_ERROR(
            "RPSS[%d] RP[%d]: Failed to send SET_SECONDARY_BUS_RESET_REQUEST! dfwk_status: %d\n",
            rpss_idx,
            rp_idx,
            dfwk_status);
        BUG_ASSERT_PARAM((dfwk_status == DFWK_SUCCESS), dfwk_status, 0);
    }

    return sync_req.status;
}

silibs_status_t send_sync_rp_probe_vsecras(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx, ras_error_record_t* error_record)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if ((rpss_idx >= NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Invalid RPSS/RP index!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM(((rpss_idx < NUM_RPSS) && (rp_idx < PCIESS_NUM_PORTS)), rpss_idx, rp_idx);
    }

    if (error_record == NULL)
    {
        FPFW_DBGPRINT_ERROR(
            "RPSS[%d] RP[%d]: RAS error record pointer is NULL - cannot probe VSECRAS node!\n",
            rpss_idx,
            rp_idx);
        BUG_ASSERT_PARAM((error_record != NULL), rpss_idx, rp_idx);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = PROBE_VSECRAS_NODE},
        .rpss_index = rpss_idx,
        .req_type = PROBE_VSECRAS_NODE,
        .rp_index = rp_idx,
        .p_requested_data = error_record,
    };

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Failed to send PROBE_VSECRAS_NODE! dfwk_status: %d\n", rpss_idx, rp_idx, dfwk_status);
        BUG_ASSERT_PARAM((dfwk_status == DFWK_SUCCESS), dfwk_status, 0);
    }

    return sync_req.status;
}

silibs_status_t send_sync_rp_probe_dtim(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx, ras_error_record_t* error_record)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if ((rpss_idx >= NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Invalid RPSS/RP index!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM(((rpss_idx < NUM_RPSS) && (rp_idx < PCIESS_NUM_PORTS)), rpss_idx, rp_idx);
    }

    if (error_record == NULL)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: RAS error record pointer is NULL - cannot probe DTIM node!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM((error_record != NULL), rpss_idx, rp_idx);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = PROBE_DTIM_NODE},
        .rpss_index = rpss_idx,
        .req_type = PROBE_DTIM_NODE,
        .rp_index = rp_idx,
        .p_requested_data = error_record,
    };

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Failed to send PROBE_DTIM_NODE! dfwk_status: %d\n", rpss_idx, rp_idx, dfwk_status);
        BUG_ASSERT_PARAM((dfwk_status == DFWK_SUCCESS), dfwk_status, 0);
    }

    return sync_req.status;
}

silibs_status_t send_sync_rp_probe_ltim(PDFWK_INTERFACE_HEADER iface, uint8_t rpss_idx, uint8_t rp_idx, ras_error_record_t* error_record)
{
    int32_t dfwk_status = DFWK_SUCCESS;

    if ((rpss_idx >= NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Invalid RPSS/RP index!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM(((rpss_idx < NUM_RPSS) && (rp_idx < PCIESS_NUM_PORTS)), rpss_idx, rp_idx);
    }

    if (error_record == NULL)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: RAS error record pointer is NULL - cannot probe LTIM node!\n", rpss_idx, rp_idx);
        BUG_ASSERT_PARAM((error_record != NULL), rpss_idx, rp_idx);
    }

    pcie_sync_request_t sync_req = {
        .header = {.RequestType = PROBE_LTIM_NODE},
        .rpss_index = rpss_idx,
        .req_type = PROBE_LTIM_NODE,
        .rp_index = rp_idx,
        .p_requested_data = error_record,
    };

    dfwk_status = DfwkInterfaceSendSync(iface, &sync_req.header);
    if (DFWK_FAILED(dfwk_status))
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Failed to send PROBE_LTIM_NODE! dfwk_status: %d\n", rpss_idx, rp_idx, dfwk_status);
        BUG_ASSERT_PARAM((dfwk_status == DFWK_SUCCESS), dfwk_status, 0);
    }

    return sync_req.status;
}
