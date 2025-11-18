/*
 * Copyright (c) Microsoft Corporation.
 *
 * Description:
 *      functions to support the AP error domain.  This domain includes errors that occur in the AP space that
 * don't have a specific Error Domain.  Currently, this file only handles the injection method for the AEST OS
 * first error domains The actual reports are via ARM compliant RAS error nodes and are handled by the OS.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <atu_api.h>
#include <bug_check.h>
#include <cper.h>
#include <error_domain_i.h>
#include <health_monitor.h>
#include <idsw_kng.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <mscp_error_domain.h>
#include <nvic.h>
#include <smmu_yardley_eac_vab_regs.h>
#include <utils.h>
#include <vab_tbu_ace_lite_regs.h>
#include <vab_tbu_lti_x4_regs.h>
#include <vab_tcu_x2_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <ap_top_regs.h>

// clang-format off
#include <cmsis_m7.h>
#ifdef PLATFORM_CACHING_ENABLED
#include <cmsis_compiler.h>
#include <m-profile/armv7m_cachel1.h>
#endif
// clang-format on

/*-- Symbolic Constant Macros (defines) --*/
#define MODULE_NAME             "[SMMU] "
#define SMMU_LOG_INFO(fmt, ...) FPFW_DBGPRINT_INFO(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define SMMU_LOG_WARN(fmt, ...) FPFW_DBGPRINT_WARNING(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

#define MAX_VAB_RPSS_INSTANCES 8
#define ATU_VAB_RPSS_DIE0_MAP  0
#define ATU_VAB_RPSS_DIE1_MAP  4

static atu_map_entry_t atu_vab_rpss_map[MAX_VAB_RPSS_INSTANCES] = {
    ATU_MAPPING_D0_VAB0_RPSS0(),
    ATU_MAPPING_D0_VAB1_RPSS1(),
    ATU_MAPPING_D0_VAB2_RPSS2(),
    ATU_MAPPING_D0_VAB3_RPSS3(),
    ATU_MAPPING_D1_VAB0_RPSS0(),
    ATU_MAPPING_D1_VAB1_RPSS1(),
    ATU_MAPPING_D1_VAB2_RPSS2(),
    ATU_MAPPING_D1_VAB3_RPSS3(),
};
/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

uint32_t map_ap_address(atu_map_entry_t* atu_entry, VAB_ID_PER_DIE rpss)
{
    BUG_ASSERT(atu_entry != NULL);
    KNG_DIE_ID die_id = idsw_get_die_id();

    uint8_t vab_id = (die_id == DIE_0) ? ATU_VAB_RPSS_DIE0_MAP : ATU_VAB_RPSS_DIE1_MAP;

    vab_id += rpss;

    *atu_entry = atu_vab_rpss_map[vab_id];

    int status = atu_map(ATU_ID_MSCP, atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);

    return atu_entry->mscp_start_address;
}

void unmap_ap_address(atu_map_entry_t* atu_entry)
{
    int status = atu_unmap(ATU_ID_MSCP, atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);
}

/**
 *  Error Injection callback for error domain
 *
 *  @param einj_payload
 *      Pointer to error injection structure handed in from the host services
 *
 *  @param ctx
 *      unused void context
 *
 *  @return
 *      hm_cmd_status_t status of the injection request
 */
PLACED_CODE acpi_einj_cmd_status_t hm_smmu_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);
    uint64_t err_ins_addr = 0;
    atu_map_entry_t ap_atu_entry;

    // Check if einj_payload is NULL
    if (einj_payload == NULL)
    {
        SMMU_LOG_WARN("einj_payload is NULL\n");
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Check input parameters and get the selected injection register
    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_SMMU)
    {
        SMMU_LOG_WARN("Invalid AP error domain(%d)\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (einj_payload->component_type > (uint16_t)VAB3_RPSS3)
    {
        SMMU_LOG_WARN("Invalid VAB RPSS Input (%d)\n", einj_payload->component_type);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (einj_payload->component_group == ACPI_ERROR_DOMAIN_SMMU)
    {
        SMMU_LOG_INFO("Injecting SMMU error");

        uint64_t base_addr = map_ap_address(&ap_atu_entry, (VAB_ID_PER_DIE)einj_payload->component_type);

        switch (einj_payload->param_type.error_type)
        {
        case AP_SMMU_ERROR_TCU_VAB:
            SMMU_LOG_INFO("AP_SMMU_ERROR_TCU_VAB");
            err_ins_addr = base_addr + SMMU_YARDLEY_EAC_VAB_TCU_VAB_ADDRESS + VAB_TCU_X2_TCU_ERRGEN_ADDRESS;
            break;
        case AP_SMMU_ERROR_TBU_LTI_X4_VAB:
            SMMU_LOG_INFO("AP_SMMU_ERROR_TBU_LTI_X4_VAB");
            err_ins_addr = base_addr + SMMU_YARDLEY_EAC_VAB_TBU_LTI_X4_VAB_ADDRESS + VAB_TBU_LTI_X4_TBU_ERRGEN_ADDRESS;
            break;
        case AP_SMMU_ERROR_TBU_ACE_LITE_VAB:
            SMMU_LOG_INFO("AP_SMMU_ERROR_TBU_ACE_LITE_VAB");
            err_ins_addr = base_addr + SMMU_YARDLEY_EAC_VAB_TBU_ACE_LITE_VAB_ADDRESS + VAB_TBU_ACE_LITE_TBU_ERRGEN_ADDRESS;
            break;
        default:
            SMMU_LOG_WARN("Invalid/Unsupported AP error block(%d)\n", einj_payload->param_type.error_type);
            return ACPI_EINJ_INVALID_ACCESS;
            break;
        }

        // Write the injection word
        MMIO_WRITE32(err_ins_addr, einj_payload->value_type.error_values);

        SMMU_LOG_INFO("err_ins_addr : %d", err_ins_addr);
        SMMU_LOG_INFO("err_values injected: %d", einj_payload->value_type.error_values);

        unmap_ap_address(&ap_atu_entry);
    }
    else
    {
        SMMU_LOG_WARN("Invalid/Unsupported Error Domain (%d)\n", einj_payload->component_group);
        return ACPI_EINJ_UNKNOWN_FAILURE;
    }

    return ACPI_EINJ_SUCCESS;
}