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
#include <ddrss_config.h>
#include <ddrss_runtime_api.h>
#include <errno.h>
#include <health_monitor.h>
#include <idsw_kng.h> // for idsw_get_die_id
#include <memory.h>
#include <rhtlm_service.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdbool.h> // for bool
#include <stdint.h>  // for uint32_t, uint16_t
#include <stdio.h>   // for NULL
#include <stdlib.h>  // for strtoul
#include <string.h>  // for memset
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
typedef struct
{
    char* option;
    uint32_t mask;
} rh_filter_mask_op;

/*-------- Function Prototypes -----------*/
static bool check_mc_param(int Argc, const char** Argv, uint32_t* mc, bool just_mc);
static bool is_mc_belong_to_die(uint32_t mc);
static FPFW_CLI_STATUS display_info(int Argc, const char** Argv);
static FPFW_CLI_STATUS default_rh_cfg(int Argc, const char** Argv);
static FPFW_CLI_STATUS read_ddr_rh_config(int Argc, const char** Argv);
static FPFW_CLI_STATUS read_ddr_rh_evt(int Argc, const char** Argv);
static FPFW_CLI_STATUS set_rh_filtermask(int Argc, const char** Argv);
static FPFW_CLI_STATUS enable_all_rh_filtermask(int Argc, const char** Argv);
static FPFW_CLI_STATUS submit_hm_rh_record(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND cli_rh_tlm_commands[] = {
    {NULL_LIST_ENTRY, "rhtlm", "info", display_info, "Show service info", "Usage: info"},
    {NULL_LIST_ENTRY, "rhtlm", "defaultcfg", default_rh_cfg, "Defaults config to specific mc", "Usage: defaultcfg <mc>"},
    {NULL_LIST_ENTRY, "rhtlm", "readrhcfg", read_ddr_rh_config, "Read RowHammer cfg", "Usage: readrhcfg"},
    {NULL_LIST_ENTRY, "rhtlm", "readrhevt", read_ddr_rh_evt, "Read RowHammer telemetry", "Usage: readrhevt"},
    {NULL_LIST_ENTRY, "rhtlm", "setrhfiltermask", set_rh_filtermask, "Set RowHammer Filter Mask", "Usage: setrhfiltermask "},
    {NULL_LIST_ENTRY, "rhtlm", "enableallfilter", enable_all_rh_filtermask, "Enable all filter mask", "Usage: enableallfilter "},
    {NULL_LIST_ENTRY, "rhtlm", "rhcper", submit_hm_rh_record, "send RH Telemetry(dummy)", "Usage: rhcper "},

};

/*------------- Functions ----------------*/

/**
 * @brief  : Print Row Hammer Cfg
 * @param  : p_tm - pointer to RH Cfg
 * @retval : void
 */
static PLACED_CODE void print_rhm_cfg_telemetry(ddrss_rhm_tm_cfg_t* p_tm)
{
    if (p_tm == NULL)
    {
        return;
    }

    FpFwCliPrint("seed :0x%x\n", p_tm->rm_pn_seed);
    FpFwCliPrint("sel  :0x%x\n", p_tm->rm_tm_sel);
    FpFwCliPrint("flt  :0x%x\n", p_tm->rm_tm_filter);
    FpFwCliPrint("a_thr:0x%x\n", p_tm->rm_act_thr);
    FpFwCliPrint("s_thr:0x%x\n", p_tm->rm_sample_thr);
    FpFwCliPrint("soc  :0x%x\n", p_tm->rm_soc);
    FpFwCliPrint("smc1 :0x%x\n", p_tm->rm_smc_1);
    FpFwCliPrint("smc2 :0x%x\n", p_tm->rm_smc_2);
    FpFwCliPrint("cfg  :0x%x\n", p_tm->rm_cfg);
}

/**
 * @brief  : Print Row Hammer telemetry record
 * @param  : p_ddrss_rm_telemetry - pointer to RH Telemetry record
 * @retval : void
 */
static PLACED_CODE void print_rh_tm_evt(ddrss_rhm_tm_evt_t* p_ddrss_rm_telemetry)
{
    if (p_ddrss_rm_telemetry == NULL)
    {
        return;
    }

    FpFwCliPrint("valid  :0x%x\n", p_ddrss_rm_telemetry->valid);
    FpFwCliPrint("tmstp  :0x%llx\n", p_ddrss_rm_telemetry->timestamp);
    FpFwCliPrint("drop   :0x%x\n", p_ddrss_rm_telemetry->dropped);
    FpFwCliPrint("type   :0x%x\n", p_ddrss_rm_telemetry->type);
    FpFwCliPrint("p_err  :0x%x\n", p_ddrss_rm_telemetry->parity_err);
    FpFwCliPrint("act_r  :0x%x\n", p_ddrss_rm_telemetry->act_rank);
    FpFwCliPrint("act_bg :0x%x\n", p_ddrss_rm_telemetry->act_bg);
    FpFwCliPrint("act_bn :0x%x\n", p_ddrss_rm_telemetry->act_bank);
    FpFwCliPrint("act_r  :0x%lx\n", p_ddrss_rm_telemetry->act_row);
    FpFwCliPrint("smc    :0x%lx\n", p_ddrss_rm_telemetry->smc);
    FpFwCliPrint("soc    :0x%lx\n", p_ddrss_rm_telemetry->soc);
    FpFwCliPrint("sc  : 0x%lx\n", p_ddrss_rm_telemetry->spill_count);

    FpFwCliPrint("address :");
    for (uint8_t i = 0; i < DDRSS_RHM_TM_MAX_WAYS; i++)
    {
        FpFwCliPrint("0x%lx ", p_ddrss_rm_telemetry->way_info[i].address);
    }
    FpFwCliPrint("\n");
    FpFwCliPrint("count :");
    for (uint8_t i = 0; i < DDRSS_RHM_TM_MAX_WAYS; i++)
    {
        FpFwCliPrint("0x%lx ", p_ddrss_rm_telemetry->way_info[i].count);
    }
    FpFwCliPrint("\n");

    FpFwCliPrint("lock :");
    for (uint8_t i = 0; i < DDRSS_RHM_TM_MAX_WAYS; i++)
    {
        FpFwCliPrint("0x%lx ", p_ddrss_rm_telemetry->way_info[i].lock);
    }
    FpFwCliPrint("\n");
}

/**
 * @brief  : Process and check Memory Controller param
 * @param  : Argc - arg count
 *           Argv - arg value
 *           [out] mc - pointer where to store MC value
 *           just_mc - look just for MC argument
 * @retval : true - valid param and valid MC for die
 *           false - otherwise
 */
static PLACED_CODE bool check_mc_param(int Argc, const char** Argv, uint32_t* mc, bool just_mc)
{
    if (Argc == 1) // no argument passed
    {
        *mc = 0; // Default to MC0
    }
    else if (Argc >= 2) // only mc is passed as argument
    {
        if (Argv[1] != NULL)
        {
            *mc = (uint32_t)(strtoul(Argv[1], 0, 0));
        }
        else
        {
            return false;
        }
    }

    if (Argc > 2 && just_mc == true)
    {
        FpFwCliPrint("Invalid number of arguments");
        return false;
    }

    if (!is_mc_belong_to_die(*mc))
    {
        return false;
    }

    return true;
}

/**
 * @brief  : Check if Memory Controller is accessible from curent DIE
 * @param  : mc - memory controller value
 *
 * @retval : true - mc and die match
 *           false - otherwise
 */
PLACED_CODE bool is_mc_belong_to_die(uint32_t mc)
{
    KNG_DIE_ID die_id = idsw_get_die_id();
    if (die_id == DIE_0)
    {
        if (mc >= RHTLM_MC_DIE0_COUNT_NR)
        {
            FpFwCliPrint("MC must be between 0 and 11 for DIE 0");
            return false;
        }
    }
    else
    {
        if ((mc >= RHTLM_MC_DIE1_COUNT_NR) || (mc < RHTLM_MC_DIE0_COUNT_NR))
        {
            FpFwCliPrint("MC must be between 12 and 23 for DIE 1");
            return false;
        }
    }

    return true;
}

/**
 * @brief  : Register rhtlm CLI to FpFwCli table service
 * @param  : void
 *
 * @retval : void
 */
void cli_rhtlm_initialize(void)
{
    FpFwCliRegisterTable(&cli_rh_tlm_commands[0], FPFW_ARRAY_SIZE(cli_rh_tlm_commands));
}

static PLACED_CODE FPFW_CLI_STATUS display_info(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    rhtlm_print_service_status();

    return CLI_SUCCESS;
}

/**
 * @brief  : Set to a default cfg MC
 * @param  : Argc - arg count
 *           Argv - arg value
 * @retval : CLI_ERROR - something went wrong
 *           CLI_SUCCESS - Success
 */
static PLACED_CODE FPFW_CLI_STATUS default_rh_cfg(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_mc_param(Argc, Argv, &mc, false))
    {
        return CLI_ERROR;
    }

    ddrss_rhm_cfg_t ddrss_rhm_cfg = {.rm_erhm_en = 1,
                                     .rm_drt_clr = 0b0100,
                                     .rm_cnt_clr = 0x4000,
                                     .rm_soc = 0x20,
                                     .rm_smc_1 = 0x2237,
                                     .rm_smc_2 = 0b1111,
                                     .rm_act_thr = 17,
                                     .rm_sample_thr = 16,
                                     .tm_sel_always = 0b11001,
                                     .tm_sel_filtered = 0b11001,
                                     .tm_filter = {.sub_bank_mask = 0xFFFF,
                                                   .bank_mask = 0xF,
                                                   .bg_mask = 0xF, // suggested 0xFF
                                                   .rank_mask = 0b11}};

    FpFwCliPrint("Def RH cfg!\n");
    int result;
    result = ddrss_update_rhm_config(mc, &ddrss_rhm_cfg);
    FpFwCliPrint("res %d ,exp %d \n", result, SILIBS_SUCCESS);

    return CLI_SUCCESS;
}

/**
 * @brief  : Read generated telemetry from MC , if available
 * @param  : Argc - arg count
 *           Argv - arg value
 * @retval : CLI_ERROR - something went wrong
 *           CLI_SUCCESS - Success
 */
static PLACED_CODE FPFW_CLI_STATUS read_ddr_rh_evt(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    int result = SILIBS_SUCCESS;

    if (!check_mc_param(Argc, Argv, &mc, true))
    {
        return CLI_ERROR;
    }

    FpFwCliPrint("Read RH Tlm !\n");

    ddrss_rhm_tm_evt_t ddrss_rm_telemetry;

    result = ddrss_get_telemetry_record(mc, DDRSS_TELEMETRY_TYPE_RHM_EVT, &ddrss_rm_telemetry, sizeof(ddrss_rm_telemetry));
    if (result != SILIBS_SUCCESS)
    {
        FpFwCliPrint("res %d\n", result);
        if (result == SILIBS_E_DATA)
        {
            FpFwCliPrint("No avb tlm at %d \n", mc);
        }
    }
    else
    {
        print_rh_tm_evt(&ddrss_rm_telemetry);
    }

    return CLI_SUCCESS;
}

/**
 * @brief  : Read configuration telemetry record from MC , if available
 * @param  : Argc - arg count
 *           Argv - arg value
 * @retval : CLI_ERROR - something went wrong
 *           CLI_SUCCESS - Success
 */
static PLACED_CODE FPFW_CLI_STATUS read_ddr_rh_config(int Argc, const char** Argv)
{
    uint32_t mc = 0;

    if (!check_mc_param(Argc, Argv, &mc, true))
    {
        return CLI_ERROR;
    }

    FpFwCliPrint("Read Rh cfg!\n");
    ddrss_rhm_tm_cfg_t ddr_current_cfg;
    int result;
    result = ddrss_get_telemetry_record(mc, DDRSS_TELEMETRY_TYPE_RHM_CFG, &ddr_current_cfg, sizeof(ddr_current_cfg));
    FpFwCliPrint("res %d , exp %d \n", result, SILIBS_SUCCESS);
    if (result == SILIBS_SUCCESS)
    {
        print_rhm_cfg_telemetry(&ddr_current_cfg);
    }

    return CLI_SUCCESS;
}

/**
 * @brief  : Set RH Filter mask (the mask is taken in account for all MC RH Tlm)
 * @param  : Argc - arg count
 *           Argv - arg value
 * @retval : CLI_ERROR - something went wrong
 *           CLI_SUCCESS - Success
 */
static PLACED_CODE FPFW_CLI_STATUS set_rh_filtermask(int Argc, const char** Argv)
{
    ddrss_rhm_tm_rpt_mask_t tm_rec_msk = {0};

    rh_filter_mask_op options_map[] = {{"-lr", 1U << 0},
                                       {"-up", 1U << 1},
                                       {"-bank", 1U << 2},
                                       {"-bg", 1U << 3},
                                       {"-rank", 1U << 4},
                                       {"-smc", 1U << 5},
                                       {"-soc", 1U << 6},
                                       {"-spc", 1U << 7},
                                       {"-waddr", 1U << 8},
                                       {"-wic", 1U << 9},
                                       {"-wil", 1U << 10},
                                       {"-tpar", 1U << 24},
                                       {"-tdrfm", 1U << 25},
                                       {"-drfmsam", 1U << 26},
                                       {"-tes", 1U << 27},
                                       {"-teh", 1U << 28}};

    int map_size = sizeof(options_map) / sizeof(options_map[0]);

    for (int i = 1; i < Argc; ++i)
    {
        bool found = false;
        for (int j = 0; j < map_size; ++j)
        {
            if (strcmp(Argv[i], options_map[j].option) == 0)
            {
                tm_rec_msk.as_uint32 |= options_map[j].mask;
                found = true;
                break;
            }
        }
        if (!found)
        {
            FpFwCliPrint("Unk opt %s\n", Argv[i]);
        }
    }

    FpFwCliPrint("Set Rh flt mask!\n");
    int result;
    result = ddrss_set_rhm_telemetry_record_mask(&tm_rec_msk);
    FpFwCliPrint("res %d,exp %d \n", result, SILIBS_SUCCESS);

    return CLI_SUCCESS;
}

/**
 * @brief  : Enable all Filter mask (the mask is taken in account for all MC RH Tlm)
 * @param  : Argc - arg count
 *           Argv - arg value
 * @retval : CLI_ERROR - something went wrong
 *           CLI_SUCCESS - Success
 */
static PLACED_CODE FPFW_CLI_STATUS enable_all_rh_filtermask(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    ddrss_rhm_tm_rpt_mask_t tm_rec_msk = {0};
    tm_rec_msk.as_uint32 = 0xFFFFFFFF;

    FpFwCliPrint("all flt \n");
    int result;
    result = ddrss_set_rhm_telemetry_record_mask(&tm_rec_msk);
    FpFwCliPrint("res %d , exp %d \n", result, SILIBS_SUCCESS);

    return CLI_SUCCESS;
}

/**
 * @brief  : Submit via HM a RH record (dummy)
 * @param  : Argc - arg count
 *           Argv - arg value
 * @retval : CLI_ERROR - something went wrong
 *           CLI_SUCCESS - Success
 */
static PLACED_CODE FPFW_CLI_STATUS submit_hm_rh_record(int Argc, const char** Argv)
{

    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    acpi_cper_section_t cper_section = {0};
    acpi_err_sec_ddrss_rhm_tm_t rh_cper_section = {0};

    memset(&cper_section, 0x0, sizeof(cper_section));
    memset(&rh_cper_section, 0x0, sizeof(rh_cper_section));

    rh_cper_section.mc = 0;
    rh_cper_section.valid = 1;
    rh_cper_section.tlm_sample_type = RHTLM_SAMPLE_CLI;
    cper_section.sec_rh_tlm = rh_cper_section;
    // prod_ddrss_convert_rh_rec_to_rh_cper();

    FpFwCliPrint("Sebd RH CPER to HM !\n");

    hm_submit_cper(ACPI_ERROR_DOMAIN_RHTLM, ACPI_ERROR_SEVERITY_INFORMATIONAL, &cper_section, sizeof(cper_section));

    return CLI_SUCCESS;
}
