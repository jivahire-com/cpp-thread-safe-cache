//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh_error_handler.c
 *  This file holds the Mesh Error Handler ISR functions
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
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
#include <vab_regs.h>
/*------------- Defines -----------------*/

ras_agent_entity_t d2dss2_agent[NUM_OF_CCG_WITH_D2D] = {0};
bool g_mesh_error_print = false;
// Create a FW CPER
static acpi_err_sec_generic_t mesh_cper = {0x0};

atu_map_entry_t atu_d2d2_map[NUM_OF_CCG_WITH_D2D] = {
    {.ap_base_address = AP_TOP_D0_D2DSS_0_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ALIGN_UP(D2DSS_CXS_D2D_RAS_SIZE, _8KB) - 1,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_1_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ALIGN_UP(D2DSS_CXS_D2D_RAS_SIZE, _8KB) - 1,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_2_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ALIGN_UP(D2DSS_CXS_D2D_RAS_SIZE, _8KB) - 1,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_3_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ALIGN_UP(D2DSS_CXS_D2D_RAS_SIZE, _8KB) - 1,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_4_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ALIGN_UP(D2DSS_CXS_D2D_RAS_SIZE, _8KB) - 1,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_5_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ALIGN_UP(D2DSS_CXS_D2D_RAS_SIZE, _8KB) - 1,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_6_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ALIGN_UP(D2DSS_CXS_D2D_RAS_SIZE, _8KB) - 1,
     .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT}},
    {.ap_base_address = AP_TOP_D0_D2DSS_7_ADDRESS + D2DSS_CXS_D2D_RAS_ADDRESS,
     .mscp_start_address = 0,
     .mscp_end_address = ALIGN_UP(D2DSS_CXS_D2D_RAS_SIZE, _8KB) - 1,
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
    default:
        MESH_INFO("NODE_ID_UNKNOWN\n");
        break;
    }
    switch (mesh_cper->node_id.vendor_specific_data[MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_1])
    {
    case MESH_ERROR_RAS_ERROR:
        MESH_INFO("RAS_ERROR\n");
        break;
    case MESH_ERROR_RAS_FAULT:
        MESH_INFO("RAS_FAULT\n");
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
        MESH_INFO("Error Record Misc 0x%08x_%08x\n",
                  (uint32_t)(mesh_cper->err_misc0 >> 32),
                  (uint32_t)mesh_cper->err_misc0);
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
}

uint32_t d2d_set_atu_map(uint8_t d2d_subsystem)
{
    uint32_t translated_addr_ras2 = 0x0;
    MESH_CRIT("D2D2:: Setting ATU Map for D2D Subsystem %d\n", d2d_subsystem);

    FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_d2d2_map[d2d_subsystem]));

    FPFW_RUNTIME_ASSERT(!atu_translate_address(ATU_ID_MSCP, atu_d2d2_map[d2d_subsystem].ap_base_address, &translated_addr_ras2));

    MESH_CRIT("D2D2:: d2d2_ras_address 0x%08x_%08x, translated_addr_ras2 = 0x%x \n",
              (uint32_t)(atu_d2d2_map[d2d_subsystem].ap_base_address >> 32),
              (uint32_t)(atu_d2d2_map[d2d_subsystem].ap_base_address & 0xFFFFFFFF),
              translated_addr_ras2);

    return translated_addr_ras2;
}

void d2d_clear_atu_map(uint8_t d2d_subsystem)
{
    MESH_CRIT("D2D2:: Clearing ATU Map for D2D Subsystem %d\n", d2d_subsystem);
    FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_d2d2_map[d2d_subsystem]));

    // Clear the MSCP base address after unmap
    atu_d2d2_map[d2d_subsystem].mscp_start_address = 0;
    atu_d2d2_map[d2d_subsystem].mscp_end_address = ALIGN_UP(D2DSS_CXS_D2D_RAS_SIZE, _8KB) - 1;
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

    ras_error_record_t record;
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {

        translated_addr_ras2 = d2d_set_atu_map(d2d_subsystem);

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
            if (record.handler)
            {
                if (record.handler(&record))
                {
                    MESH_CRIT("Error encountered while handling record\n");
                }
            }
            else
            {
                MESH_CRIT("Invalid Record, No Further Action\n");
                d2d_clear_atu_map(d2d_subsystem); // Clear the ATU Map since error was found and we will check next D2D Subsystem
                continue;
            }

            // ADO 1513835 The combined INT is cleared outside this module

            d2d_clear_atu_map(d2d_subsystem); // Clear the ATU Map since error was found and we are jumping out of the loop
            BUG_CHECK(record.err_code_valid ? (KNG_STATUS)record.err_code : KNG_E_FAIL, 0, 0);
            break; // Break out of the loop
        }
    d2d_ras_error_isr_exit:
        d2d_clear_atu_map(d2d_subsystem);
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

    MESH_CRIT("D2D2 RAS Start\n");
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        if ((uint8_t)idhw_get_die_id() == SOC_D1)
        {
            atu_d2d2_map[d2d_subsystem].ap_base_address += 0x1000000000;
        }
        translated_addr_ras2 = d2d_set_atu_map(d2d_subsystem);

        // Create ARM RAS Agents
        ras_arm_agent_setup_entity(&d2dss2_agent[d2d_subsystem]);
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
        uint16_t temp16 = config_get_d2d_ecc_ce_counter();
        if (temp16 != 0)
        {
            uint32_t data32 = D2DSS_CE_COUNTER_MAX - temp16;
            MESH_CRIT("D2D:: CE Counter To write 0x%08x\n", data32);
            // Read
            MESH_CRIT("D2D:: CE Counter Reg Read Addr 0x%08x, Value 0x%08x\n",
                      (uintptr_t)(translated_addr_ras2 + D2DSS_ERR0MISC0_HI),
                      MMIO_READ32((uintptr_t)(translated_addr_ras2 + D2DSS_ERR0MISC0_HI)));
            // Write
            MMIO_WRITE32((uintptr_t)(translated_addr_ras2 + D2DSS_ERR0MISC0_HI), data32);
            // Readback
            MESH_CRIT("D2D:: CE Counter Reg Readback Addr 0x%08x, Value 0x%08x\n",
                      (uintptr_t)(translated_addr_ras2 + D2DSS_ERR0MISC0_HI),
                      MMIO_READ32((uintptr_t)(translated_addr_ras2 + D2DSS_ERR0MISC0_HI)));
        }

        d2d_clear_atu_map(d2d_subsystem);
    }
    MESH_CRIT("D2D2 RAS End\n");
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
    translated_addr_ras2 = d2d_set_atu_map(d2d_subsystem);

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
    FPFW_RUNTIME_ASSERT(!ras_arm_agent_trigger_by_type(&d2dss2_agent[d2d_subsystem], (uint64_t)err_inj));

d2d_ras_error_inj_exit:
    d2d_clear_atu_map(d2d_subsystem);
}
