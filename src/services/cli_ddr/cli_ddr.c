//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_ddr.c
 * Source file to implement ddr cli
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>        // for FPFW_CLI_STATUS, CLI_SUCCESS, FpFwCliReg...
#include <FpFwLinkedList.h> // for NULL_LIST_ENTRY
#include <FpFwUtils.h>      // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <ddr_err_inj.h>
#include <ddr_manager_i3c.h>
#include <ddr_memory_map.h>
#include <ddrss_lib.h>
#include <idsw_kng.h> // for idsw_get_die_id
#include <stdio.h>    // for printf
#include <stdlib.h>

#ifdef UNIT_TEST
    #define STATIC
#else
    #define STATIC static
#endif

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static bool check_params(int Argc, const char** Argv, uint32_t* mc);
static bool is_mc_are_belong_to_die(uint32_t mc);

/* Added 3 placeholder commands to set the structure - Functions to be plugged when Silibs apis are available */
STATIC FPFW_CLI_STATUS ecc_ce_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS ecc_ue_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS cmd_addr_parity_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS wrrtydat_ue_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS media_patrol_scrub_ce_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS fedb_merge_data_ce_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS media_patrol_scrub_ue_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS mainline_traffic_ce_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS mainline_traffic_ue_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS mrdp_parity_ue_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS ca_parity_persistent_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS ca_parity_transient_error_injection(int Argc, const char** Argv);

// Row Hammer
STATIC FPFW_CLI_STATUS rh_counters_sram_parity_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS rh_drfm_sram_parity_error_injection(int Argc, const char** Argv);

STATIC FPFW_CLI_STATUS read_dimm_pmic_power(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS read_dimm_temp_sensor(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/

// TODO Silibs to provide APIs with arguments list
// https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
STATIC FPFW_CLI_COMMAND cli_ddr_commands[] = {
    {NULL_LIST_ENTRY, "ddr_err_inj", "ecc_ce_err", ecc_ce_error_injection, "ECC CE error injection", "Usage: ecc_ce_err <parameter tbd>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "ecc_ue_err", ecc_ue_error_injection, "ECC UE error injection", "Usage: ecc_ue_err <parameter tbd>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "capar_err", cmd_addr_parity_error_injection, "CAPAR error", "Usage: caddr_parity_err <optional mc> <optional injection cmd>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "wrrtydat_ue_err", wrrtydat_ue_error_injection, "WRRTYDAT UE error injection", "Usage: wrrtydat_ue_err <parameter tbd>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "media_patrol_scrub_ce", media_patrol_scrub_ce_error_injection, "Media Patrol Scrub CE error injection", "Usage: media_patrol_scrub_ce <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "merge_data_ce", fedb_merge_data_ce_error_injection, "FEDB Merge Data CE error injection (1828223)", "Usage: merge_data_ce <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "merge_data_ue", media_patrol_scrub_ue_error_injection, "FEDB Merge Data UE error injection (1831500)", "Usage: merge_data_ue <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "mainline_traffic_ce", mainline_traffic_ce_error_injection, "Mainline Traffic CE error injection (1877177)", "Usage: mainline_traffic_ce <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "mainline_traffic_ue", mainline_traffic_ue_error_injection, "Mainline Traffic UE error injection (2031570)", "Usage: mainline_traffic_ue <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "mrdp_parity_ue", mrdp_parity_ue_error_injection, "MRDP Parity UE error injection (1877181)", "Usage: mrdp_parity_ue <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "ca_parity_persistent", ca_parity_persistent_error_injection, "Command Address Parity - Persistent einj (2031571)", "Usage: ca_parity_persistent <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "ca_parity_transient", ca_parity_transient_error_injection, "Command Address Parity - Transient einj (2093837)", "Usage: ca_parity_transient <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "rh_counters", rh_counters_sram_parity_error_injection, "Row hammer counters sram parity einj (TBD)", "Usage: rh_counters <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "rh_drfm", rh_drfm_sram_parity_error_injection, "Command Address Parity - Transient einj (TBD)", "Usage: rh_drfm <mc>"},
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    {NULL_LIST_ENTRY, "ddr_i3c", "read_dimm_pmic_power", read_dimm_pmic_power, "Read PMIC power", "Usage: read_dimm_pmic_power <dimm_idx>"},
    {NULL_LIST_ENTRY, "ddr_i3c", "read_dimm_temp_sensor", read_dimm_temp_sensor, "Read DIMM temperature sensor", "Usage: read_dimm_temp_sensor <dimm_idx> <channel_idx>"},
};

// ecc_ce_err (mc) (phy_add) {error bit}
STATIC FPFW_CLI_STATUS ecc_ce_error_injection(int Argc, const char** Argv)
{
    KNG_DIE_ID die_id = idsw_get_die_id();
    uint32_t mc = 0;
    uint64_t p_addr = 0x20080000000ULL;
    uint8_t bit = 0;
    uint16_t BIT_Value;
    bool check_null = true;

    if (Argc > 4)
    {
        FpFwCliPrint("Invalid Argument");
        return CLI_ERROR;
    }

    if (Argc > 1) // only mc is passed as argument
    {
        mc = (uint32_t)(strtoul(Argv[1], 0, 0));
    }

    if (Argc > 2) // only mc & phy_addr is passed as argument
    {
        p_addr = (uint64_t)(strtoull(Argv[2], 0, 0));
    }

    if (Argc > 3) // Mc, phy_addr & one bit is passed as argument.
    {
        bit = (uint8_t)(strtoul(Argv[3], 0, 0));
    }

    if (bit >= 10)
    {
        check_null = false;
        BIT_Value = 1;
    }
    else
    {
        BIT_Value = (1 << bit);
    }

    if (!ddr_ecc_err_inj_validation(mc, BIT_Value))
    {
        check_null = false;
    }

    if (check_null == false)
    {
        FpFwCliPrint("Invalid Argument");
        return CLI_ERROR;
    }

    ddr_ecc_error_injection(die_id, mc, p_addr, BIT_Value);
    FpFwCliPrint("DDR: ecc_ce_error_injection - Done!!\n");

    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS ecc_ue_error_injection(int Argc, const char** Argv)
{
    KNG_DIE_ID die_id = idsw_get_die_id();
    uint32_t mc = 0;
    uint64_t p_addr = 0;
    uint16_t BIT_Value = 0;
    bool check_null = false;

    if (Argc == 1) // no argument passed
    {
        mc = 0;
        p_addr = 0;
        BIT_Value = BIT0 | BIT1;
        check_null |= true;
    }
    else if (Argc == 2) // only mc is passed as argument
    {
        if (Argv[1] != NULL)
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
            check_null |= true;
        }
        p_addr = 0;
        BIT_Value = BIT0 | BIT1;
    }
    else if (Argc == 3) // only mc & phy_addr is passed as argument
    {
        if ((Argv[1] != NULL) && (Argv[2] != NULL))
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
            p_addr = (uint64_t)(strtoul(Argv[2], 0, 0));
            check_null |= true;
        }
        BIT_Value = BIT0 | BIT1;
    }
    else if (Argc >= 5) // Mc, phy_addr & atleast two bit is passed as argument.
    {
        int i;
        int bit;
        if ((Argv[1] != NULL) && (Argv[2] != NULL))
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
            p_addr = (uint64_t)(strtoul(Argv[2], 0, 0));
            check_null |= true;
        }
        for (i = 3; i < Argc; i++)
        {
            if (Argv[i] != NULL)
            {
                bit = (uint16_t)(strtoul(Argv[i], 0, 0));
                if (bit >= 0 && bit < 10)
                {
                    BIT_Value |= 1 << bit;
                    check_null |= true;
                }
                else
                {
                    check_null &= false;
                    break;
                }
            }
            else
            {
                check_null &= false;
                break;
            }
        }
    }
    else
    {
        FpFwCliPrint("Invalid Argument");
        return CLI_ERROR;
    }

    if ((check_null == false) || (!ddr_ecc_err_inj_validation(mc, BIT_Value)))
    {
        FpFwCliPrint("Invalid Argument");
        return CLI_ERROR;
    }

    ddr_ecc_error_injection(die_id, mc, p_addr, BIT_Value);
    FpFwCliPrint("DDR: ecc_ue_error_injection - Done!!\n");
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS cmd_addr_parity_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    DDRSS_MEDIA_CA_INJ_CMD cmd = DDRSS_MEDIA_CA_INJ_CMD_RD;

    if (Argc == 1) // no argument passed
    {
        mc = 0;
    }
    else if (Argc == 2) // only mc is passed as argument
    {
        if (Argv[1] != NULL)
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
        }
    }
    else if (Argc == 3) // mc + cmd are passed as arguments
    {
        if ((Argv[1] != NULL) && (Argv[2] != NULL))
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
            cmd = (uint32_t)(strtoul(Argv[2], 0, 0));
        }
    }
    else
    {
        FpFwCliPrint("Invalid number of arguments");
        return CLI_ERROR;
    }

    if (!is_mc_are_belong_to_die(mc))
    {
        return CLI_ERROR;
    }

    if (cmd > DDRSS_MEDIA_CA_INJ_CMD_DRFMSB)
    {
        FpFwCliPrint("Invalid Argument - cmd (if supplied) must be between [0 <= cmd <= 0x85]\n");
        return CLI_ERROR;
    }

    ddr_ca_parity_error_injection(mc, cmd);
    FpFwCliPrint("DDR: cmd_addr_parity_error_injection - Done!!\n");

    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS wrrtydat_ue_error_injection(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    // TODO Silibs to provide APIs with exact arguments list
    // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974

    FpFwCliPrint("Work in progress: wrrtydat_ue_error_injection\n");

    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS media_patrol_scrub_ce_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;

    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_media_patrol_scrub_ce(mc);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS fedb_merge_data_ce_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_fedb_merge_data_ce(mc);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS media_patrol_scrub_ue_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_media_patrol_scrub_ue(mc);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS mainline_traffic_ce_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_mainline_traffic_ce(mc);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS mainline_traffic_ue_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_mainline_traffic_ue(mc);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS mrdp_parity_ue_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_mrdp_parity_ue(mc);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS ca_parity_persistent_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_ca_parity_persistent(mc);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS ca_parity_transient_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_ca_parity_transient(mc);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS rh_counters_sram_parity_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_rh_counters_sram_parity(mc);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS rh_drfm_sram_parity_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_rh_drfm_sram_parity(mc);
    return CLI_SUCCESS;
}

static bool check_params(int Argc, const char** Argv, uint32_t* mc)
{
    if (Argc == 1) // no argument passed
    {
        *mc = 0; // Default to MC0
    }
    else if (Argc == 2) // only mc is passed as argument
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
    else
    {
        FpFwCliPrint("Invalid number of arguments");
        return false;
    }

    if (!is_mc_are_belong_to_die(*mc))
    {
        return false;
    }

    return true;
}

STATIC FPFW_CLI_STATUS read_dimm_pmic_power(int Argc, const char** Argv)
{
    uint8_t ddrss_index = 0;
    uint16_t power_mw = 0;
    int sts = 0;

    if (Argc != 2)
    {
        FpFwCliPrint("Invalid number of arguments.  Usage: read_dimm_pmic_power <dimm_idx>\n");
        return CLI_ERROR;
    }

    ddrss_index = (uint8_t)(strtoul(Argv[1], NULL, 0));
    sts = ddr_manager_power_mw_read(ddrss_index, &power_mw);
    if (sts != 0)
    {
        FpFwCliPrint("Error reading PMIC power\n");
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS read_dimm_temp_sensor(int Argc, const char** Argv)
{
    uint8_t ddrss_index = 0;
    uint8_t channel_idx = 0;
    ddr_manager_i3c_temperature_t temperature = {0};
    int sts = 0;

    if (Argc != 3)
    {
        FpFwCliPrint("Invalid number of arguments.  Usage: read_dimm_temp_sensor <dimm_idx> <channel_idx>\n");
        return CLI_ERROR;
    }

    ddrss_index = (uint8_t)(strtoul(Argv[1], NULL, 0));
    channel_idx = (uint8_t)(strtoul(Argv[2], NULL, 0));
    sts = ddr_manager_temperature_sensor_read(ddrss_index, channel_idx, &temperature);
    if (sts != 0)
    {
        FpFwCliPrint("Error reading DIMM temperature sensor\n");
        return CLI_ERROR;
    }

    FpFwCliPrint("Temperature: %d.%d\n", temperature.temp_int, temperature.temp_frac);
    return CLI_SUCCESS;
}

static bool is_mc_are_belong_to_die(uint32_t mc)
{
    KNG_DIE_ID die_id = idsw_get_die_id();
    if (die_id == DIE_0)
    {
        if (mc > 11)
        {
            FpFwCliPrint("Invalid Argument - mc (if supplied) must be between 0 and 11 for DIE 0");
            return false;
        }
    }
    else
    {
        if ((mc > 23) || (mc < 12))
        {
            FpFwCliPrint("Invalid Argument - mc (if supplied) must be between 12 and 23 for DIE 1");
            return false;
        }
    }

    return true;
}

void cli_ddr_init(void)
{
    FpFwCliRegisterTable(&cli_ddr_commands[0], FPFW_ARRAY_SIZE(cli_ddr_commands));
}
