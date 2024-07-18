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
#include <pcie_irq.h>
#include <pcie_rp_common.h>
#include <pcie_ss_common.h>
#include <pciess.h>
#include <rpss_addr_map_regs.h>
#include <silibs_common.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define RPSS_PCIE_LINK_UP (BIT13)
#define RPSS_INTU_STRIDE  (RPSS_ADDR_MAP_RPSS_INTU_IAGG_P1_ADDRESS - RPSS_ADDR_MAP_RPSS_INTU_IAGG_P0_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static pcie_ss_entity_t* get_rpss_entity_from_irq_num(uint32_t irq_num);
static bool is_pcie_link_up(uint32_t status);

/*-- Declarations (Statics and globals) --*/

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

static bool is_pcie_link_up(uint32_t status)
{
    if ((status & RPSS_PCIE_LINK_UP) != 0x00)
    {
        return true;
    }

    return false;
}

void rpss_irq_callback(uint32_t irq_num)
{
    pcie_ss_entity_t* ss = get_rpss_entity_from_irq_num(irq_num);

    for (uint8_t i = 0; i < ROOT_PORTS_PER_RPSS; i++)
    {
        pcie_rp_entity_t* rp = &(ss->rps[i]);

        if (!rp->enabled)
        {
            continue;
        }

        uint32_t status = 0;
        uint64_t rpss_intu_addr =
            rp->bases._base + RPSS_ADDR_MAP_RPSS_INTU_IAGG_P0_ADDRESS + (RPSS_INTU_STRIDE * rp->id);
        intu_get_interrupt_status(rpss_intu_addr, &status);

        if (is_pcie_link_up(status) == true)
        {
            pcie_link_up_interrupt_callback(ss, i);
        }
        else
        {
            printf("Unhandled INTU status received on rp_idx(%d): (0x%llx)\n", i, (uint64_t)status);
        }
        pciess_rp_clear_intus(&(ss->rps[i]));
    }
}
