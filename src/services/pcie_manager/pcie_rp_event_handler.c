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
#include <fpfw_cfg_mgr.h>
#include <pcie_dfwk.h> // for pcie_async_request_t, pcie_dfwk_interf...
#include <pcie_error_management_i.h>
#include <pcie_link_management_i.h>
#include <pcie_manager_i.h> // for rpss_req_completion_cb, rpss_service_t...
#include <pcie_rp_event_handler.h>
#include <pcie_sync_requests_i.h>
#include <pciess_int.h>       // pcie Interrupts
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <stdbool.h>          // for true
#include <stdint.h>           // for uint8_t
#include <system_info.h>
#include <tx_api.h> // for TX_WAIT_FOREVER, ULONG, tx_queue_receive

/*-- Symbolic Constant Macros (defines) --*/
#define WAIT_SBR_MS        (FPFW_MAX((TX_TIMER_TICKS_PER_SECOND / (200UL)), (1UL))) // 5ms
#define OVL_LT_RETRIES_MAX (3)

inline uint32_t CLEAR_BIT(uint32_t number, unsigned bit_pos)
{
    return number & ~((unsigned)0x1 << bit_pos);
}
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/*-- Static Functions --*/
static bool rp_is_overlake(uint8_t rpss_idx, uint8_t rp_idx)
{
    if (rp_idx != 0)
    {
        return false;
    }

    /* Check for mirrored config for 2S */
    uint8_t soc_position = BOARD_ID_GET_SOC_POSITION(system_info_get_board_id());
    if (soc_position == 0x01)
    {
        return (config_get_overlake_rpss_index_secondary_soc() == rpss_idx);
    }

    return (config_get_overlake_rpss_index_primary_soc() == rpss_idx);
}

/*-- Global Functions --*/
void process_wait_for_event_data(pcie_manager_context_t* ctx, pciess_completion_request_t* req)
{
    uint32_t int_mask = req->async_data.int_mask;
    uint8_t rpss_idx = ctx->rpss_idx;
    uint8_t rp_idx = req->rp_index;

    static uint8_t ovl_lt_retries = 0;
    silibs_status_t status = SILIBS_SUCCESS;

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
            status = send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx->iface, rpss_idx, rp_idx);

            if (rp_is_overlake(rpss_idx, rp_idx) && config_get_enable_overlake_sbr_workaround())
            {
                if (status == SILIBS_E_OVERWRITTEN && ovl_lt_retries < OVL_LT_RETRIES_MAX)
                {
                    FPFW_DBGPRINT_INFO("Link training failed for Overlake RP on PCIe SS %d, issue SBR\n", rpss_idx);
                    /* Send the SBR, this will result in link-down flow being triggered */
                    send_sync_rp_set_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx->iface, rpss_idx, rp_idx);
                    tx_thread_sleep(WAIT_SBR_MS);
                    send_sync_rp_clear_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx->iface, rpss_idx, rp_idx);
                    ovl_lt_retries++;
                }
                else if (status == SILIBS_SUCCESS)
                {
                    /*
                     * We should only retry on cold reset so set it to the max
                     * to ensure any future link down events don't trigger SBR
                     * flow
                     */
                    ovl_lt_retries = OVL_LT_RETRIES_MAX;
                }
            }
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
