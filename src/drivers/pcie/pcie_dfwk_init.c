//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Instantiates PCIe root port devices using SCP driver framework.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <bug_check.h>
#include <oi_pcie.h>
#include <pcie_cli_helpers_i.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <pcie_einj_helpers_i.h>
#include <pcie_ras_i.h>
#include <pcie_rpss_init_i.h>
#include <pcie_ss_common.h>
#include <pciess.h>
#include <pciess_int.h>
#include <silibs_status.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
int32_t pcie_sched_sync_op(PDFWK_SYNC_REQUEST_HEADER incoming)
{
    /* No null check required here as DFWK handles it for us */
    pcie_sync_request_t* r = (pcie_sync_request_t*)incoming;
    silibs_status_t sts = SILIBS_SUCCESS;
    pcie_ss_entity_t* rpss;

    if (r->rp_index >= PCIESS_NUM_PORTS || r->rpss_index >= PCIE_NUM_RPSS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: pcie sync request out of bounds!\n", r->rpss_index, r->rp_index);
        r->status = SILIBS_E_PARAM;
        return 0;
    }

    switch (r->header.RequestType)
    {
    case (INITIAL_CONFIG_REQUEST):
        FPFW_DBGPRINT_INFO("RPSS[%d]: Begin initial programming!\n", r->rpss_index);
        sts = begin_rpss_init(incoming);
        break;
    case (PRE_RP_INIT_REQUEST):
        FPFW_DBGPRINT_INFO("RPSS[%d]: Begin pre-rp ready programming!\n", r->rpss_index);
        sts = begin_rpss_pre_rp_ready_init(incoming);
        break;
    case (POST_RP_INIT_REQUEST):
        FPFW_DBGPRINT_INFO("RPSS[%d]: Begin post-rp ready programming!\n", r->rpss_index);
        sts = begin_rpss_post_rp_ready_init(incoming);
        break;
    case (INITIATE_LINK_TRAINING):
        FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d]: Begin link training!\n", r->rpss_index, r->rp_index);
        begin_link_training(incoming);
        break;
    case (GET_RPSS_ENTITY_REQUEST):
        r->status = SILIBS_E_PARAM;
        if (r->p_requested_data != NULL)
        {
            *((pcie_ss_entity_t**)(r->p_requested_data)) = pciess_get_entity(r->rpss_index);
            r->status = SILIBS_SUCCESS;
        }
        break;
    case (GET_RP_READY_REQUEST):
        sts = get_rp_ready(incoming);
        break;
    case (GET_LINK_STATUS):
        sts = get_rp_link_status(incoming);
        break;
    case (SET_SECONDARY_BUS_RESET_REQUEST):
        rpss = pciess_get_entity(r->rpss_index);
        sts = oi_pcie_rp_dbi_set_secondary_bus_reset(&rpss->rps[r->rp_index]);
        break;
    case (CLEAR_SECONDARY_BUS_RESET_REQUEST):
        rpss = pciess_get_entity(r->rpss_index);
        sts = oi_pcie_rp_dbi_reset_secondary_bus_reset(&rpss->rps[r->rp_index]);
        break;
    case (PROBE_VSECRAS_NODE):
        sts = handle_pcie_vsecras_probe_request(incoming);
        break;
    case (PROBE_DTIM_NODE):
        sts = handle_pcie_dtim_probe_request(incoming);
        break;
    case (PROBE_LTIM_NODE):
        sts = handle_pcie_ltim_probe_request(incoming);
        break;
    case (CLI_REQUEST):
        handle_cli_request(r);
        break;
    case (INJECT_PCIE_ERROR):
        sts = handle_pcie_error_injection(incoming);
        break;
    default:
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Bad sync req received!\n", r->rpss_index);
        sts = SILIBS_E_PARAM;
        break;
    }
    r->status = sts;

    return 0;
}

void pcie_dfwk_interface_init(pciess_device_t* dev, pciess_device_interface_t* iface)
{
    BUG_ASSERT(dev != NULL);
    BUG_ASSERT(iface != NULL);

    DfwkInterfaceInitialize(&iface->header, &dev->header, &dev->default_queue, pcie_sched_sync_op);
    iface->dev = dev;
}

void pcie_dfwk_init(pciess_device_t* dev, PDFWK_SCHEDULE schedule)
{
    BUG_ASSERT(dev != NULL);
    BUG_ASSERT(schedule != NULL);

    BUG_ASSERT(init_async_request_pool() == TX_SUCCESS);
    DfwkDeviceInitialize(&(dev->header), schedule);
    DfwkQueueInitialize(&(dev->default_queue), &(dev->header), pcie_default_dispatch, NULL, DfwkQueueType_ImmediateDispatch);

    for (uint8_t i = 0; i < PCIESS_NUM_PORTS; i++)
    {
        DfwkQueueInitialize(&(dev->per_rp_queue[i]), &(dev->header), pcie_per_rp_dispatch, NULL, DfwkQueueType_SerializedDispatch);
    }
}
