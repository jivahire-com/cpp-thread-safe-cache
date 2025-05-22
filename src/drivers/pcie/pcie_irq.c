//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements handlers and calls the relevant silibs APIs to walk the PCIe RPSS
 * INTU tree to understand what event might have triggered an RPSS interrupt.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwAssert.h>
#include <idsw.h>
#include <interrupts.h>
#include <intu_lib.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <pcie_irq.h>
#include <pcie_rp_common.h>
#include <pcie_ss_common.h>
#include <pciess.h>
#include <pciess_int.h>
#include <rpss_addr_map_regs.h>
#include <silibs_common.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static pcie_ss_entity_t* get_rpss_entity_from_irq_num(uint32_t irq_num);

/*-- Declarations (Statics and globals) --*/
static pciess_int_probe_t int_info = {0};

/*------------- Functions ----------------*/
static pcie_ss_entity_t* get_rpss_entity_from_irq_num(uint32_t irq_num)
{
    RPSS_INSTANCE rpss_idx = NUM_RPSS;
    idsw_die_id_t die_id = idsw_get_die_id();

    switch (irq_num)
    {
    case HW_INT_VAB0_COMBINED_SCP_INT:
        rpss_idx = (die_id == 0) ? RPSS0 : RPSS4;
        break;

    case HW_INT_VAB1_COMBINED_SCP_INT:
        rpss_idx = (die_id == 0) ? RPSS1 : RPSS5;
        break;

    case HW_INT_VAB2_COMBINED_SCP_INT:
        rpss_idx = (die_id == 0) ? RPSS2 : RPSS6;
        break;

    case HW_INT_VAB3_COMBINED_SCP_INT:
        rpss_idx = (die_id == 0) ? RPSS3 : RPSS7;
        break;

    /*
     * VAB4 and VAB5 irqs should never be handled by the PCIe driver
     * as they don't belong to a RPSS VAB.
     */
    case HW_INT_VAB4_COMBINED_SCP_INT:
    case HW_INT_VAB5_COMBINED_SCP_INT:
    default:
        FPFW_RUNTIME_ASSERT(irq_num <= HW_INT_VAB3_COMBINED_SCP_INT);
        break;
    }

    return pciess_get_entity(rpss_idx);
}

void rpss_irq_callback(uint32_t irq_num)
{
    pcie_ss_entity_t* ss = get_rpss_entity_from_irq_num(irq_num);
    pcie_async_request_t* pending_req = {0};

    memset(&int_info, 0x0, sizeof(pciess_int_probe_t));

    /* Probe all pciess interrupts */
    if (pciess_probe(ss, &int_info, INTU_TO_SCP))
    {
        for (uint8_t rp_idx = 0; rp_idx < PCIESS_NUM_PORTS; rp_idx++)
        {
            bool int_present = false;
            uint32_t int_mask = 0;
            uint32_t int_data = 0;
            for (uint32_t int_idx = 0; int_idx < PCIESS_RP_NUM_INTERRUPTS; int_idx++)
            {
                if (int_info.rp_ints[rp_idx].ints[int_idx].asserted == false)
                {
                    continue;
                }

                switch (int_idx)
                {
                case PCIESS_RP_INT_LINK_DOWN:
                    int_present = true;
                    int_mask |= 0x1 << PCIESS_RP_INT_LINK_DOWN;
                    FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d] IRQ[%d]: Link down!\n", ss->id, rp_idx, (unsigned int)irq_num);
                    break;
                case PCIESS_RP_INT_LINK_UP:
                    int_present = true;
                    int_mask |= 0x1 << PCIESS_RP_INT_LINK_UP;
                    FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d] IRQ[%d]: Link up!\n", ss->id, rp_idx, (unsigned int)irq_num);
                    break;
                case PCIESS_RP_INT_DTIM:
                    int_present = true;
                    int_mask |= 0x1 << PCIESS_RP_INT_DTIM;
                    int_data = int_info.rp_ints[rp_idx].ints[int_idx].status;
                    break;
                case PCIESS_RP_INT_LTIM:
                    int_present = true;
                    int_mask |= 0x1 << PCIESS_RP_INT_LTIM;
                    int_data = int_info.rp_ints[rp_idx].ints[int_idx].status;
                    break;
                case PCIESS_RP_INT_RASDP:
                    int_present = true;
                    int_mask |= 0x1 << PCIESS_RP_INT_RASDP;
                    /*
                     * vsecras probing is deferred to the service layer as
                     * it is a much slower operation to handle in the ISR.
                     */
                    pcie_rp_vsecras_clear_rasdp_error_mode(&(ss->rps[rp_idx]));
                    break;
                case PCIESS_RP_INT_DPC:
                    int_present = true;
                    int_mask |= 0x1 << PCIESS_RP_INT_DPC;
                    break;
                case PCIESS_RP_INT_GLOBAL_IDE:
                case PCIESS_RP_INT_AES_HCFG:
                    FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d] IRQ[%d]: IDE interrupt!\n", ss->id, rp_idx, (unsigned int)irq_num);
                    break;
                /* Fall through for all non-por interrupts that aren't handled */
                case PCIESS_RP_INT_HP_PME:
                case PCIESS_RP_INT_WAKEUP:
                case PCIESS_RP_INT_PM_PME:
                case PCIESS_RP_INT_PM_TO_ACK:
                case PCIESS_RP_INT_SEND_C:
                case PCIESS_RP_INT_SEND_NF:
                case PCIESS_RP_INT_SEND_F:
                default:
                    FPFW_DBGPRINT_WARNING("RPSS[%d] RP[%d] IRQ[%d]: Non-POR interrupt (%d) fired\n!\n",
                                          ss->id,
                                          rp_idx,
                                          (unsigned int)irq_num,
                                          (unsigned int)int_idx);
                    break;
                }
            }

            /*
             * If int_present is set for a given RP, then complete the request
             * for that specific RP
             */
            if (int_present)
            {
                pending_req = get_pending_async_req_for_this_rp(ss->id, rp_idx, WAIT_FOR_EVENT);
                if (pending_req)
                {
                    pending_req->async_data.int_mask = int_mask;
                    pending_req->async_data.int_data = int_data;
                    complete_async_req_for_this_rp(pending_req);
                }
            }
        }

        /* Clear all pending interrupts */
        pciess_clear_handler(ss, &int_info, INTU_TO_SCP);
    }
}
