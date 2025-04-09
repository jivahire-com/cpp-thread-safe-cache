//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe rp event processing helper function
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkClient.h>   // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FpFwAssert.h>   // for FPFW_RUNTIME_ASSERT
#include <idsw_kng.h>
#include <pcie_config_variable.h>
#include <pcie_dfwk.h>      // for pcie_async_request_t, pcie_dfwk_interf...
#include <pcie_manager_i.h> // for rpss_req_completion_cb, rpss_service_t...
#include <pcie_ss_common.h> // pciess_entity
#include <pciess.h>
#include <pciess_int.h>       // pcie Interrupts
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <silibs_kng_soc.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <tx_api.h>  // for TX_WAIT_FOREVER, ULONG, tx_queue_receive

/*-- Symbolic Constant Macros (defines) --*/
inline uint32_t CLEAR_BIT(uint32_t number, unsigned bit_pos)
{
    return number & ~((unsigned)0x1 << bit_pos);
}
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/*-- Static Functions --*/

/*-- Global Functions --*/
silibs_status_t pcie_rp_check_link(uint8_t rpss_idx, uint8_t rp_idx)
{
    silibs_status_t status = SILIBS_SUCCESS;
    pcie_ss_entity_t* rpss = send_sync_get_rpss_entity(rpss_idx);

    if (rpss->rps[rp_idx].enabled)
    {
        /* TODO: Log SEL events in case of failures */
        status = pciess_rp_get_link_train_done(&rpss->rps[rp_idx]);
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
    }

    return status;
}

void process_wait_for_event_data(uint8_t rpss_id, pciess_completion_request_t* req)
{
    uint32_t data = req->async_data.data;

    while (data)
    {
        unsigned idx = __builtin_ctzll(data);
        switch (idx)
        {
        case PCIESS_RP_INT_LINK_DOWN: {
            data = CLEAR_BIT(data, PCIESS_RP_INT_LINK_DOWN);
            /* Task 1515664: PCIe Reset Flow - Hot Reset*/
            break;
        }
        case PCIESS_RP_INT_LINK_UP: {
            data = CLEAR_BIT(data, PCIESS_RP_INT_LINK_UP);
            pcie_rp_check_link(rpss_id, req->rp_index);
            break;
        }

        case PCIESS_RP_INT_DTIM:
        case PCIESS_RP_INT_LTIM:
        case PCIESS_RP_INT_RASDP:
        case PCIESS_RP_INT_HP_PME:
        case PCIESS_RP_INT_WAKEUP:
        case PCIESS_RP_INT_PM_PME:
        case PCIESS_RP_INT_PM_TO_ACK:
        case PCIESS_RP_INT_GLOBAL_IDE:
        case PCIESS_RP_INT_AES_HCFG:
        case PCIESS_RP_INT_SEND_C:
        case PCIESS_RP_INT_SEND_NF:
        case PCIESS_RP_INT_SEND_F:
        case PCIESS_RP_INT_DPC:
        default: {
            // Clear the mask and do  nothing
            data = CLEAR_BIT(data, idx);
            break;
        }
        }
    }
}
