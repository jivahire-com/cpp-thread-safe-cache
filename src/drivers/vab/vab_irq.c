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
#include <interrupts.h>
#include <intu_lib.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <pcie_irq.h>
#include <silibs_common.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vab.h>
#include <vab_intu.h>
#include <vab_irq.h>
#include <vab_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void map_and_get_vab_ctx(SUBSYSTEM_WITH_VAB_ID vab, vab_isr_ctx_t** vab_isr_ctxt);

/*-- Declarations (Statics and globals) --*/
/*
 * Both die 0 and die 1 VAB addresses are statically defined here to allow
 * dynamically mapping a VAB on either die at runtime.
 */
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

/*
 * VAB ISR contexts and probe structures contain a maximum of
 * MAX_VAB_INSTANCES_PER_DIE (6) entries. An SCP on each die does not
 * need to handle more VABs than that at a time.
 */
static rpss_intu_probe_t rpss0_probe = {0};
static rpss_intu_probe_t rpss1_probe = {0};
static rpss_intu_probe_t rpss2_probe = {0};
static rpss_intu_probe_t rpss3_probe = {0};
static sdmss_intu_probe_t sdmss_probe = {0};
static cdedss_ioss_intu_probe_t cdedss_ioss_probe = {0};

static vab_isr_ctx_t vab_ctxts[MAX_VAB_INSTANCES_PER_DIE] = {
    /* RPSS interrupts */
    {
        .vab_irq_num = HW_INT_VAB0_COMBINED_SCP_INT,
        .probe =
            {
                .intu0 = (vabss_int_t*)&(rpss0_probe.intu0),
                .num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS,
                .intu1 = (vabss_int_t*)&(rpss0_probe.intu1),
                .num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS,
            },
        .process_probe = process_rpss_probe,
    },
    {
        .vab_irq_num = HW_INT_VAB1_COMBINED_SCP_INT,
        .probe =
            {
                .intu0 = (vabss_int_t*)&(rpss1_probe.intu0),
                .num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS,
                .intu1 = (vabss_int_t*)&(rpss1_probe.intu1),
                .num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS,
            },
        .process_probe = process_rpss_probe,
    },
    {
        .vab_irq_num = HW_INT_VAB2_COMBINED_SCP_INT,
        .probe =
            {
                .intu0 = (vabss_int_t*)&(rpss2_probe.intu0),
                .num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS,
                .intu1 = (vabss_int_t*)&(rpss2_probe.intu1),
                .num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS,
            },
        .process_probe = process_rpss_probe,
    },
    {
        .vab_irq_num = HW_INT_VAB3_COMBINED_SCP_INT,
        .probe =
            {
                .intu0 = (vabss_int_t*)&(rpss3_probe.intu0),
                .num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS,
                .intu1 = (vabss_int_t*)&(rpss3_probe.intu1),
                .num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS,
            },
        .process_probe = process_rpss_probe,
    },
    /* SDMSS/D2DSS VAB interrupt */
    {
        .vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT,
        .probe =
            {
                .intu0 = (vabss_int_t*)&(sdmss_probe.intu0),
                .num_intu0_pins = VAB_SDMSS_INTU0_NUM_CFGS,
                .intu1 = (vabss_int_t*)&(sdmss_probe.intu1),
                .num_intu1_pins = VAB_SDMSS_INTU1_NUM_CFGS,
            },
        .process_probe = process_sdmss_probe,
    },
    /* IOSS/CDEDSS interrupts */
    {
        .vab_irq_num = HW_INT_VAB5_COMBINED_SCP_INT,
        .probe =
            {
                .intu0 = (vabss_int_t*)&(cdedss_ioss_probe.intu0),
                .num_intu0_pins = VAB_CDEDSS_INTU0_NUM_CFGS,
                .intu1 = (vabss_int_t*)&(cdedss_ioss_probe.intu1),
                .num_intu1_pins = VAB_CDEDSS_INTU1_NUM_CFGS,
            },
        .process_probe = process_cdedss_probe,
    },
};

/*------------- Functions ----------------*/
void process_rpss_probe(uint32_t irq, vabss_int_probe_t* probe)
{
    for (uint8_t idx = 0; idx < probe->num_intu0_pins; idx++)
    {
        if (probe->intu0[idx].asserted)
        {
            switch (idx)
            {
            case VAB_RPS_INTU0_RPSS_SCP_INT:
                printf("INT[%u] VAB-INTU0[%d] RPSS combined pin fired - probe rpss intu further!\n", (uint8_t)irq, idx);
                rpss_irq_callback(irq);
                break;
            default:
                printf("INT[%u] VAB-INTU0[%d] pin fired - critical error!\n", (uint8_t)irq, idx);
                FPFW_RUNTIME_ASSERT(false);
                break;
            }
        }
    }

    /*
     * All intu1 pins on an rpss VAB are treated as fatal errors - only way to
     * recover from these is a cold reset of that particular VABSS.
     */
    for (uint8_t idx = 0; idx < probe->num_intu1_pins; idx++)
    {
        if (probe->intu1[idx].asserted)
        {
            printf("INT[%u] VAB-INTU1[%d] pin fired - critical error!\n", (uint8_t)irq, idx);
            FPFW_RUNTIME_ASSERT(false);
            break;
        }
    }
}

void process_sdmss_probe(uint32_t irq, vabss_int_probe_t* probe)
{
    /*
     * All intu0 and intu1 pins on a sdmss VAB are treated as fatal errors
     * The only way to recover from these is a cold reset of that
     * particular VABSS.
     */
    for (uint8_t idx = 0; idx < probe->num_intu0_pins; idx++)
    {
        if (probe->intu0[idx].asserted)
        {
            printf("INT[%u] VAB-INTU0[%d] pin fired - critical error!\n", (uint8_t)irq, idx);
            FPFW_RUNTIME_ASSERT(false);
            break;
        }
    }

    for (uint8_t idx = 0; idx < probe->num_intu1_pins; idx++)
    {
        if (probe->intu1[idx].asserted)
        {
            printf("INT[%u] VAB-INTU1[%d] pin fired - critical error!\n", (uint8_t)irq, idx);
            FPFW_RUNTIME_ASSERT(false);
            break;
        }
    }
}

void process_cdedss_probe(uint32_t irq, vabss_int_probe_t* probe)
{
    /*
     * All intu0 and intu1 pins on a cdedss VAB are treated as fatal errors
     * The only way to recover from these is a cold reset of that
     * particular VABSS.
     */
    for (uint8_t idx = 0; idx < probe->num_intu0_pins; idx++)
    {
        if (probe->intu0[idx].asserted)
        {
            printf("INT[%u] VAB-INTU0[%d] pin fired - critical error!\n", (uint8_t)irq, idx);
            FPFW_RUNTIME_ASSERT(false);
            break;
        }
    }

    for (uint8_t idx = 0; idx < probe->num_intu1_pins; idx++)
    {
        if (probe->intu1[idx].asserted)
        {
            printf("INT[%u] VAB-INTU1[%d] pin fired - critical error!\n", (uint8_t)irq, idx);
            FPFW_RUNTIME_ASSERT(false);
            break;
        }
    }
}

void vab_isr(void* param)
{
    vab_isr_ctx_t* ctx = (vab_isr_ctx_t*)param;
    uint32_t irq = ctx->vab_irq_num;
    vabss_int_probe_t* probe = &(ctx->probe);
    silibs_status_t status;

    memset((void*)(probe->intu0), 0, (sizeof(vabss_int_t) * probe->num_intu0_pins));
    memset((void*)(probe->intu1), 0, (sizeof(vabss_int_t) * probe->num_intu1_pins));
    status = vabss_intu_probe(ctx->vab_base, &(ctx->probe), INTU_TO_SCP);
    if (status != SILIBS_SUCCESS)
    {
        printf("INT[%u] VAB intu probe failed with status: %d\n", (uint8_t)irq, (int8_t)status);
        return;
    }

    ctx->process_probe(irq, probe);

    vabss_intu_clear(ctx->vab_base, &(ctx->probe), INTU_TO_SCP);
}

static void map_and_get_vab_ctx(SUBSYSTEM_WITH_VAB_ID vab, vab_isr_ctx_t** vab_isr_ctxt)
{
    if (vab_isr_ctxt == NULL)
    {
        return;
    }

    /*
     * This maps macros defined for die 0 or die 1 into their respective index
     * into the vab ISR context array.
     */
    switch (vab)
    {
    case D0_VAB0_RPSS0:
    case D1_VAB0_RPSS0:
        FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vabss_map[vab]));
        vab_ctxts[VAB0_RPSS0].vab_base = atu_vabss_map[vab].mscp_start_address;
        *vab_isr_ctxt = &(vab_ctxts[VAB0_RPSS0]);
        break;

    case D0_VAB1_RPSS1:
    case D1_VAB1_RPSS1:
        FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vabss_map[vab]));
        vab_ctxts[VAB1_RPSS1].vab_base = atu_vabss_map[vab].mscp_start_address;
        *vab_isr_ctxt = &(vab_ctxts[VAB1_RPSS1]);
        break;

    case D0_VAB2_RPSS2:
    case D1_VAB2_RPSS2:
        FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vabss_map[vab]));
        vab_ctxts[VAB2_RPSS2].vab_base = atu_vabss_map[vab].mscp_start_address;
        *vab_isr_ctxt = &(vab_ctxts[VAB2_RPSS2]);
        break;

    case D0_VAB3_RPSS3:
    case D1_VAB3_RPSS3:
        FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vabss_map[vab]));
        vab_ctxts[VAB3_RPSS3].vab_base = atu_vabss_map[vab].mscp_start_address;
        *vab_isr_ctxt = &(vab_ctxts[VAB3_RPSS3]);
        break;

    case D0_VAB4_SDMSS:
    case D1_VAB4_SDMSS:
        FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vabss_map[vab]));
        vab_ctxts[VAB4_SDMSS].vab_base = atu_vabss_map[vab].mscp_start_address;
        *vab_isr_ctxt = &(vab_ctxts[VAB4_SDMSS]);
        break;

    case D0_VAB5_CDEDSS_IOSS:
    case D1_VAB5_CDEDSS_IOSS:
        FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vabss_map[vab]));
        vab_ctxts[VAB5_CDEDSS_IOSS].vab_base = atu_vabss_map[vab].mscp_start_address;
        *vab_isr_ctxt = &(vab_ctxts[VAB5_CDEDSS_IOSS]);
        break;

    case MAX_VAB_INSTANCES:
    default:
        *vab_isr_ctxt = NULL;
        printf("Failed to enable VAB ISR for invalid vab index - [%d]!\n", vab);
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

void enable_vab_isrs(uint16_t vab_instances_to_init)
{
    for (SUBSYSTEM_WITH_VAB_ID vab = D0_VAB0_RPSS0; vab < MAX_VAB_INSTANCES; vab++)
    {
        if ((vab_instances_to_init >> vab) & 0x1)
        {
            vab_isr_ctx_t* vab_isr_ctxt = NULL;

            map_and_get_vab_ctx(vab, &vab_isr_ctxt);

            uint32_t irqnum = vab_isr_ctxt->vab_irq_num;

            uint32_t status = FPFwCoreInterruptRegisterCallback(irqnum, vab_isr, (void*)(vab_isr_ctxt));
            FPFW_RUNTIME_ASSERT(status == 0);

            status = FPFwCoreInterruptEnableVector(irqnum);
            FPFW_RUNTIME_ASSERT(status == 0);
        }
    }
}
