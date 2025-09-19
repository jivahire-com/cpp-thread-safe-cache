//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Define and install interrupt handlers for VAB interrupts.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FPFwInterrupts.h>
#include <FpFwAssert.h>
#include <atu_lib.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <intu_lib.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <silibs_common.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vab.h>
#include <vab_atu_mappings.h>
#include <vab_intu.h>
#include <vab_irq.h>
#include <vab_regs.h>
#include <vab_utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void map_and_get_vab_ctx(SUBSYSTEM_WITH_VAB_ID vab, vab_isr_ctx_t** vab_isr_ctxt);
static SUBSYSTEM_WITH_VAB_ID convert_to_subsystem_with_vab_id(VAB_ID_PER_DIE id_per_die, DIE_INSTANCE die);

/*-- Declarations (Statics and globals) --*/
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
        .process_probe = process_vab_rpss_probe,
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
        .process_probe = process_vab_rpss_probe,
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
        .process_probe = process_vab_rpss_probe,
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
        .process_probe = process_vab_rpss_probe,
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
        .process_probe = process_vab_sdmss_probe,
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
        .process_probe = process_vab_cdedss_probe,
    },
};

/*------------- Functions ----------------*/

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

    ctx->process_probe(ctx);

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
        vab_ctxts[VAB0_RPSS0].vab_base = get_vab_resolved_base(vab);
        vab_ctxts[VAB0_RPSS0].vab_id = vab;
        *vab_isr_ctxt = &(vab_ctxts[VAB0_RPSS0]);
        break;

    case D0_VAB1_RPSS1:
    case D1_VAB1_RPSS1:
        vab_ctxts[VAB1_RPSS1].vab_base = get_vab_resolved_base(vab);
        vab_ctxts[VAB1_RPSS1].vab_id = vab;
        *vab_isr_ctxt = &(vab_ctxts[VAB1_RPSS1]);
        break;

    case D0_VAB2_RPSS2:
    case D1_VAB2_RPSS2:
        vab_ctxts[VAB2_RPSS2].vab_base = get_vab_resolved_base(vab);
        vab_ctxts[VAB2_RPSS2].vab_id = vab;
        *vab_isr_ctxt = &(vab_ctxts[VAB2_RPSS2]);
        break;

    case D0_VAB3_RPSS3:
    case D1_VAB3_RPSS3:
        vab_ctxts[VAB3_RPSS3].vab_base = get_vab_resolved_base(vab);
        vab_ctxts[VAB3_RPSS3].vab_id = vab;
        *vab_isr_ctxt = &(vab_ctxts[VAB3_RPSS3]);
        break;

    case D0_VAB4_SDMSS:
    case D1_VAB4_SDMSS:
        vab_ctxts[VAB4_SDMSS].vab_base = get_vab_resolved_base(vab);
        vab_ctxts[VAB4_SDMSS].vab_id = vab;
        *vab_isr_ctxt = &(vab_ctxts[VAB4_SDMSS]);
        break;

    case D0_VAB5_CDEDSS_IOSS:
    case D1_VAB5_CDEDSS_IOSS:
        vab_ctxts[VAB5_CDEDSS_IOSS].vab_base = get_vab_resolved_base(vab);
        vab_ctxts[VAB5_CDEDSS_IOSS].vab_id = vab;
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

static SUBSYSTEM_WITH_VAB_ID convert_to_subsystem_with_vab_id(VAB_ID_PER_DIE id_per_die, DIE_INSTANCE die)
{
    /* Note: Die is not checked */
    switch (id_per_die)
    {
    case VAB0_RPSS0:
    case VAB1_RPSS1:
    case VAB2_RPSS2:
    case VAB3_RPSS3:
        return id_per_die + (die ? D1_VAB0_RPSS0 : D0_VAB0_RPSS0);
    case VAB4_SDMSS:
        return die ? D1_VAB4_SDMSS : D0_VAB4_SDMSS;
    case VAB5_CDEDSS_IOSS:
        return die ? D1_VAB5_CDEDSS_IOSS : D0_VAB5_CDEDSS_IOSS;
    default:
        FPFW_DBGPRINT_ALWAYS("|VAB| Error: Invalid VAB instance\n");
        return MAX_VAB_INSTANCES;
    }
}

void enable_vab_isrs(uint16_t vab_instances_to_init)
{
    /*
     * INTU-0 Pin 6 is tripping on some big FPGA systems and causing a
     * crash.
     * This needs to be investigated a bit more. Till then, disable
     * VAB INTU signalling on big FPGA systems.
     * WI: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2516114
     */
    if (IS_PLATFORM_FPGA())
    {
        return;
    }

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

acpi_einj_cmd_status_t vab_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);
    FPFW_DBGPRINT_INFO("|VAB| Info: Error Injection Callback Start\n");
    if (einj_payload == NULL)
    {
        FPFW_DBGPRINT_ALWAYS("|VAB| Error: NULL injection payload\n");
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_VAB)
    {
        FPFW_DBGPRINT_ALWAYS("|VAB| Error: Unexpected error domain: %d\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    /*
     * component_type     - refers to VAB_ID_PER_DIE, and
     * component_instance - refers to the die number
     * e.g. RPSS2 VAB on Die0 would be (type, instance) => (VAB2_RPSS2, SOC_D0)
     */
    if (idhw_is_single_die_boot_en() == false)
    {
        if (einj_payload->component_instance != idsw_get_die_id())
        {
            FPFW_DBGPRINT_ALWAYS("|VAB| Error: Invalid component instance %d\n", einj_payload->component_instance);
            return ACPI_EINJ_INVALID_ACCESS;
        }
    }

    /* Cast generic to expected parameter structure */
    vab_error_inj_param_t params;
    silibs_status_t status;
    acpi_einj_cmd_status_t ret = ACPI_EINJ_INVALID_ACCESS;
    SUBSYSTEM_WITH_VAB_ID vab =
        convert_to_subsystem_with_vab_id(einj_payload->component_type, einj_payload->component_instance);
    if (vab == MAX_VAB_INSTANCES)
    {
        return ACPI_EINJ_INVALID_ACCESS;
    }

    uintptr_t vab_base = get_vab_resolved_base(vab);
    params.as_uint64 = einj_payload->param_type.error_parameters[0];

    if (params.target == VAB_ERROR_TARGET_FABRIC)
    {
        status = vab_fabric_error_trigger_by_type(vab_base, params.component, params.error_type);
        if (status)
        {
            FPFW_DBGPRINT_ALWAYS("|VAB| Error: Unable to set fabric parity injection: (%d)\n", status);
            goto exit;
        }

        if (params.error_type != VAB_FORCED_PARITY)
        {
            /*
             * An access is required to trigger the error. This is not directly supported so a warning is
             * issued instead. Injections on RNI/RND side are not exercisable without loopback on RPSS or
             * forced MSIs, and for functional consistency we only arm the injection as requested.
             */
            FPFW_DBGPRINT_ALWAYS("|VAB| Warning: Parity error has been armed. An access on the interface "
                                 "will be required to trigger it.\n");
        }
    }
    else if (params.target == VAB_ERROR_TARGET_RAS_NODE)
    {
        /* These triggers support trigger-on-countdown, so no extra steps are expected to see the end result */
        status = vab_ras_trigger_by_type(vab_base, vab, params.component, params.error_type);
        if (status)
        {
            FPFW_DBGPRINT_ALWAYS("|VAB| Error: Unable to set RAS Node spoof: (%d)\n", status);
            goto exit;
        }
    }
    else
    {
        FPFW_DBGPRINT_ALWAYS("|VAB| Error: Invalid injection target: (%d)\n", params.target);
        goto exit;
    }

    ret = ACPI_EINJ_SUCCESS;

exit:
    return ret;
}
