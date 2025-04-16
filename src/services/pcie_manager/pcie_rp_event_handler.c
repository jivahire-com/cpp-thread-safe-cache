//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe rp event processing helper function
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkClient.h> // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <DfwkPtrTypes.h>
#include <pcie_dfwk.h> // for pcie_async_request_t, pcie_dfwk_interf...
#include <pcie_link_management_i.h>
#include <pcie_manager_i.h> // for rpss_req_completion_cb, rpss_service_t...
#include <pcie_sync_requests_i.h>
#include <pciess_int.h>       // pcie Interrupts
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <stdbool.h>          // for true
#include <stdint.h>           // for uint8_t
#include <tx_api.h>           // for TX_WAIT_FOREVER, ULONG, tx_queue_receive

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
void process_wait_for_event_data(pcie_manager_context_t* ctx, pciess_completion_request_t* req)
{
    uint32_t data = req->async_data.data;
    uint8_t rpss_idx = ctx->rpss_idx;

    while (data)
    {
        unsigned idx = __builtin_ctzll(data);
        switch (idx)
        {
        case PCIESS_RP_INT_LINK_DOWN: {
            data = CLEAR_BIT(data, PCIESS_RP_INT_LINK_DOWN);
            handle_pcie_link_down_event(ctx, req);
            break;
        }
        case PCIESS_RP_INT_LINK_UP: {
            data = CLEAR_BIT(data, PCIESS_RP_INT_LINK_UP);
            send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx->iface, rpss_idx, req->rp_index);
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
