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
#include <errno.h>
#include <exec_tlm_cmpnt.h>
#include <in_band_tlm_cmpnt.h>
#include <memory.h>
#include <power_tlm_fuse.h>
#include <stdbool.h> // for bool
#include <stdint.h>  // for uint32_t, uint16_t
#include <stdio.h>   // for NULL
#include <stdlib.h>  // for strtoul
#include <string.h>  // for memset

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static FPFW_CLI_STATUS show_info(int Argc, const char** Argv);
static FPFW_CLI_STATUS clear_data(int Argc, const char** Argv);
static FPFW_CLI_STATUS read_power_fuse_tile(int Argc, const char** Argv);
static FPFW_CLI_STATUS read_power_fuse_soctop(int Argc, const char** Argv);
static FPFW_CLI_STATUS disable_collection(int Argc, const char** Argv);
static FPFW_CLI_STATUS change_mode(int Argc, const char** Argv);
static FPFW_CLI_STATUS change_timer_periods(int Argc, const char** Argv);
static FPFW_CLI_STATUS oob_log(int Argc, const char** Argv);

static bool parse_arg(const char* arg, uint32_t* out);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND cli_pwr_tlm_commands[] = {
    {NULL_LIST_ENTRY, "pwrtlm", "info", show_info, "Show service info", "Usage: info"},
    {NULL_LIST_ENTRY, "pwrtlm", "clear", clear_data, "Clear collected data", "Usage: clear"},
    {NULL_LIST_ENTRY, "pwrtlm", "readfusetile", read_power_fuse_tile, "Read power telemetery fuse tile", "Usage: readfusetile"},
    {NULL_LIST_ENTRY, "pwrtlm", "readfusesoctop", read_power_fuse_soctop, "Read power telemetery fuse soctop", "Usage: readfusesoctop"},
    {NULL_LIST_ENTRY, "pwrtlm", "disable", disable_collection, "Disable data Collection", "Usage: disable"},
    {NULL_LIST_ENTRY, "pwrtlm", "mode", change_mode, "Change Mode", "Usage: mode <disabled | publish | collect | snsr_fifo_dbg> "},
    {NULL_LIST_ENTRY, "pwrtlm", "timers", change_timer_periods, "Change timer periods in mS", "Usage: timers (mS) <aggr_tmr> <inst_tmr> <pwr_tmr> <24hr_tmr> "},
    {NULL_LIST_ENTRY, "pwrtlm", "ooblog", oob_log, "Log Out of Band Sensors", "Usage: ooblog <enable | disable> "},
};

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
                 status.op_mode == TLM_OP_MODE_PUBLISHING        ? "Publishing"
                 : status.op_mode == TLM_OP_MODE_COLLECTING_DATA ? "Collecting"
                 : status.op_mode == TLM_OP_MODE_DISABLED        ? "Disabled"
                                                                 : "Sensor FIFO Raw Data");

    FpFwCliPrint("  Data Aggregation Period: %d mS\n", status.data_aggr_period_ms);
    FpFwCliPrint("  Instantaneous Sample Period: %d mS\n", status.inst_pkg_sample_period_ms);
    FpFwCliPrint("  Power Package Period: %d mS\n", status.pwr_pkg_period_ms);
    FpFwCliPrint("  24 Hour Package Period: %d mS\n", status.twenty_four_hr_pkg_period_ms);
    FpFwCliPrint("  Data Aggregation Timer Active: %s\n", status.data_aggr_timer_active ? "Active" : "Inactive");
    FpFwCliPrint("  Instantaneous Sample Timer Active: %s\n", status.inst_sample_timer_active ? "Active" : "Inactive");
    FpFwCliPrint("  Power Package Timer Active: %s\n", status.pwr_pkg_timer_active ? "Active" : "Inactive");
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
        FpFwCliPrint("DTS coefficient:\n\ttileDtsCoefficientsTest[%d].k_val = %d\n"
                     "\ttileDtsCoefficientsTest[%d].y_val = %d\n",
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
        FpFwCliPrint("DTS coefficient:\n\tsoctopDtsCoefficientsTest[%d].k_val = %d\n"
                     "\tsoctopDtsCoefficientsTest[%d].y_val = %d\n",
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

    exec_tlm_cmpnt_set_mode_change_pending(TLM_OP_MODE_DISABLED);

    FpFwCliPrint("\nPower Telemetry Disabled!\n");
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS change_mode(int Argc, const char** Argv)
{
    if (Argc == 2)
    {
        tlm_operating_mode_t new_mode = TLM_OP_MODE_DISABLED;

        if (strcmp(Argv[1], "disabled") == 0)
        {
            new_mode = TLM_OP_MODE_DISABLED;
        }
        else if (strcmp(Argv[1], "publish") == 0)
        {
            new_mode = TLM_OP_MODE_PUBLISHING;
        }
        else if (strcmp(Argv[1], "collect") == 0)
        {
            new_mode = TLM_OP_MODE_COLLECTING_DATA;
        }
        else if (strcmp(Argv[1], "snsr_fifo_dbg") == 0)
        {
            new_mode = TLM_OP_MODE_SENSOR_FIFO_RAW_DATA;
        }
        else
        {
            FpFwCliPrint("\nERROR:: %s\n", cli_pwr_tlm_commands[5].Usage);
            return CLI_ERROR;
        }

        exec_tlm_cmpnt_set_mode_change_pending(new_mode);

        return CLI_SUCCESS;
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_pwr_tlm_commands[5].Usage);
    return CLI_ERROR;
}

static FPFW_CLI_STATUS change_timer_periods(int Argc, const char** Argv)
{
    if (Argc == 5)
    {
        uint32_t aggr_tmr_ms = 0;
        uint32_t inst_tmr_ms = 0;
        uint32_t pwr_tmr_ms = 0;
        uint32_t twenty_four_hr_tmr_ms = 0;

        if (parse_arg(Argv[1], &aggr_tmr_ms) && parse_arg(Argv[2], &inst_tmr_ms) &&
            parse_arg(Argv[3], &pwr_tmr_ms) && parse_arg(Argv[4], &twenty_four_hr_tmr_ms))
        {
            exec_tlm_cmpnt_udpdate_timer_periods(aggr_tmr_ms, inst_tmr_ms, pwr_tmr_ms, twenty_four_hr_tmr_ms);
        }
        else
        {
            FpFwCliPrint("\nERROR:: %s\n", cli_pwr_tlm_commands[6].Usage);
            return CLI_ERROR;
        }
        return CLI_SUCCESS;
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_pwr_tlm_commands[6].Usage);
    return CLI_ERROR;
}

static FPFW_CLI_STATUS oob_log(int Argc, const char** Argv)
{
    if (Argc == 2)
    {
        bool oob_logging = false;

        if (strcmp(Argv[1], "enable") == 0)
        {
            oob_logging = true;
        }
        else if (strcmp(Argv[1], "disable") == 0)
        {
            oob_logging = false;
        }
        else
        {
            FpFwCliPrint("\nERROR:: %s\n", cli_pwr_tlm_commands[7].Usage);
            return CLI_ERROR;
        }

        exec_tlm_cmpnt_set_oob_log_enable(oob_logging);

        return CLI_SUCCESS;
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_pwr_tlm_commands[7].Usage);
    return CLI_ERROR;
}

bool parse_arg(const char* arg, uint32_t* out)
{
    char* end;
    errno = 0;

    uint32_t val = strtoul(arg, &end, 0);

    if (arg == end || *end != '\0' || errno == ERANGE)
    {
        return false;
    }

    *out = val;
    return true;
}