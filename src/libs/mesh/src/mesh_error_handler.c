//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh_error_handler.c
 *  This file holds the Mesh Error Handler ISR functions
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>
#include <cmn800.h>
#include <cmn800_error_handler.h> // for acpi_err_sec_generic_t
#include <cmn800_sequence.h>      // for cmn800_sequence, d2dss_sequence, cmn8...
#include <cmn_config.h>           // for CMN800_CONFIG_CONFIG
#include <fpfw_icc_base.h>        // for fpfw_icc_base_ctx_t
#include <idhw.h>                 // for idhw_is_single_die_boot_en
#include <idsw.h>                 // for idsw_get_platform_sdv,
#include <idsw_kng.h>             // for PLATFORM_FPGA_LARGE
#include <kng_soc_constants.h>    // for NUM_DIE
#include <mesh.h>                 // for mesh_init
#include <mesh_error_handler.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <stdio.h>   // for MESH_INFO
#include <system_info.h>

/*------------- Defines -----------------*/

bool g_mesh_error_print = false;
// Create a FW CPER
static acpi_err_sec_generic_t mesh_cper = {0x0};

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
        MESH_INFO("Error Record Feature 0x%x_%x\n", (uint32_t)(mesh_cper->err_fr >> 32), (uint32_t)mesh_cper->err_fr);
        MESH_INFO("Error Record Control 0x%x_%x\n", (uint32_t)(mesh_cper->err_ctrl >> 32), (uint32_t)mesh_cper->err_ctrl);
        MESH_INFO("Error Record Status 0x%x_%x\n",
                  (uint32_t)(mesh_cper->err_status >> 32),
                  (uint32_t)mesh_cper->err_status);
        MESH_INFO("Error Record Address 0x%x_%x\n", (uint32_t)(mesh_cper->err_addr >> 32), (uint32_t)mesh_cper->err_addr);
        MESH_INFO("Error Record Misc 0x%x_%x\n", (uint32_t)(mesh_cper->err_misc0 >> 32), (uint32_t)mesh_cper->err_misc0);
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
