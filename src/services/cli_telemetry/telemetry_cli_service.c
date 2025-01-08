//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_cli_service.c
 * Implementation of the telemetry cli service.
 */

/*------------- Includes -----------------*/

#include "FpFwLinkedList.h" // for NULL_LIST_ENTRY
#include "fpfw_status.h"    // for FPFW_STATUS_SUCCEEDED, fpf...

#include <FpFwCli.h>   // for FpFwCliPrint, FPFW_CLI_COM...
#include <FpFwUtils.h> // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <data_proc_tlm_cmpnt.h>
#include <exec_tlm_cmpnt.h>
#include <memory.h>
#include <power_tlm_fuse.h>
#include <stdbool.h> // for bool
#include <stdint.h>  // for uint32_t, uint16_t
#include <stdio.h>   // for NULL
#include <stdlib.h>  // for strtoul

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static FPFW_CLI_STATUS show_info(int Argc, const char** Argv);
static FPFW_CLI_STATUS clear_data(int Argc, const char** Argv);
static FPFW_CLI_STATUS read_power_fuse_tile(int Argc, const char** Argv);
static FPFW_CLI_STATUS read_power_fuse_soctop(int Argc, const char** Argv);
static FPFW_CLI_STATUS disable_collection(int Argc, const char** Argv);
/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND cli_pwr_tlm_commands[] = {
    {NULL_LIST_ENTRY, "pwrtlm", "info", show_info, "Show service info", "Usage: info"},
    {NULL_LIST_ENTRY, "pwrtlm", "clear", clear_data, "Clear collected data", "Usage: clear"},
    {NULL_LIST_ENTRY, "pwrtlm", "readfusetile", read_power_fuse_tile, "Read power telemetery fuse tile", "Usage: readfusetile"},
    {NULL_LIST_ENTRY, "pwrtlm", "readfusesoctop", read_power_fuse_soctop, "Read power telemetery fuse soctop", "Usage: readfusesoctop"},
    {NULL_LIST_ENTRY, "pwrtlm", "disable", disable_collection, "Disable data Collection", "Usage: disable"}};

/*------------- Functions ----------------*/

void telemetry_cli_svc_initialize(void)
{
    FpFwCliRegisterTable(&cli_pwr_tlm_commands[0], FPFW_ARRAY_SIZE(cli_pwr_tlm_commands));
}

static FPFW_CLI_STATUS show_info(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    telemetry_executive_status_t status;
    exec_tlm_cmpnt_get_status(&status);

    FpFwCliPrint("\nPower Telemetry Executive Status:\n");
    FpFwCliPrint("  Operating Mode: %s\n",
                 status.op_mode == TLM_OP_MODE_NOMINAL    ? "Nominal"
                 : status.op_mode == TLM_OP_MODE_DISABLED ? "Disabled"
                                                          : "Sensor FIFO Raw Data");
    FpFwCliPrint("  Power Package Period: %d mS\n", status.pwr_pkg_period_ms);
    FpFwCliPrint("  Instantaneous Sample Period: %d mS\n", status.inst_pkg_sample_period_ms);
    FpFwCliPrint("  Power Aggregation Period: %d mS\n", status.pwr_aggr_period_ms);
    FpFwCliPrint("  Power Package Timer Active: %s\n", status.pwr_pkg_timer_active ? "Active" : "Inactive");
    FpFwCliPrint("  Instantaneous Sample Timer Active: %s\n", status.inst_sample_timer_active ? "Active" : "Inactive");
    FpFwCliPrint("  Power Aggregation Timer Active: %s\n", status.pwr_aggr_timer_active ? "Active" : "Inactive");
    FpFwCliPrint("  24 Hour Package Timer Active: %s\n", status.twenty_four_hr_pkg_timer_active ? "Active" : "Inactive");

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS clear_data(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    data_proc_tlm_cmpnt_clear_pwr_tlm_data();

    FpFwCliPrint("\nAll Telemetry Data Cleared!\n");
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS read_power_fuse_tile(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    dts_tlm_coeff_t tileDtsCoefficientsTest[NUMBER_OF_TILES_PER_DIE] = {0};
    memset(tileDtsCoefficientsTest, 0, sizeof(tileDtsCoefficientsTest));
    // Init power fuse values
    if (platform_power_fuses_get_dts_coeff_tile(tileDtsCoefficientsTest,
                                                sizeof(tileDtsCoefficientsTest) / sizeof(tileDtsCoefficientsTest[0])))
    {
        FpFwCliPrint("\nRead Power fuse(DTS coefficient) tile: Found some unexpected values!\n");
    }

    for (int i = 0; i < NUMBER_OF_TILES_PER_DIE; i++)
    {
        FpFwCliPrint("\n DTS coefficient :tileDtsCoefficientsTest[%d].k_val = %d\n"
                     "tileDtsCoefficientsTest[%d].y_val = %d\n",
                     i,
                     tileDtsCoefficientsTest[i].k_val,
                     i,
                     tileDtsCoefficientsTest[i].y_val);
    }
    FpFwCliPrint("\nRead Power fuse(DTS coefficient) tile complete!\n");
    return CLI_SUCCESS;
}
// platform_power_fuses_get_dts_coeff_soctop
static FPFW_CLI_STATUS read_power_fuse_soctop(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    dts_tlm_coeff_t soctopDtsCoefficientsTest[NUMBER_OF_TILES_PER_DIE] = {0};
    memset(soctopDtsCoefficientsTest, 0, sizeof(soctopDtsCoefficientsTest));
    // Init power fuse values
    if (platform_power_fuses_get_dts_coeff_soctop(soctopDtsCoefficientsTest,
                                                  sizeof(soctopDtsCoefficientsTest) / sizeof(soctopDtsCoefficientsTest[0])))
    {
        FpFwCliPrint("\nRead Power fuse(DTS coefficient) soctop: Found some unexpected values!\n");
    }

    for (int i = 0; i < NUMBER_OF_TILES_PER_DIE; i++)
    {
        FpFwCliPrint("\n DTS coefficient :soctopDtsCoefficientsTest[%d].k_val = %d\n"
                     "soctopDtsCoefficientsTest[%d].y_val = %d\n",
                     i,
                     soctopDtsCoefficientsTest[i].k_val,
                     i,
                     soctopDtsCoefficientsTest[i].y_val);
    }
    FpFwCliPrint("\nRead Power fuse(DTS coefficient) soctop complete!\n");
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS disable_collection(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    exec_tlm_cmpnt_disable_data_collection();

    FpFwCliPrint("\nPower Telemetry Disabled!\n");
    return CLI_SUCCESS;
}
