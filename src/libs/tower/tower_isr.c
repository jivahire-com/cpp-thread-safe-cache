//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file tower_isr.c
 *  ISR interfaces for NI-Tower instances
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <FpFwAssert.h>
#include <bug_check.h>
#include <cper.h>
#include <health_monitor.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_atu_mappings.h>
#include <stdbool.h>
#include <stdio.h>
#include <tower_fmu_utility.h>
#include <tower_isr.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
typedef struct _tower_bugcheck_info_t
{
    KNG_STATUS error_code;
    uint32_t instance;
    uint32_t node_type_id;
    bool valid;
} tower_bugcheck_info_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static tower_bugcheck_info_t tower_bugcheck_info = {0};

/*
 * We store addresses instead of ATU map entries to save on space.
 * For Towers not within the scope of MSCP, an explicit base address of 0 is
 * used as a guardrail.
 */
static uint64_t tower_addresses[TOWER_MAX_TOWERS_PER_DIE * NUM_DIE] = {
    /* Die 0 */
    0,
    0,
    AP_TOP_D0_FABRIC_NIC_GPV_ADDRESS,
    AP_TOP_D0_PERIP_NIC_GPV_ADDRESS,
    AP_TOP_D0_D2D03_NIC_GPV_ADDRESS,
    AP_TOP_D0_D2D47_NIC_GPV_ADDRESS,
    0,
    0,
    0,
    0,
    0,
    0,
    AP_TOP_D0_VAB_RPSS0_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D0_VAB_RPSS1_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D0_VAB_RPSS2_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D0_VAB_RPSS3_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D0_VAB_RPSS0_ADDRESS + VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RPSS_TOWER_ADDRESS,
    AP_TOP_D0_VAB_RPSS1_ADDRESS + VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RPSS_TOWER_ADDRESS,
    AP_TOP_D0_VAB_RPSS2_ADDRESS + VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RPSS_TOWER_ADDRESS,
    AP_TOP_D0_VAB_RPSS3_ADDRESS + VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RPSS_TOWER_ADDRESS,
    AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS + SDMSS_CONFIG_SDMSS_TOWER_ADDRESS,
    0,
    AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS + IOSS_TOP_NOCNI_NITOWER_IOSS_ADDRESS,
    0,
    0,
    0,
    0,
    /* Die 1 */
    0,
    0,
    AP_TOP_D1_FABRIC_NIC_GPV_ADDRESS,
    AP_TOP_D1_PERIP_NIC_GPV_ADDRESS,
    AP_TOP_D1_D2D03_NIC_GPV_ADDRESS,
    AP_TOP_D1_D2D47_NIC_GPV_ADDRESS,
    0,
    0,
    0,
    0,
    0,
    0,
    AP_TOP_D1_VAB_RPSS0_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D1_VAB_RPSS1_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D1_VAB_RPSS2_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D1_VAB_RPSS3_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D1_VAB_SDM_ADDRESS + VAB_SDM_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_VAB_ADDRESS + VAB_VAB_TOWER_ADDRESS,
    AP_TOP_D1_VAB_RPSS0_ADDRESS + VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RPSS_TOWER_ADDRESS,
    AP_TOP_D1_VAB_RPSS1_ADDRESS + VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RPSS_TOWER_ADDRESS,
    AP_TOP_D1_VAB_RPSS2_ADDRESS + VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RPSS_TOWER_ADDRESS,
    AP_TOP_D1_VAB_RPSS3_ADDRESS + VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RPSS_TOWER_ADDRESS,
    AP_TOP_D1_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS + SDMSS_CONFIG_SDMSS_TOWER_ADDRESS,
    0,
    AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS + IOSS_TOP_NOCNI_NITOWER_IOSS_ADDRESS,
    0,
    0,
    0,
    0,
};

/*
 * To save on code/data space, we leverage a single map with the largest size
 * allocation that we redirect to the requested Tower. Though this technically
 * maps more than just the Tower in some cases, it is benign.
 */
static atu_map_entry_t map = ATU_MAPPING_D0_VAB0_RPSS0_TOWER();

/*------------- Functions ----------------*/
static bool tower_get_map(TOWER_INSTANCE tower_id, DIE_INSTANCE die)
{
    map.ap_base_address = tower_addresses[TOWER_MAX_TOWERS_PER_DIE * die + tower_id];
    FPFW_DBGPRINT_ALWAYS("Tower base: %llx\n", map.ap_base_address);
    if (!map.ap_base_address)
    {
        return false;
    }
    FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &map));
    return true;
}

silibs_status_t tower_fmu_handler(TOWER_INSTANCE tower_id, DIE_INSTANCE die, uintptr_t base)
{
    silibs_status_t status = SILIBS_SUCCESS;
    ras_agent_entity_t* agent = tower_get_ras_agent_entity(tower_id);
    if (tower_id >= TOWER_MAX_TOWERS_PER_DIE || die >= NUM_DIE)
    {
        return SILIBS_E_PARAM;
    }
    if (!base)
    {
        if (!tower_get_map(tower_id, die))
        {
            return SILIBS_E_SUPPORT;
        }
        base = map.mscp_start_address;
    }
    if (ras_arm_fmu_agent_set_base(agent, base, tower_id))
    {
        /*
         * This occurs whenever the agent is not live, or in other words is not
         * setup for this core.
         */
        return SILIBS_E_SUPPORT;
    }

    ras_error_record_t record = {0};
    while (ras_agent_probe(agent, &record))
    {
        ras_print_record(&record);

        acpi_cper_section_t cper_section;
        acpi_err_sec_nitower_t* tower_cper = &cper_section.sec_nitower;
        uint32_t severity;
        if (ras_agent_record_to_cper(&record, tower_cper, sizeof(acpi_err_sec_nitower_t), &severity))
        {
            FPFW_DBGPRINT_ALWAYS("Unable to convert RAS record to CPER!\n");
        }
        else
        {
            hm_submit_cper(ACPI_ERROR_DOMAIN_NITOWER, severity, &cper_section, sizeof(cper_section));
        }

        if (severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
        {
            /* Setup bugcheck information if this is the first fatal error */
            if (!tower_bugcheck_info.valid)
            {
                /* TODO: This can be replaced with a more granular error code if needed. */
                tower_bugcheck_info.error_code = KNG_HM_PCIE_GENERIC;
                tower_bugcheck_info.instance = tower_id;
                tower_bugcheck_info.node_type_id = record.misc_0;
                tower_bugcheck_info.valid = true;
            }
        }
        if (record.handler)
        {
            if (record.handler(&record))
            {
                FPFW_DBGPRINT_ALWAYS("Error encountered while handling Tower record\n");
                status = SILIBS_E_STATE;
            }
        }
        else
        {
            FPFW_DBGPRINT_ALWAYS("Tower Record was marked as invalid! No further handling will be done\n");
            continue;
        }
    }

    if (tower_bugcheck_info.valid)
    {
        BUG_CHECK(tower_bugcheck_info.error_code, tower_bugcheck_info.instance, tower_bugcheck_info.node_type_id);
    }

    map.ap_base_address = UINT64_MAX;
    atu_unmap(ATU_ID_MSCP, &map);

    return status;
}

acpi_einj_cmd_status_t tower_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);
    FPFW_DBGPRINT_INFO("|Tower| Info: Error Injection Callback Start\n");
    if (einj_payload == NULL)
    {
        FPFW_DBGPRINT_ALWAYS("|Tower| Error: NULL injection payload\n");
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_NITOWER)
    {
        FPFW_DBGPRINT_ALWAYS("|Tower| Error: Unexpected error domain: %d\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    /*
     * component_type     - refers to TOWER_INSTANCE, and
     * component_instance - refers to the die
     *
     * e.g: D2DSS0 Tower on Die-0 would be (type, instance) => (TOWER_D2DSS0, SOC_D0)
     */
    if (idhw_is_single_die_boot_en() == false)
    {
        if (einj_payload->component_instance != idsw_get_die_id())
        {
            FPFW_DBGPRINT_ALWAYS("|Tower| Error: Invalid component instance %d\n", einj_payload->component_instance);
            return ACPI_EINJ_INVALID_ACCESS;
        }
    }

    /* Cast generic to expected parameter structure */
    tower_error_inj_op_type_t op_type;
    op_type.as_uint64 = einj_payload->param_type.error_parameters[0];
    tower_error_inj_param_t params;
    params.as_uint64 = einj_payload->param_type.error_parameters[1];
    silibs_status_t status;
    acpi_einj_cmd_status_t ret = ACPI_EINJ_INVALID_ACCESS;
    TOWER_INSTANCE tower_id = (TOWER_INSTANCE)einj_payload->component_type;
    if (tower_id >= TOWER_MAX_TOWERS_PER_DIE)
    {
        return ACPI_EINJ_INVALID_ACCESS;
    }

    /* Get RAS handle */
    ras_agent_entity_t* agent = tower_get_ras_agent_entity(tower_id);
    if (!agent)
    {
        return ACPI_EINJ_UNKNOWN_FAILURE;
    }

    /* All failures after ATU map will goto exit */
    if (!tower_get_map(tower_id, einj_payload->component_instance))
    {
        return ACPI_EINJ_INVALID_ACCESS;
    }

    uintptr_t tower_base = map.mscp_start_address;
    status = ras_arm_fmu_agent_set_base(agent, tower_base, tower_id);
    if (status)
    {
        return ACPI_EINJ_INVALID_ACCESS;
    }

    switch ((TOWER_EINJ_OPERATION)op_type.op)
    {
    case TOWER_EINJ_TARGET_BY_NODE:
        tower_node_entity_t tower_node = {.node_type = params.by_node.node_type,
                                          .node_id = params.by_node.node_id,
                                          .clock_domain_id = params.by_node.clock_domain_id,
                                          .power_domain_id = params.by_node.power_domain_id,
                                          .voltage_domain_id = params.by_node.voltage_domain_id};
        status = tower_fmu_inject_single_error(tower_base, tower_id, tower_node, op_type.type);
        if (status)
        {
            goto exit;
        }
        break;
    /* All of the below fall-through for now */
    case TOWER_EINJ_TARGET_BY_INDEX:
        /*
         * Note: This block can be uncommented once SiLibs PR {304712} to re-add this function clears
         * tower_fmu_inj_t inj = {.payload = {.err = op_type.type, .fmu_index = params.by_index.index}};
         * status = ras_arm_fmu_agent_trigger_by_type(agent, inj.generic);
         * if (status)
         * {
         *     goto exit;
         * }
         * break;
         */
    case TOWER_EINJ_TARGET_RAW:
        /*
         * Note: This block can be uncommented once SiLibs PR {304712} to re-add this function clears
         * status = ras_arm_fmu_agent_trigger_by_type(agent, einj_payload->param_type.error_parameters[1]);
         * if (status)
         * {
         *     goto exit;
         * }
         * break;
         */
    default:
        FPFW_DBGPRINT_ALWAYS("|Tower| Error: Invalid injection type: (%d)\n", op_type.op);
        break;
    }

    ret = ACPI_EINJ_SUCCESS;

exit:
    map.ap_base_address = UINT64_MAX;
    FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &map));
    return ret;
}
