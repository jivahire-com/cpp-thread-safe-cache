//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe rp event processing helper function
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <pcie_error_handling_i.h>
#include <pcie_link_management_i.h>
#include <pcie_rp_event_handler.h>
#include <pciess_int.h>       // pcie Interrupts
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <stdint.h>           // for uint8_t
#include <tx_api.h>           // for TX_WAIT_FOREVER, ULONG, tx_queue_receive

/*-- Symbolic Constant Macros (defines) --*/
#define LINK_TRAINING_SETTLE_TICKS (1000)

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
    uint32_t int_mask = req->async_data.int_mask;

    while (int_mask)
    {
        unsigned idx = __builtin_ctzll(int_mask);
        switch (idx)
        {
        case PCIESS_RP_INT_LINK_DOWN: {
            int_mask = CLEAR_BIT(int_mask, PCIESS_RP_INT_LINK_DOWN);
            handle_pcie_link_down_event(ctx, req);
            break;
        }

        case PCIESS_RP_INT_LINK_UP: {
            int_mask = CLEAR_BIT(int_mask, PCIESS_RP_INT_LINK_UP);
            tx_thread_sleep(LINK_TRAINING_SETTLE_TICKS);
            handle_pcie_link_up_event(ctx, req);
            break;
        }

        case PCIESS_RP_INT_RASDP:
            int_mask = CLEAR_BIT(int_mask, PCIESS_RP_INT_RASDP);
            handle_pcie_vsecras_event(ctx, req);
            break;

        case PCIESS_RP_INT_DTIM:
            int_mask = CLEAR_BIT(int_mask, PCIESS_RP_INT_DTIM);
            handle_pcie_dtim_event(ctx, req);
            break;

        case PCIESS_RP_INT_LTIM:
            int_mask = CLEAR_BIT(int_mask, PCIESS_RP_INT_LTIM);
            handle_pcie_ltim_event(ctx, req);
            break;

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
            int_mask = CLEAR_BIT(int_mask, idx);
            break;
        }
        }
    }
}
