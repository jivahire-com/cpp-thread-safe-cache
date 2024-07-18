//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Define and install interrupt handlers for VAB interrupts.
 */

/*------------- Includes -----------------*/
#include <FPFwInterrupts.h>
#include <FpFwAssert.h>
#include <atu_lib.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <intu_lib.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <pcie_irq.h>
#include <silibs_common.h>
#include <silibs_platform.h>
#include <stdint.h>
#include <vab.h>
#include <vab_irq.h>
#include <vab_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void get_vab_intu_status(vab_isr_ctx_t* ctx);
static void clear_vab_intu_status(vab_isr_ctx_t* ctx);
static bool is_rpss_combined_interrupt(vab_isr_ctx_t* ctx);

/*-- Declarations (Statics and globals) --*/
static atu_map_entry_t atu_vabss_map[MAX_VAB_INSTANCES] = {
    ATU_MAPPING_D0_VAB0_RPSS0(),
    ATU_MAPPING_D0_VAB1_RPSS1(),
    ATU_MAPPING_D0_VAB2_RPSS2(),
    ATU_MAPPING_D0_VAB3_RPSS3(),
    ATU_MAPPING_D1_VAB0_RPSS0(),
    ATU_MAPPING_D1_VAB1_RPSS1(),
    ATU_MAPPING_D1_VAB2_RPSS2(),
    ATU_MAPPING_D1_VAB3_RPSS3(),
    ATU_MAPPING_D0_VAB4_SDMSS(),
    ATU_MAPPING_D1_VAB4_SDMSS(),
    ATU_MAPPING_D0_VAB5_CDEDSS_IOSS(),
    ATU_MAPPING_D1_VAB5_CDEDSS_IOSS(),
};

/* Matches the VAB enum ordering from kng_soc_constants.h */
static vab_isr_ctx_t vab_ctxs[MAX_VAB_INSTANCES] = {
    /* Die 0 RPSS interrupts */
    {.vab_irq_num = HW_INT_VAB0_COMBINED_SCP_INT},
    {.vab_irq_num = HW_INT_VAB1_COMBINED_SCP_INT},
    {.vab_irq_num = HW_INT_VAB2_COMBINED_SCP_INT},
    {.vab_irq_num = HW_INT_VAB3_COMBINED_SCP_INT},
    /* Die 1 RPSS interrupts */
    {.vab_irq_num = HW_INT_VAB0_COMBINED_SCP_INT},
    {.vab_irq_num = HW_INT_VAB1_COMBINED_SCP_INT},
    {.vab_irq_num = HW_INT_VAB2_COMBINED_SCP_INT},
    {.vab_irq_num = HW_INT_VAB3_COMBINED_SCP_INT},
    /* Die 0 SDMSS/D2DSS VAB interrupt */
    {.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT},
    /* Die 1 SDMSS/D2DSS VAB interrupt */
    {.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT},
    /* Die 0 IOSS/CDEDSS interrupts */
    {.vab_irq_num = HW_INT_VAB5_COMBINED_SCP_INT},
    /* Die 1 IOSS/CDEDSS interrupts */
    {.vab_irq_num = HW_INT_VAB5_COMBINED_SCP_INT},
};

/*------------- Functions ----------------*/
static void get_vab_intu_status(vab_isr_ctx_t* ctx)
{
    intu_get_interrupt_status(ctx->intu0_base, &(ctx->intu0_sts));
    intu_get_interrupt_status(ctx->intu1_base, &(ctx->intu1_sts));
}

static void clear_vab_intu_status(vab_isr_ctx_t* ctx)
{
    ctx->intu0_sts = 0;
    ctx->intu1_sts = 0;
    intu_clear_interrupt_status(ctx->intu0_base, VAB_INTU_CLEAR_MASK);
    intu_clear_interrupt_status(ctx->intu1_base, VAB_INTU_CLEAR_MASK);
}

static bool is_rpss_combined_interrupt(vab_isr_ctx_t* ctx)
{
    /* Not an RPSS interrupt */
    if (ctx->vab_irq_num > HW_INT_VAB3_COMBINED_SCP_INT)
    {
        return false;
    }

    if ((ctx->intu0_sts & VAB_RPSS_COMBINED_INT) != 0x00)
    {
        return true;
    }

    return false;
}

void vab_isr(void* param)
{
    vab_isr_ctx_t* ctx = (vab_isr_ctx_t*)param;

    get_vab_intu_status(ctx);

    if (is_rpss_combined_interrupt(ctx) == true)
    {
        rpss_irq_callback(ctx->vab_irq_num);
    }

    clear_vab_intu_status(ctx);
}

void enable_vab_isrs(uint16_t vab_instances_to_init)
{
    /*
     * R19 and below RTL releases have issues where the VAB interrupt is
     * spuriously and continuously triggered for misc. parity errors.
     *
     * Details: https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/744964/
     *          https://dev.azure.com/ms-tsd/Kingsgate/_workitems/edit/730762
     *
     * Do not enable VAB interrupts on FPGA till R21 is widely deployed.
     */
    if (IS_PLATFORM_FPGA())
    {
        return;
    }

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vab_instances_to_init >> vab_id) & 0x1)
        {
            FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vabss_map[vab_id]));
            uint32_t irq_num = vab_ctxs[vab_id].vab_irq_num;
            vab_ctxs[vab_id].intu0_base = (atu_vabss_map[vab_id].mscp_start_address + VAB_VAB_FW_AGG0_ADDRESS);
            vab_ctxs[vab_id].intu0_sts = 0;
            vab_ctxs[vab_id].intu1_base = (atu_vabss_map[vab_id].mscp_start_address + VAB_VAB_FW_AGG1_ADDRESS);
            vab_ctxs[vab_id].intu1_sts = 0;

            uint32_t status = FPFwCoreInterruptRegisterCallback(irq_num, vab_isr, (void*)(&(vab_ctxs[vab_id])));
            FPFW_RUNTIME_ASSERT(status == 0);

            clear_vab_intu_status(&(vab_ctxs[vab_id]));

            status = FPFwCoreInterruptEnableVector(irq_num);
            FPFW_RUNTIME_ASSERT(status == 0);
        }
    }
}
