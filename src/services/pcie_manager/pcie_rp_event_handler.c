//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe rp event processing helper function
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <pcie_link_management_i.h>
#include <pcie_rp_event_handler.h>
#include <pcie_rp_ide.h>
#include <pciess_int.h>       // pcie Interrupts
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <stdint.h>           // for uint8_t
#include <tx_api.h>           // for TX_WAIT_FOREVER, ULONG, tx_queue_receive

/*-- Symbolic Constant Macros (defines) --*/
#define LINK_TRAINING_SETTLE_TICKS (100) /* = 1 second */

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
    uint64_t glbl_ide_status = req->async_data.glbl_ide_data;

    /* Since we are only servicing notifiers, bit-order prioritization suffices */
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

        case PCIESS_RP_INT_GLOBAL_IDE: {
            int_mask = CLEAR_BIT(int_mask, PCIESS_RP_INT_GLOBAL_IDE);

            /* All IDE notifiers are under PCIE_IDE_MISC_GLOBAL_INT */
            if ((glbl_ide_status & PCIE_IDE_MISC_GLOBAL_INT) != 0)
            {
                if ((glbl_ide_status & PCIE_IDE_KEY_NEEDED_INT) != 0)
                {
                    /*
                     * This means at least one TX/RX stream has swapped into a
                     * K-bit slot without a valid key.
                     * Since this is considered fatal to the link, we must
                     * uplevel to AER to induce a link-down event
                     */
                    handle_aer_force_link_down_event(ctx, req);
                }
                else
                {
                    /*
                     * Though more bits may be set in tandem with PCIE_IDE_KEY_NEEDED,
                     * we only service them if we aren't bringing the streams down
                     */
                    if ((glbl_ide_status & PCIE_IDE_TX_KBIT_TOGGLE_INT) || (glbl_ide_status & PCIE_IDE_TX_REKEY_REQ_INT))
                    {
                        handle_tx_rekey_event(ctx, req);
                    }

                    if ((glbl_ide_status & PCIE_IDE_RX_KBIT_TOGGLE_INT) || (glbl_ide_status & PCIE_IDE_RX_REKEY_REQ_INT))
                    {
                        handle_rx_rekey_event(ctx, req);
                    }
                }
            }
            break;
        }

        case PCIESS_RP_INT_AES_HCFG: {
            /*
             * TODO: Utilizing TX/RX_KEY_DONE is tightly-coupled with how the
             *       RMM interfaces for key installations. To be filled out
             *       whenever the end-to-end path for 'send_sync_rp_tx|rx_rekey'
             *       is complete.
             */
            int_mask = CLEAR_BIT(int_mask, PCIESS_RP_INT_AES_HCFG);
            break;
        }

        default: {
            int_mask = CLEAR_BIT(int_mask, idx);
            break;
        }
        }
    }
}
