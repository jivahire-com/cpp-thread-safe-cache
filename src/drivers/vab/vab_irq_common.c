//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Service routines for VAB RPSS domain.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <bug_check.h>
#include <cper.h>
#include <health_monitor.h>
#include <silibs_common.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tower_isr.h>
#include <vab.h>
#include <vab_init.h>
#include <vab_intu.h>
#include <vab_irq.h>
#include <vab_irq_common.h>
#include <vab_regs.h>
#include <vab_rpss_top_regs.h>
#include <vab_utils.h>

/*-- Symbolic Constant Macros (defines) --*/
/*
 * Note: Even though these are VAB_RPSS defines, VAB_SDMSS and VAB_CDEDSS_IOSS
 *       sit at the same locations
 */
#define RAS_VAB_TOWER_OFFSET \
    (VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS + NOCNI_NITOWER_VAB_ACE5LITE_TARGET_S_HNP_MM_NOCNI_NITOWER_VAB_2_ADDRESS)
#define RAS_TCU_OFFSET                                                                                \
    (VAB_RPSS_TOP_VAB_ADDRESS + VAB_SMMU_YARDLEY_VAB_ADDRESS + SMMU_YARDLEY_EAC_VAB_TCU_VAB_ADDRESS + \
     MMU_YARDLEY_TCU_X2_CUSTOM_VAB_TCU_X2_ADDRESS + VAB_TCU_X2_TCU_ERRFR_ADDRESS)
#define RAS_TBU0_OFFSET                                                                               \
    (VAB_RPSS_TOP_VAB_ADDRESS + VAB_SMMU_YARDLEY_VAB_ADDRESS + SMMU_YARDLEY_EAC_VAB_TCU_VAB_ADDRESS + \
     MMU_YARDLEY_TBU_LTI_X4_CUSTOM_VAB_TBU_LTI_X4_ADDRESS + VAB_TBU_LTI_X4_TBU_ERRFR_ADDRESS)
#define RAS_TBU1_OFFSET                                                                               \
    (VAB_RPSS_TOP_VAB_ADDRESS + VAB_SMMU_YARDLEY_VAB_ADDRESS + SMMU_YARDLEY_EAC_VAB_TCU_VAB_ADDRESS + \
     MMU_YARDLEY_TBU_ACE_LITE_CUSTOM_VAB_TBU_ACE_LITE_ADDRESS + VAB_TBU_ACE_LITE_TBU_ERRFR_ADDRESS)
#define RAS_VAB_OFFSET (VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_RAS_NODE_ADDRESS + VAB_ERG0_ERR0FR_LO_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
silibs_status_t vab_ras_node_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    silibs_status_t status;
    ras_agent_entity_t* agent = vab_get_vab_ras_agent_entity(vab_id);
    ras_arm_agent_set_base(agent, base + RAS_VAB_OFFSET);
    ras_error_record_t record = {0};

    /* The VAB RAS Node only logs ITS Faults which are considered UEs and therefore fatal */
    while (ras_agent_probe(agent, &record))
    {
        ras_print_record(&record);
        acpi_err_sec_vab_t vab_cper = {.error_type = VAB_INVALID_ITS_TARGET,
                                       .component = VAB_RAS_NODE,
                                       .its_error_data = {.its_target = (record.misc_2 & 0x1F)}};
        /* We do not care about failures in the conversion since we want to log out anyway */
        ras_arm_record_to_cper(&record, &vab_cper.record, sizeof(vab_cper.record));

        acpi_cper_section_t cper_section;
        cper_section.sec_vab = vab_cper;
        hm_submit_cper(ACPI_ERROR_DOMAIN_VAB, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(cper_section));
        if (record.handler)
        {
            status = record.handler(&record);
            if (status)
            {
                FPFW_DBGPRINT_ALWAYS("Error encountered while handling VAB RAS Node record: (%d)\n", status);
                continue;
            }
        }
        else
        {
            FPFW_DBGPRINT_ALWAYS(
                "VAB RAS Node Record was marked as invalid! No further handling will be done\n");
            continue;
        }
    }

    /*
     * Note: Since we are fataling on the last record (which could be one of many),
     *       we choose to log the last captured ITS target for simplicity
     */
    BUG_CHECK(VAB_INVALID_ITS_TARGET, VAB_RAS_NODE, (record.misc_2 & 0x1F));

    /* This return is bogus as we bugcheck-ed above */
    return SILIBS_E_PANIC;
}

silibs_status_t vab_tbu0_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    silibs_status_t status;
    ras_agent_entity_t* agent = vab_get_smmu_ras_agent_entity(vab_id, SMMU_RAS_NODE_TBU0);
    ras_arm_agent_set_base(agent, base + RAS_TBU0_OFFSET);
    ras_error_record_t record = {0};

    while (ras_agent_probe(agent, &record))
    {
        ras_print_record(&record);
        if (record.handler)
        {
            status = record.handler(&record);
            if (status)
            {
                FPFW_DBGPRINT_ALWAYS("Error encountered while handling TBU0 record\n");
                return status;
            }
        }
        else
        {
            FPFW_DBGPRINT_ALWAYS("TBU0 Record was marked as invalid! No further handling will be done\n");
            continue;
        }
    }

    return SILIBS_SUCCESS;
}

silibs_status_t vab_tbu1_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    silibs_status_t status;
    ras_agent_entity_t* agent = vab_get_smmu_ras_agent_entity(vab_id, SMMU_RAS_NODE_TBU1);
    ras_arm_agent_set_base(agent, base + RAS_TBU1_OFFSET);
    ras_error_record_t record = {0};

    while (ras_agent_probe(agent, &record))
    {
        ras_print_record(&record);
        if (record.handler)
        {
            status = record.handler(&record);
            if (status)
            {
                FPFW_DBGPRINT_ALWAYS("Error encountered while handling TBU1 record\n");
                return status;
            }
        }
        else
        {
            FPFW_DBGPRINT_ALWAYS("TBU1 Record was marked as invalid! No further handling will be done\n");
            continue;
        }
    }

    return SILIBS_SUCCESS;
}

silibs_status_t vab_tcu_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    silibs_status_t status;
    ras_agent_entity_t* agent = vab_get_smmu_ras_agent_entity(vab_id, SMMU_RAS_NODE_TCU);
    ras_arm_agent_set_base(agent, base + RAS_TCU_OFFSET);
    ras_error_record_t record = {0};

    while (ras_agent_probe(agent, &record))
    {
        ras_print_record(&record);
        if (record.handler)
        {
            status = record.handler(&record);
            if (status)
            {
                FPFW_DBGPRINT_ALWAYS("Error encountered while handling TCU record\n");
                return status;
            }
        }
        else
        {
            FPFW_DBGPRINT_ALWAYS("TCU Record was marked as invalid! No further handling will be done\n");
            continue;
        }
    }

    return SILIBS_SUCCESS;
}

static silibs_status_t vab_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base, vab_error_component_t component)
{
    /* All parity errors in the VAB are considered fatal */
    UNUSED(vab_id);
    uint32_t data;

    silibs_status_t status = vab_get_1p_parity_status(base, &data);
    if (status)
    {
        data = 0;
    }

    acpi_err_sec_vab_t vab_cper = {.error_type = VAB_PARITY_FAULT,
                                   .component = component,
                                   .parity_error_data = {.csr.raw_data = (data & 0x7), .fabric.raw_data = (data >> 4)},
                                   .record.flags.record_valid = false};
    acpi_cper_section_t cper_section;
    cper_section.sec_vab = vab_cper;
    hm_submit_cper(ACPI_ERROR_DOMAIN_VAB, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(cper_section));

    BUG_CHECK(VAB_PARITY_FAULT, component, data);
    return SILIBS_E_PANIC;
}

silibs_status_t vab_fabric_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    vab_parity_handler(vab_id, base, VAB_FABRIC);

    /* VAB Parity errors will always bugcheck so this return is bogus */
    return SILIBS_E_PANIC;
}

silibs_status_t vab_csr_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    vab_parity_handler(vab_id, base, VAB_CSR);

    /* VAB Parity errors will always bugcheck so this return is bogus */
    return SILIBS_E_PANIC;
}

silibs_status_t vab_pcr_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    vab_parity_handler(vab_id, base, VAB_PCR);

    /* VAB Parity errors will always bugcheck so this return is bogus */
    return SILIBS_E_PANIC;
}

silibs_status_t vab_integ_pcr_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    /* VAB Integration-level parity faults are reported in the VAB domain */
    UNUSED(vab_id);
    UNUSED(base);

    // Log CPER
    acpi_err_sec_vab_t vab_cper = {.error_type = VAB_INTEG_PARITY_FAULT,
                                   .component = VAB_INTEG_PCR,
                                   /* Though these fields are RESERVED for the VAB_INTEG_PARITY_FAULT type, we set them to 0 for cleanliness */
                                   .parity_error_data = {.csr.raw_data = 0, .fabric.raw_data = 0},
                                   .record.flags.record_valid = false};

    acpi_cper_section_t cper_section;
    cper_section.sec_vab = vab_cper;
    hm_submit_cper(ACPI_ERROR_DOMAIN_VAB, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(cper_section));

    BUG_CHECK(VAB_INTEG_PARITY_FAULT, VAB_INTEG_PCR, 0x0);

    /* We always bugcheck above so this return is bogus */
    return SILIBS_E_PANIC;
}

silibs_status_t vab_integ_csr_parity_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    /* VAB Integration-level parity faults are reported in the VAB domain */
    UNUSED(vab_id);
    UNUSED(base);

    acpi_err_sec_vab_t vab_cper = {.error_type = VAB_INTEG_PARITY_FAULT,
                                   .component = VAB_INTEG_CSR,
                                   // Note: Though these fields are RESERVED for the VAB_INTEG_PARITY_FAULT type, we set them to 0 for cleanliness
                                   .parity_error_data = {.csr.raw_data = 0, .fabric.raw_data = 0},
                                   .record.flags.record_valid = false};
    acpi_cper_section_t cper_section;
    cper_section.sec_vab = vab_cper;
    hm_submit_cper(ACPI_ERROR_DOMAIN_VAB, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(cper_section));

    BUG_CHECK(VAB_INTEG_PARITY_FAULT, VAB_INTEG_CSR, 0x0);

    /* We always bugcheck above so this return is bogus */
    return SILIBS_E_PANIC;
}

silibs_status_t vab_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    DIE_INSTANCE die;
    if (vab_id <= D0_VAB3_RPSS3 || vab_id == D0_VAB4_SDMSS || vab_id == D0_VAB5_CDEDSS_IOSS)
    {
        die = SOC_D0;
    }
    else
    {
        die = SOC_D1;
    }
    return tower_fmu_handler(vab_convert_vab_id_to_vab_tower_id(vab_id), die, base + RAS_VAB_TOWER_OFFSET);
}
