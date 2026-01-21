//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh_error_handler.c
 *  This file holds the Mesh Error Handler ISR functions
 */

/*------------- Includes -----------------*/
#include <atu_lib.h>
#include <bug_check.h>
#include <cmn800.h>
#include <cmn800_error_handler.h> // for acpi_err_sec_generic_t
#include <cmn800_sequence.h>      // for cmn800_sequence, d2dss_sequence, cmn8...
#include <cmn_config.h>           // for CMN800_CONFIG_CONFIG
#include <d2d_ras_regs.h>
#include <d2dss_cxs_regs.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_ctx_t
#include <idhw.h>          // for idhw_is_single_die_boot_en
#include <idsw.h>          // for idsw_get_platform_sdv,
#include <idsw_kng.h>      // for PLATFORM_FPGA_LARGE
#include <kng_atu_mappings.h>
#include <kng_error.h>
#include <kng_soc_constants.h> // for NUM_DIE
#include <mesh.h>              // for mesh_init
#include <mesh_error_handler.h>
#include <ras_arm.h>
#include <silibs_ap_top_regs.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <stdio.h>   // for MESH_INFO
#include <system_info.h>
#include <utils.h>
#include <vab_regs.h>
/*------------- Defines -----------------*/

ras_agent_entity_t d2dss2_agent[NUM_OF_CCG_WITH_D2D] = {0};
bool g_mesh_error_print = false;
// Create a FW CPER
static acpi_err_sec_generic_t mesh_cper = {0x0};
static ras_error_record_t record = {0x0};

atu_map_entry_t atu_d2d2_map[NUM_OF_CCG_WITH_D2D] = {
    {.ap_base_address = AP_TOP_D0_D2DSS_0_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ATU_D2D_MAP_END_ADDRESS,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_1_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ATU_D2D_MAP_END_ADDRESS,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_2_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ATU_D2D_MAP_END_ADDRESS,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_3_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ATU_D2D_MAP_END_ADDRESS,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_4_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ATU_D2D_MAP_END_ADDRESS,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_5_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ATU_D2D_MAP_END_ADDRESS,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_6_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ATU_D2D_MAP_END_ADDRESS,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_7_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ATU_D2D_MAP_END_ADDRESS,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}}};
/*------------- Functions ----------------*/

void mesh_error_print(bool enable)
{
    g_mesh_error_print = enable;
}

/**
 * Function to print the Mesh CPER structure that is filled
 **/
static void print_mesh_cper(acpi_err_sec_generic_t* mesh_cper)
{
    if (mesh_cper == NULL)
    {
        MESH_INFO("mesh_cper is NULL\n");
        return;
    }
    MESH_INFO("print_mesh_cper Mesh Error\n");
    switch (mesh_cper->node_id.vendor_specific_data[MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_0])
    {
    case MESH_ERROR_NODE_ID_HNF:
        MESH_INFO("NODE_ID_HNF\n");
        break;
    case MESH_ERROR_NODE_ID_MXP:
        MESH_INFO("NODE_ID_MXP\n");
        break;
    case MESH_ERROR_NODE_ID_HNI:
        MESH_INFO("NODE_ID_HNI\n");
        break;
    case MESH_ERROR_NODE_ID_HNP:
        MESH_INFO("NODE_ID_HNP\n");
        break;
    case MESH_ERROR_NODE_ID_SBSX:
        MESH_INFO("NODE_ID_SBSX\n");
        break;
    case MESH_ERROR_NODE_ID_CCG:
        MESH_INFO("NODE_ID_CCG\n");
        break;
    case MESH_ERROR_NODE_ID_D2D:
        MESH_INFO("NODE_ID_D2D\n");
        break;
    default:
        MESH_INFO("NODE_ID_UNKNOWN\n");
        break;
    }
    switch (mesh_cper->node_id.vendor_specific_data[MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_1])
    {
    case MESH_ERROR_RAS_ERROR:
        MESH_INFO("MESH_ERROR_RAS_ERROR\n");
        break;
    case MESH_ERROR_RAS_FAULT:
        MESH_INFO("MESH_ERROR_RAS_FAULT\n");
        break;
    case D2D_ERROR_RAS_ERROR:
        MESH_INFO("D2D_ERROR_RAS_ERROR\n");
        break;
    case D2D_ERROR_RAS_FAULT:
        MESH_INFO("D2D_ERROR_RAS_FAULT\n");
        break;
    case MESH_ERROR_RAS_ERROR_TYPE_MAX:
        MESH_INFO("MESH_ERROR_RAS_ERROR_TYPE_MAX\n");
        break;
    default:
        MESH_INFO("RAS_TYPE_UNKNOWN\n");
        break;
    }
    if (g_mesh_error_print == true)
    {
        MESH_INFO("Mesh Node Index Offset 0x%lx\n",
                  mesh_cper->node_id.vendor_specific_data[MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_2]);
        MESH_INFO("Mesh Node Index Number 0x%lx\n",
                  mesh_cper->node_id.vendor_specific_data[MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_3]);
        MESH_INFO("Error Record Number 0x%lx\n", mesh_cper->err_record_num);
        MESH_INFO("Error Record Version 0x%x\n", mesh_cper->version);
        MESH_INFO("Error Record Die Id 0x%x\n", mesh_cper->die_id);
        MESH_INFO("Error Record Feature 0x%08x_%08x\n",
                  (uint32_t)(mesh_cper->err_fr >> 32),
                  (uint32_t)mesh_cper->err_fr);
        MESH_INFO("Error Record Control 0x%08x_%08x\n",
                  (uint32_t)(mesh_cper->err_ctrl >> 32),
                  (uint32_t)mesh_cper->err_ctrl);
        MESH_INFO("Error Record Status 0x%08x_%08x\n",
                  (uint32_t)(mesh_cper->err_status >> 32),
                  (uint32_t)mesh_cper->err_status);
        MESH_INFO("Error Record Address 0x%08x_%08x\n",
                  (uint32_t)(mesh_cper->err_addr >> 32),
                  (uint32_t)mesh_cper->err_addr);
        MESH_INFO("Error Record Misc0 0x%08x_%08x\n",
                  (uint32_t)(mesh_cper->err_misc0 >> 32),
                  (uint32_t)mesh_cper->err_misc0);
        MESH_INFO("Error Record Misc1 0x%08x_%08x\n",
                  (uint32_t)(mesh_cper->err_misc1 >> 32),
                  (uint32_t)mesh_cper->err_misc1);
    }
}

/**
 * Function to decode the Mesh Error Status
 **/
void fnc_decode_mesh_cper_status(acpi_err_sec_generic_t* mesh_cper, uint8_t* acpi_severity)
{
    print_mesh_cper(mesh_cper);

    if ((mesh_cper->err_status) & BIT29)
    { // UE
        *acpi_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL;
        MESH_INFO("UE - ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL");
    }
    else if ((mesh_cper->err_status) & BIT23)
    { // DE
        *acpi_severity = ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL;
        MESH_INFO("DE - ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL");
    }
    else if ((mesh_cper->err_status) & BIT25)
    { // CE
        *acpi_severity = ACPI_ERROR_SEVERITY_CORRECTED;
        MESH_INFO("CE - ACPI_ERROR_SEVERITY_CORRECTED");
    }
    else
    {
        *acpi_severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;
        MESH_INFO("ACPI_ERROR_SEVERITY_INFORMATIONAL");
    }
}

/**
 * Mesh Fault ISR
 * This function is called when a Mesh Fault ISR is triggered by the hardware INT
 * @param context
 * @return void
 **/
void mesh_fault_isr(void* context)
{
    UNUSED(context);

    MESH_CRIT("Mesh Fault ISR\n");

    // Clear mesh_cper
    mesh_cper = (acpi_err_sec_generic_t){0x0};

    mesh_error_print(true);

    interrupt_handler_mesh_ras_error(&mesh_cper, true, false, (uint8_t)idhw_get_die_id());

    print_mesh_cper(&mesh_cper);

    mesh_error_print(false);

    uint8_t acpi_severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;
    acpi_cper_section_t cper_section = {0};
    cper_section.sec_mesh = mesh_cper;
    fnc_decode_mesh_cper_status(&mesh_cper, &acpi_severity);
    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, acpi_severity, &cper_section, sizeof(cper_section));

    if (acpi_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
    {
        // If the severity is fatal, we need to bug check
        MESH_CRIT("Fatal Fault in Mesh, Bug Check\n");
        BUG_CHECK(KNG_E_RAS_MESH_FAULT_UE, 0, 0);
    }
}

/**
 * Mesh Error ISR
 * This function is called when a Mesh Error ISR is triggered by the hardware INT
 * @param context
 * @return void
 **/
void mesh_error_isr(void* context)
{
    UNUSED(context);

    MESH_CRIT("Mesh Error ISR\n");

    // Clear mesh_cper
    mesh_cper = (acpi_err_sec_generic_t){0x0};

    mesh_error_print(true);

    interrupt_handler_mesh_ras_error(&mesh_cper, false, false, (uint8_t)idhw_get_die_id());

    print_mesh_cper(&mesh_cper);

    mesh_error_print(false);

    uint8_t acpi_severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;
    acpi_cper_section_t cper_section = {0};
    cper_section.sec_mesh = mesh_cper;
    fnc_decode_mesh_cper_status(&mesh_cper, &acpi_severity);
    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, acpi_severity, &cper_section, sizeof(cper_section));

    if (acpi_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
    {
        // If the severity is fatal, we need to bug check
        MESH_CRIT("Fatal Error in Mesh, Bug Check\n");
        BUG_CHECK(KNG_E_RAS_MESH_ERROR_UE, 0, 0);
    }
}

/**
 * Mesh Non-Secure Fault ISR
 * This function is called when a Mesh Non-Secure Fault ISR is triggered by the hardware INT
 * @param context
 * @return void
 **/
void mesh_ns_fault_isr(void* context)
{
    UNUSED(context);

    MESH_CRIT("Mesh NS Fault ISR\n");

    // Clear mesh_cper
    mesh_cper = (acpi_err_sec_generic_t){0x0};

    mesh_error_print(true);

    interrupt_handler_mesh_ras_error(&mesh_cper, true, true, (uint8_t)idhw_get_die_id());

    print_mesh_cper(&mesh_cper);

    mesh_error_print(false);

    uint8_t acpi_severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;
    acpi_cper_section_t cper_section = {0};
    cper_section.sec_mesh = mesh_cper;
    fnc_decode_mesh_cper_status(&mesh_cper, &acpi_severity);
    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, acpi_severity, &cper_section, sizeof(cper_section));

    if (acpi_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
    {
        // If the severity is fatal, we need to bug check
        MESH_CRIT("Fatal Non-Secure Fault in Mesh, Bug Check\n");
        BUG_CHECK(KNG_E_RAS_MESH_NON_SECURE_FAULT_UE, 0, 0);
    }
}

/**
 * Mesh Non-Secure Error ISR
 * This function is called when a Mesh Non-Secure Error ISR is triggered by the hardware INT
 * @param context
 * @return void
 **/
void mesh_ns_error_isr(void* context)
{
    UNUSED(context);

    MESH_CRIT("Mesh NS Error ISR\n");

    // Clear mesh_cper
    mesh_cper = (acpi_err_sec_generic_t){0x0};

    mesh_error_print(true);

    interrupt_handler_mesh_ras_error(&mesh_cper, false, true, (uint8_t)idhw_get_die_id());

    print_mesh_cper(&mesh_cper);

    mesh_error_print(false);

    uint8_t acpi_severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;
    acpi_cper_section_t cper_section = {0};
    cper_section.sec_mesh = mesh_cper;
    fnc_decode_mesh_cper_status(&mesh_cper, &acpi_severity);
    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, acpi_severity, &cper_section, sizeof(cper_section));

    if (acpi_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
    {
        // If the severity is fatal, we need to bug check
        MESH_CRIT("Fatal Non-Secure Error in Mesh, Bug Check\n");
        BUG_CHECK(KNG_E_RAS_MESH_NON_SECURE_ERROR_UE, 0, 0);
    }
}

uint32_t d2d_set_atu_map(uint8_t d2d_subsystem, bool* d2d_atu_unmap_required)
{
    uint32_t translated_addr_ras2 = 0x0;
    MESH_CRIT("D2D2:: Setting ATU Map for D2D Subsystem %d\n", d2d_subsystem);

    // Create a temp entry for the ATU Map
    atu_map_entry_t temp_atu_map_entry = {.ap_base_address = atu_d2d2_map[d2d_subsystem].ap_base_address,
                                          .mscp_start_address = atu_d2d2_map[d2d_subsystem].mscp_start_address,
                                          .mscp_end_address = atu_d2d2_map[d2d_subsystem].mscp_end_address,
                                          .attribute = atu_d2d2_map[d2d_subsystem].attribute};

    if ((atu_find_map(ATU_ID_MSCP, &temp_atu_map_entry) == SILIBS_SUCCESS))
    {
        MESH_CRIT("D2D2:: ATU Map already exists for D2D Subsystem %d\n", d2d_subsystem);
        *d2d_atu_unmap_required = false;
    }
    else
    {
        BUG_ASSERT(!atu_map(ATU_ID_MSCP, &temp_atu_map_entry));
        *d2d_atu_unmap_required = true;
    }

    BUG_ASSERT_PARAM(!atu_translate_address(ATU_ID_MSCP, temp_atu_map_entry.ap_base_address, &translated_addr_ras2),
                     temp_atu_map_entry.ap_base_address,
                     0);

    MESH_CRIT("D2D2:: d2d2_ras_address 0x%08x_%08x, translated_addr_ras2 = 0x%x \n",
              (uint32_t)(temp_atu_map_entry.ap_base_address >> 32),
              (uint32_t)(temp_atu_map_entry.ap_base_address & 0xFFFFFFFF),
              translated_addr_ras2);

    return translated_addr_ras2;
}

void d2d_clear_atu_map(uint8_t d2d_subsystem, uint32_t translated_addr_ras2)
{
    MESH_CRIT("D2D2:: Clearing ATU Map for D2D Subsystem %d Start\n", d2d_subsystem);
    // Create a temp entry for the ATU Map
    atu_map_entry_t temp_atu_map_entry = {.ap_base_address = UINT64_MAX,
                                          .mscp_start_address = translated_addr_ras2,
                                          .mscp_end_address = (translated_addr_ras2 + ATU_D2D_MAP_END_ADDRESS),
                                          .attribute = atu_d2d2_map[d2d_subsystem].attribute};

    // Call ATU Unmap
    BUG_ASSERT(!atu_unmap(ATU_ID_MSCP, &temp_atu_map_entry));

    MESH_CRIT("D2D2:: Clearing ATU Map for D2D Subsystem %d End\n", d2d_subsystem);
}

int mesh_error_handler_convert_d2d_ras_record_to_cper(ras_error_record_t* record, acpi_err_sec_generic_t* mesh_cper, uint8_t d2d_subsystem)
{
    if (record == NULL || mesh_cper == NULL)
    {
        MESH_CRIT("Invalid Record or Mesh CPER\n");
        return SILIBS_E_DEVICE;
    }

    // Convert the RAS record to CPER
    mesh_cper->node_id.vendor_specific_data[MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_0] = MESH_ERROR_NODE_ID_D2D;
    mesh_cper->node_id.vendor_specific_data[MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_1] = D2D_ERROR_RAS_ERROR;
    mesh_cper->node_id.vendor_specific_data[MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_2] = d2d_subsystem;
    mesh_cper->node_id.vendor_specific_data[MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_3] = 0;
    mesh_cper->err_record_num = 0;                  // Error Record Number
    mesh_cper->version = 0;                         // Version of the CPER
    mesh_cper->die_id = (uint8_t)idhw_get_die_id(); // Die ID of the SoC

    // Fill in the error record details
    mesh_cper->err_fr = record->syndrome;
    mesh_cper->err_ctrl = record->status; // Status is used for Error Control Register
    mesh_cper->err_status = record->status;
    mesh_cper->err_addr = record->err_address;
    mesh_cper->err_misc0 = record->misc_0;
    mesh_cper->err_misc1 = record->misc_1;
    mesh_cper->err_misc2 = record->misc_2;
    mesh_cper->err_misc3 = record->misc_3;

    return SILIBS_SUCCESS;
}

/**
 * D2D Error ISR
 * This function is called when a D2D Error ISR is triggered by the hardware INT
 * @param context
 * @return void
 **/
void d2d_error_isr(void* context)
{
    UNUSED(context);
    MESH_CRIT("D2D Error ISR\n");
    uint32_t translated_addr_ras2 = 0x0;
    uint8_t acpi_severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;
    bool d2d_atu_unmap_required = true;

    // Clear the d2d ras error record
    record = (ras_error_record_t){0};
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        translated_addr_ras2 = d2d_set_atu_map(d2d_subsystem, &d2d_atu_unmap_required);

        // Update the Base
        int status = ras_arm_agent_set_base(&d2dss2_agent[d2d_subsystem], translated_addr_ras2);
        if (status != SILIBS_SUCCESS)
        {
            MESH_CRIT("Failed to set base addr, D2DSS2_AGENT%d\n", d2d_subsystem);
            goto d2d_ras_error_isr_exit;
        }
        bool error_found = ras_arm_agent_probe(&d2dss2_agent[d2d_subsystem], &record);
        if (error_found == true)
        {
            MESH_CRIT("Error found in D2DSS2_AGENT%d\n", d2d_subsystem);
            ras_print_record(&record);
            // Convert the error record to CPER
            // Clear mesh_cper
            mesh_cper = (acpi_err_sec_generic_t){0x0};
            int status = mesh_error_handler_convert_d2d_ras_record_to_cper(&record, &mesh_cper, d2d_subsystem);
            if (status == SILIBS_SUCCESS)
            {
                MESH_CRIT("Converted D2D RAS Record to CPER successfully\n");
                // Print the CPER
                mesh_error_print(true);
                print_mesh_cper(&mesh_cper);
                mesh_error_print(false);
                // Submit the CPER to the Health Monitor
                acpi_cper_section_t cper_section = {0};
                cper_section.sec_mesh = mesh_cper;
                fnc_decode_mesh_cper_status(&mesh_cper, &acpi_severity);
                hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, acpi_severity, &cper_section, sizeof(cper_section));
            }
            else
            {
                MESH_CRIT("Failed to convert D2D RAS Record to CPER, status: %d\n", status);
            }
            if (record.handler)
            {
                if (record.handler(&record)) // Clears the Error Record Status Register
                {
                    MESH_CRIT("Error encountered while handling record\n");
                }
            }
            else
            {
                MESH_CRIT("Invalid Record, No Further Action\n");
                if (d2d_atu_unmap_required)
                {
                    d2d_clear_atu_map(d2d_subsystem, translated_addr_ras2); // Clear the ATU Map since error was found and we will check next D2D Subsystem
                }
                continue;
            }
            if ((record.ce_count_overflow) && (record.ce_count_valid))
            {
                uint16_t ce_counter = config_get_d2d_ecc_ce_counter() | RAS_ARM_COUNTER_SET_REQUEST;
                ras_arm_agent_set_record_counter_thresholds_by_record(&record, ce_counter, 0);
            }
            if (d2d_atu_unmap_required)
            {
                d2d_clear_atu_map(d2d_subsystem, translated_addr_ras2); // Clear the ATU Map since error was found and we are jumping out of the loop
            }
            if (acpi_severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
            {
                // If the severity is fatal, we need to bug check
                MESH_CRIT("Fatal Error in D2D Subsystem %d, Bug Check\n", d2d_subsystem);
                BUG_CHECK(((record.err_code_valid && record.err_code != 0) ? (KNG_STATUS)record.err_code : KNG_E_RAS_MESH_D2D_UE),
                          0,
                          0);
            }
            break; // Break out of the loop
        }
    d2d_ras_error_isr_exit:
        if (d2d_atu_unmap_required)
        {
            d2d_clear_atu_map(d2d_subsystem, translated_addr_ras2);
        }
    }
}

/**
 * D2D RAS Initialization
 * This function initializes the D2D RAS Agents
 * @param void
 * @return void
 **/
void d2d_ras_init(void)
{
    uint32_t translated_addr_ras2 = 0x0;
    bool d2d_atu_unmap_required = true;

    MESH_CRIT("D2D2 RAS Start\n");
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        if ((uint8_t)idhw_get_die_id() == SOC_D1)
        {
            atu_d2d2_map[d2d_subsystem].ap_base_address += 0x1000000000;
        }
        translated_addr_ras2 = d2d_set_atu_map(d2d_subsystem, &d2d_atu_unmap_required);

        // Create ARM RAS Agents
        ras_arm_agent_setup_entity(&d2dss2_agent[d2d_subsystem]);
        if (system_info_is_warm_start())
        {
            ras_arm_agent_notify_warm(&d2dss2_agent[d2d_subsystem]);
        }
        ASSERT_FAIL(ras_arm_agent_init(&d2dss2_agent[d2d_subsystem], translated_addr_ras2, "D2DSS2_AGENT") == SILIBS_SUCCESS);

        // Enable CRI signaling
        // Need to convert the knobs to RAS_ARM_COMMON types
        ASSERT_FAIL(ras_arm_agent_enable_signaling_by_type(
                        &d2dss2_agent[d2d_subsystem],
                        RAS_ARM_CNTRL_DFI | RAS_ARM_CNTRL_CI | RAS_ARM_CNTRL_DUI | RAS_ARM_CNTRL_CFI | RAS_ARM_CNTRL_UE |
                            RAS_ARM_CNTRL_FI | RAS_ARM_CNTRL_UI | RAS_ARM_CNTRL_ED) == SILIBS_SUCCESS);
        // Enable fhi ERRFHICR2
        ASSERT_FAIL(ras_arm_agent_enable_fhi(&d2dss2_agent[d2d_subsystem]) == SILIBS_SUCCESS);

        // Enable logging
        ASSERT_FAIL(ras_arm_agent_enable_logging(&d2dss2_agent[d2d_subsystem]) == SILIBS_SUCCESS);

        // Program the CE Counter Register
        if (config_get_d2d_ecc_ce_counter() != 0x0)
        {
            uint16_t cec = config_get_d2d_ecc_ce_counter() | RAS_ARM_COUNTER_SET_REQUEST;
            int status = 0x0;
            status = ras_arm_agent_set_record_counter_thresholds_by_index(&d2dss2_agent[d2d_subsystem], 0, cec, 0);
            if (status != SILIBS_SUCCESS)
            {
                MESH_CRIT("Failed to set CE Counter Thresholds for D2DSS2_AGENT%d, index 0, status: %d, ce "
                          "counter %d\n",
                          d2d_subsystem,
                          status,
                          config_get_d2d_ecc_ce_counter());
            }
            status = ras_arm_agent_set_record_counter_thresholds_by_index(&d2dss2_agent[d2d_subsystem], 1, cec, 0);
            if (status != SILIBS_SUCCESS)
            {
                MESH_CRIT("Failed to set CE Counter Thresholds for D2DSS2_AGENT%d, index 1, status: %d, ce "
                          "counter %d\n",
                          d2d_subsystem,
                          status,
                          config_get_d2d_ecc_ce_counter());
            }
        }
        if (d2d_atu_unmap_required)
        {
            d2d_clear_atu_map(d2d_subsystem, translated_addr_ras2);
        }
    }
    MESH_CRIT("D2D2 RAS End\n");
}

/**
 * D2D RAS Set the CE Counter
 * This function sets the CE Counter for the D2D RAS Agents
 * @param d2d_subsystem
 * @param cec_cli
 * @return void
 **/
void d2d_ecc_ce_counter_update(uint8_t d2d_subsystem, uint16_t cec_cli)
{
    uint32_t translated_addr_ras2 = 0x0;
    bool d2d_atu_unmap_required = true;

    MESH_CRIT("d2d_ecc_ce_counter_update Start\n");
    if (d2d_subsystem >= NUM_OF_CCG_WITH_D2D)
    {
        MESH_CRIT("Invalid D2D Subsystem 0x%x\n", d2d_subsystem);
        return;
    }
    if ((uint8_t)idhw_get_die_id() == SOC_D1)
    {
        atu_d2d2_map[d2d_subsystem].ap_base_address += 0x1000000000;
    }
    translated_addr_ras2 = d2d_set_atu_map(d2d_subsystem, &d2d_atu_unmap_required);
    ras_arm_agent_set_base(&d2dss2_agent[d2d_subsystem], translated_addr_ras2);
    // Program the CE Counter Register based on CLI input
    if (cec_cli != 0x0)
    {
        uint16_t cec = cec_cli | RAS_ARM_COUNTER_SET_REQUEST;
        int status = 0x0;
        status = ras_arm_agent_set_record_counter_thresholds_by_index(&d2dss2_agent[d2d_subsystem], 0, cec, 0);
        if (status != SILIBS_SUCCESS)
        {
            MESH_CRIT("Failed to set CE Counter Thresholds for D2DSS2_AGENT%d, index 0, status: %d, ce "
                      "counter %d\n",
                      d2d_subsystem,
                      status,
                      cec_cli);
        }
        status = ras_arm_agent_set_record_counter_thresholds_by_index(&d2dss2_agent[d2d_subsystem], 1, cec, 0);
        if (status != SILIBS_SUCCESS)
        {
            MESH_CRIT("Failed to set CE Counter Thresholds for D2DSS2_AGENT%d, index 1, status: %d, ce "
                      "counter %d\n",
                      d2d_subsystem,
                      status,
                      cec_cli);
        }
    }
    if (d2d_atu_unmap_required)
    {
        d2d_clear_atu_map(d2d_subsystem, translated_addr_ras2);
    }
    MESH_CRIT("d2d_ecc_ce_counter_update End\n");
}

/**
 * D2D RAS Error Injection
 * This function injects the D2D RAS Error
 * @param d2d_subsystem
 * @param err_inj
 * @param err_cnt_down
 * @return void
 **/
void d2d_ras_error_inj(uint8_t d2d_subsystem, uint32_t err_inj, uint32_t err_cnt_down)
{
    uint32_t translated_addr_ras2 = 0x0;
    if (d2d_subsystem >= NUM_OF_CCG_WITH_D2D)
    {
        MESH_CRIT("Invalid D2D Subsystem 0x%x\n", d2d_subsystem);
        return;
    }
    bool d2d_atu_unmap_required = true;
    translated_addr_ras2 = d2d_set_atu_map(d2d_subsystem, &d2d_atu_unmap_required);

    // Update the Base
    int status = ras_arm_agent_set_base(&d2dss2_agent[d2d_subsystem], translated_addr_ras2);
    if (status != SILIBS_SUCCESS)
    {
        MESH_CRIT("Failed to set base addr, D2DSS2_AGENT%d\n", d2d_subsystem);
        goto d2d_ras_error_inj_exit;
    }

    ras_inj_syndrome_t syn[1] = {
        {.addr = D2DSS_TEST_RAS_DUMMY_ADDRESS, .inj_counter = D2DSS_TEST_RAS_INJ_COUNTER, .valid = 1}};
    d2dss2_agent[d2d_subsystem].syn = syn;
    d2dss2_agent[d2d_subsystem].syn->inj_counter = err_cnt_down;
    BUG_ASSERT_PARAM(!ras_arm_agent_trigger_by_type(&d2dss2_agent[d2d_subsystem], (uint64_t)err_inj), err_inj, d2d_subsystem);

d2d_ras_error_inj_exit:
    if (d2d_atu_unmap_required)
    {
        d2d_clear_atu_map(d2d_subsystem, translated_addr_ras2);
    }
}

/**
 * Mesh/D2D Error Injection Callback
 * This function is called when a Mesh/D2D Error Injection Callback is triggered
 * from the Health Monitor module
 * @param einj_payload
 * @param ctx
 * @return acpi_einj_cmd_status_t
 **/
PLACED_CODE acpi_einj_cmd_status_t mesh_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);
    MESH_INFO("Mesh Error Injection Callback Start\n");
    if (einj_payload == NULL)
    {
        MESH_CRIT("Set error with address struct pointer invalid\n");
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_MESH)
    {
        MESH_ERR("Invalid Mesh error domain(%d)\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (idhw_is_single_die_boot_en() == true)
    {
        if (einj_payload->component_instance != idsw_get_die_id())
        {
            MESH_ERR("Invalid DIE_ID(%d)", einj_payload->component_instance);
            return ACPI_EINJ_INVALID_ACCESS;
        }
    }
    if (einj_payload->component_type == COMPONENT_TYPE_MESH)
    {
        if (einj_payload->component_instance <= SOC_D1)
        {
            // error_injection (0x0)
            if (einj_payload->status_operation.value == OPERATION_STATUS_ERR_INJ)
            {
                uint8_t node_type = (uint8_t)((einj_payload->param_type.error_parameters[0] >> 8) & 0xFF);
                uint8_t node_id = (uint8_t)(einj_payload->param_type.error_parameters[0] & 0xFF);
                uint16_t node_control_reg = (uint16_t)(einj_payload->param_type.error_parameters[1] & 0xFFFF);
                uint32_t err_inj = (uint32_t)(einj_payload->value_type.data_64 & 0xFFFFFFFF);
                uint8_t byte_par_err_inj = (uint32_t)((einj_payload->value_type.data_64 >> 32) & 0xFF);
                MESH_INFO("cmn800_error_injection Start\n");
                MESH_INFO("node_type: 0x%x, node_id: 0x%x, node_control_reg: 0x%x, err_inj: 0x%x, "
                          "byte_par_err_inj: 0x%x, die_num: %d\n",
                          node_type,
                          node_id,
                          node_control_reg,
                          err_inj,
                          byte_par_err_inj,
                          (uint8_t)idhw_get_die_id());
                cmn800_error_injection(node_type, node_id, node_control_reg, err_inj, byte_par_err_inj, (uint8_t)idhw_get_die_id());
                MESH_INFO("cmn800_error_injection End\n");
            }
            else if (einj_payload->status_operation.value == OPERATION_STATUS_PSEUDO_FAULT_ERR_GEN)
            {
                // pseudo_fault_error generation (0x1)
                uint8_t non_secure = (uint8_t)((einj_payload->param_type.error_parameters[0] >> 16) & 0x1);
                uint8_t node_type = (uint8_t)((einj_payload->param_type.error_parameters[0] >> 8) & 0xFF);
                uint8_t node_id = (uint8_t)(einj_payload->param_type.error_parameters[0] & 0xFF);
                uint16_t node_control_reg = (uint16_t)(einj_payload->param_type.error_parameters[1] & 0xFFFF);
                uint32_t err_inj = (uint32_t)(einj_payload->value_type.data_64 & 0xFFFFFFFF);
                uint32_t err_cnt_down = (uint32_t)((einj_payload->value_type.data_64 >> 32) & 0xFFFFFFFF);
                MESH_INFO("cmn800_pseudo_fault_error_injection Start\n");
                MESH_INFO("%s node_type: 0x%x, node_id: 0x%x, node_control_reg: 0x%x, err_inj: 0x%x, "
                          "err_cnt_down: 0x%x, die_num: %d\n",
                          (non_secure == 0x0) ? "Secure" : "Non-Secure",
                          node_type,
                          node_id,
                          node_control_reg,
                          err_inj,
                          err_cnt_down,
                          (uint8_t)idhw_get_die_id());
                cmn800_pseudo_fault_error_injection(node_type,
                                                    node_id,
                                                    node_control_reg,
                                                    err_inj,
                                                    err_cnt_down,
                                                    (uint8_t)idhw_get_die_id(),
                                                    (bool)non_secure);
                MESH_INFO("cmn800_pseudo_fault_error_injection End\n");
            }
            else
            {
                MESH_ERR("Invalid value(%d)\n", einj_payload->status_operation.value);
                return ACPI_EINJ_INVALID_ACCESS;
            }
        }
    }
    else if (einj_payload->component_type == COMPONENT_TYPE_D2D)
    {
        if (einj_payload->component_instance <= SOC_D1)
        {
            uint8_t node_id = (uint8_t)(einj_payload->param_type.error_parameters[0] & 0xFF);
            uint32_t err_inj = (uint32_t)(einj_payload->value_type.data_64 & 0xFFFFFFFF);
            uint32_t err_cnt_down = (uint32_t)((einj_payload->value_type.data_64 >> 32) & 0xFFFFFFFF);
            MESH_INFO("d2d_pseudo_error_injection Start\n");
            MESH_INFO("node_id: 0x%x, err_inj: 0x%x, "
                      "err_cnt_down: 0x%x, die_num: %d\n",
                      node_id,
                      err_inj,
                      err_cnt_down,
                      (uint8_t)idhw_get_die_id());
            d2d_ras_error_inj(node_id, err_inj, err_cnt_down);
            MESH_INFO("d2d_pseudo_error_injection End\n");
        }
    }
    else
    {
        MESH_ERR("Invalid component type(%d)\n", einj_payload->component_type);
        return ACPI_EINJ_INVALID_ACCESS;
    }
    MESH_INFO("Mesh Error Injection Callback End\n");

    return ACPI_EINJ_SUCCESS;
}
