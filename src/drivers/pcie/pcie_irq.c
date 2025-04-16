//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements handlers and calls the relevant silibs APIs to walk the PCIe RPSS
 * INTU tree to understand what event might have triggered an RPSS interrupt.
 */

/*------------- Includes -----------------*/
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
#include <stdio.h>

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

    /* Probe all PCIESS INT */
    if (pciess_probe(ss, &int_info, INTU_TO_SCP))
    {
        for (uint8_t rp_idx = 0; rp_idx < PCIESS_NUM_PORTS; rp_idx++)
        {
            bool int_present = false;
            uint32_t int_data = 0; // for Now this is the only data filled into the request
            for (uint32_t int_idx = 0; int_idx < PCIESS_RP_NUM_INTERRUPTS; int_idx++)
            {
                if (int_info.rp_ints[rp_idx].ints[int_idx].asserted == true)
                {
                    switch (int_idx)
                    {
                    case PCIESS_RP_INT_LINK_DOWN: {
                        if (int_info.rp_ints[rp_idx].ints[int_idx].asserted == true)
                        {
                            int_present = true;
                            int_data |= 0x1 << PCIESS_RP_INT_LINK_DOWN;
                            printf("Link Down : RPSS IRQ: 0x%x \t RP: 0x%x \n", (unsigned int)irq_num, rp_idx);
                        }
                        break;
                    }
                    case PCIESS_RP_INT_LINK_UP: {
                        if (int_info.rp_ints[rp_idx].ints[int_idx].asserted == true)
                        {
                            int_present = true;
                            int_data |= 0x1 << PCIESS_RP_INT_LINK_UP;
                            printf("Link Up : RPSS IRQ: 0x%x \t RP: 0x%x \n", (unsigned int)irq_num, rp_idx);
                        }
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
                        printf("Default:: Unhandled FW PCIE INT \n");
                        break;
                    }
                    }
                }
            }
            // If int_present is set for a given RP, then complete the request for that
            // specific RP
            if (int_present)
            {
                pending_req = get_pending_async_req_for_this_rp(ss->id, rp_idx, WAIT_FOR_EVENT);
                if (pending_req)
                {
                    // Update Req Completion Data
                    // Update pending_req with INT data
                    pending_req->async_data.data = int_data;

                    complete_async_req_for_this_rp(pending_req);
                }
            }
        }
    }

    /* Clear Pending PCIESS INT  */
    pciess_clear_handler(ss, &int_info, INTU_TO_SCP);
}
