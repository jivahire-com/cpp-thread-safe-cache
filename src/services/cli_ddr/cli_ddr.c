//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_ddr.c
 * Source file to implement ddr cli
 */

/*------------- Includes -----------------*/
#include <DfwkCommon.h>
#include <FpFwCli.h>        // for FPFW_CLI_STATUS, CLI_SUCCESS, FpFwCliReg...
#include <FpFwLinkedList.h> // for NULL_LIST_ENTRY
#include <FpFwUtils.h>      // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <atu_api.h>
#include <bug_check.h>
#include <cli_ddr.h>
#include <ddr_err_inj.h>
#include <ddr_manager.h>
#include <ddr_manager_bwl.h>
#include <ddr_manager_i3c.h>
#include <ddr_memory_map.h>
#include <ddrss.h>
#include <ddrss_lib.h>
#include <ddrss_sdl.h> // for SDL_VAR_NAME, SDL_VAR_GUID, MEMORY_DEFECT_LIST_HEADER
#include <idsw_kng.h>  // for idsw_get_die_id
#include <memory_map/mscp_exp_rmss_memory_map.h>
#include <stdio.h> // for printf
#include <stdlib.h>
#include <utils.h>
#include <variable_services.h> // for var_service_shared_mem_t, var_serv...

/*-- Symbolic Constant Macros (defines) --*/
#ifdef UNIT_TEST
    #define STATIC
#else
    #define STATIC static
#endif

// Uncomment to add test features that should not be released
#define SDL_DEV_MODE (true)

#ifdef SDL_DEV_MODE
static var_service_shared_mem_t mem_ctx = {0};
static var_service_req_ctx_t sdl_set_var_ctx = {0};
static var_service_req_params_t req_params = {0};
static uint16_t sdl_var_name[] = SDL_VAR_NAME;
static const guid_t sdl_var_guid[] = {SDL_VAR_GUID};
#endif

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

// DIMM I3C Commands
STATIC FPFW_CLI_STATUS read_dimm_pmic_power(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS read_dimm_temp_sensor(int Argc, const char** Argv);

// BWL/Throttling Commands
STATIC FPFW_CLI_STATUS ddr_manager_bwl_force(int Argc, const char** Argv);

// XTS AES Keystore Error Injection
STATIC FPFW_CLI_STATUS xts_aes_keystore_ce_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS xts_aes_keystore_ue_error_injection(int Argc, const char** Argv);

#ifdef SDL_DEV_MODE
// Dont release this -- test only
STATIC FPFW_CLI_STATUS write_sdl(int Argc, const char** Argv);
void vs_sdl_cb(void* context, var_service_req_ctx_t* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size);
STATIC FPFW_CLI_STATUS ddr_manager_ppr_status_update(int Argc, const char** Argv);
#endif

/*-- Declarations (Statics and globals) --*/

// TODO Silibs to provide APIs with arguments list
// https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
STATIC FPFW_CLI_COMMAND cli_ddr_commands[] = {
    {NULL_LIST_ENTRY, "ddr_err_inj", "ecc_ce_err", ecc_ce_error_injection, "CE injection", "ecc_ce_err <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "ecc_ue_err", ecc_ue_error_injection, "UE injection", "ecc_ue_err <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "capar_err", cmd_addr_parity_error_injection, "CAPAR error", "caddr_parity_err <mc> <einj_cmd>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "wrrtydat_ue_err", wrrtydat_ue_error_injection, "WRRTYDAT UE injection", "wrrtydat_ue_err"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "media_patrol_scrub_ce", media_patrol_scrub_ce_error_injection, "Media Patrol Scrub CE injection", "media_patrol_scrub_ce <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "merge_data_ce", fedb_merge_data_ce_error_injection, "FEDB Merge Data CE injection", "merge_data_ce <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "merge_data_ue", media_patrol_scrub_ue_error_injection, "FEDB Merge Data UE injection", "merge_data_ue <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "mainline_traffic_ce", mainline_traffic_ce_error_injection, "Mainline Traffic CE injection", "mainline_traffic_ce <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "mainline_traffic_ue", mainline_traffic_ue_error_injection, "Mainline Traffic UE injection", "mainline_traffic_ue <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "mrdp_parity_ue", mrdp_parity_ue_error_injection, "MRDP Parity UE injection", "mrdp_parity_ue <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "ca_parity_persistent", ca_parity_persistent_error_injection, "Command Address Parity - Persistent einj", "ca_parity_persistent <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "ca_parity_transient", ca_parity_transient_error_injection, "Command Address Parity - Transient einj", "ca_parity_transient <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "xts_aes_keystore_ce", xts_aes_keystore_ce_error_injection, "XTS AES Keystore CE injection", "xts_aes_keystore_ce <mc>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "xts_aes_keystore_ue", xts_aes_keystore_ue_error_injection, "XTS AES Keystore UE injection", "xts_aes_keystore_ue <mc>"},

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    {NULL_LIST_ENTRY, "ddr", "read_dimm_pmic_power", read_dimm_pmic_power, "Read PMIC power", "read_dimm_pmic_power <dimm_idx>"},
    {NULL_LIST_ENTRY, "ddr", "read_dimm_temp_sensor", read_dimm_temp_sensor, "Read DIMM temperature sensor", "read_dimm_temp_sensor <dimm_idx> <channel_idx>"},
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    {NULL_LIST_ENTRY, "ddr", "rh_counters", rh_counters_sram_parity_error_injection, "Row hammer counters sram parity einj (TBD)", "rh_counters <mc>"},
    {NULL_LIST_ENTRY, "ddr", "rh_drfm", rh_drfm_sram_parity_error_injection, "Command Address Parity - Transient einj (TBD)", "rh_drfm <mc>"},
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    {NULL_LIST_ENTRY, "ddr", "bwl_force", ddr_manager_bwl_force, "Control BWL forced throttling state", "bwl_force <0|1>"},

#ifdef SDL_DEV_MODE
    {NULL_LIST_ENTRY, "ddr_ppr", "write_sdl", write_sdl, "(Over-)Writes an empty SDL variable to flash", "write_sdl >"},
    {NULL_LIST_ENTRY, "ddr", "ppr", ddr_manager_ppr_status_update, "Invoke PPR status update", "ppr <dimm_num>"},
#endif
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
};

STATIC PLACED_CODE uint32_t get_default_error_injection_mc()
{
    KNG_DIE_ID die_id = idsw_get_die_id();
    return (die_id == DIE_0) ? 0 : DDRSS_MAX_MC_NUM_PER_DIE;
}

// ecc_ce_err (mc) (phy_add) {error bit}
STATIC PLACED_CODE FPFW_CLI_STATUS ecc_ce_error_injection(int Argc, const char** Argv)
{
    KNG_DIE_ID die_id = idsw_get_die_id();
    uint32_t mc = get_default_error_injection_mc();
    uint64_t p_addr = ddr_err_inj_get_default_address();
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
    FpFwCliPrint("ecc_ce_error_injection - Done\n");

    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS ecc_ue_error_injection(int Argc, const char** Argv)
{
    KNG_DIE_ID die_id = idsw_get_die_id();
    uint32_t mc = get_default_error_injection_mc();
    uint64_t p_addr = ddr_err_inj_get_default_address();
    uint16_t BIT_Value = BIT0 | BIT1;
    bool check_null = false;

    if (Argc == 1) // no argument passed
    {
        mc = 0;
        check_null |= true;
    }
    else if (Argc == 2) // only mc is passed as argument
    {
        if (Argv[1] != NULL)
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
            check_null |= true;
        }
    }
    else if (Argc == 3) // only mc & phy_addr is passed as argument
    {
        if ((Argv[1] != NULL) && (Argv[2] != NULL))
        {
            mc = (uint32_t)(strtoul(Argv[1], 0, 0));
            p_addr = (uint64_t)(strtoul(Argv[2], 0, 0));
            check_null |= true;
        }
    }
    else
    {
        FpFwCliPrint("Invalid # arguments");
        return CLI_ERROR;
    }

    if ((check_null == false) || (!ddr_ecc_err_inj_validation(mc, BIT_Value)))
    {
        FpFwCliPrint("Invalid Argument mc or BIT_Value\n");
        return CLI_ERROR;
    }

    ddr_ecc_error_injection(die_id, mc, p_addr, BIT_Value);
    FpFwCliPrint("ecc_ue_error_injection - Done\n");
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS cmd_addr_parity_error_injection(int Argc, const char** Argv)
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
        FpFwCliPrint("Invalid Argument\n");
        return CLI_ERROR;
    }

    ddr_ca_parity_error_injection(mc, cmd);
    FpFwCliPrint("cmd_addr_parity_error_injection - Done\n");

    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS wrrtydat_ue_error_injection(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    // TODO Silibs to provide APIs with exact arguments list
    // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974

    FpFwCliPrint("Work in progress\n");

    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS media_patrol_scrub_ce_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;

    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_media_patrol_scrub_ce(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS fedb_merge_data_ce_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_fedb_merge_data_ce(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS media_patrol_scrub_ue_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_media_patrol_scrub_ue(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS mainline_traffic_ce_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_mainline_traffic_ce(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS mainline_traffic_ue_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_mainline_traffic_ue(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS mrdp_parity_ue_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_mrdp_parity_ue(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS ca_parity_persistent_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_ca_parity_persistent(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS ca_parity_transient_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_inj_ca_parity_transient(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS rh_counters_sram_parity_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_rh_counters_sram_parity(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS rh_drfm_sram_parity_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_rh_drfm_sram_parity(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS xts_aes_keystore_ce_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_xts_aes_keystore_ce(mc);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS xts_aes_keystore_ue_error_injection(int Argc, const char** Argv)
{
    uint32_t mc = 0;
    if (!check_params(Argc, Argv, &mc))
    {
        return CLI_ERROR;
    }
    ddr_err_xts_aes_keystore_ue(mc);
    return CLI_SUCCESS;
}

static PLACED_CODE bool check_params(int Argc, const char** Argv, uint32_t* mc)
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

STATIC PLACED_CODE FPFW_CLI_STATUS read_dimm_pmic_power(int Argc, const char** Argv)
{
    uint8_t ddrss_index = 0;
    uint16_t power_mw = 0;
    int sts = 0;

    if (Argc != 2)
    {
        FpFwCliPrint("Invalid arguments\n");
        return CLI_ERROR;
    }

    ddrss_index = (uint8_t)(strtoul(Argv[1], NULL, 0));
    if (ddrss_index > 5)
    {
        FpFwCliPrint("Invalid arguments\n");
        return CLI_ERROR;
    }

    sts = ddr_manager_power_mw_read(ddrss_index, &power_mw);
    if (sts != 0)
    {
        FpFwCliPrint("Error reading PMIC power\n");
        return CLI_ERROR;
    }

    FpFwCliPrint("DIMM %d Power: %d mW\n", ddrss_index, power_mw);
    return CLI_SUCCESS;
}

STATIC PLACED_CODE FPFW_CLI_STATUS read_dimm_temp_sensor(int Argc, const char** Argv)
{
    uint8_t ddrss_index = 0;
    uint8_t channel_idx = 0;
    ddr_manager_i3c_temperature_t temperature = {0};
    int sts = 0;

    if (Argc != 3)
    {
        FpFwCliPrint("Invalid arguments\n");
        return CLI_ERROR;
    }

    ddrss_index = (uint8_t)(strtoul(Argv[1], NULL, 0));
    if (ddrss_index > 5)
    {
        FpFwCliPrint("Invalid arguments\n");
        return CLI_ERROR;
    }

    channel_idx = (uint8_t)(strtoul(Argv[2], NULL, 0));
    if (channel_idx > 1)
    {
        FpFwCliPrint("Invalid arguments\n");
        return CLI_ERROR;
    }

    sts = ddr_manager_temperature_sensor_read(ddrss_index, channel_idx, &temperature);
    if (sts != 0)
    {
        FpFwCliPrint("Error reading DIMM temperature sensor\n");
        return CLI_ERROR;
    }

    FpFwCliPrint("Temperature: %d.%d\n", temperature.temp_int, temperature.temp_frac);
    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS ddr_manager_bwl_force(int Argc, const char** Argv)
{
    if (Argc != 2)
    {
        FpFwCliPrint("Invalid arguments\n");
        return CLI_ERROR;
    }

    char* endptr;
    uint8_t force = (uint8_t)(strtoul(Argv[1], &endptr, 0));
    if (*endptr != '\0' || (force != 0 && force != 1))
    {
        FpFwCliPrint("Invalid arguments\n");
        return CLI_ERROR;
    }

    if (force == 1)
    {
        ddr_manager_enable_bwl_force();
    }
    else
    {
        ddr_manager_disable_bwl_force();
    }

    return CLI_SUCCESS;
}

static bool is_mc_are_belong_to_die(uint32_t mc)
{
    KNG_DIE_ID die_id = idsw_get_die_id();
    if (die_id == DIE_0)
    {
        if (mc > 11)
        {
            FpFwCliPrint("Invalid arguments\n");
            return false;
        }
    }
    else
    {
        if ((mc > 23) || (mc < 12))
        {
            FpFwCliPrint("Invalid arguments\n");
            return false;
        }
    }

    return true;
}

#ifdef SDL_DEV_MODE
void vs_sdl_cb(void* context, var_service_req_ctx_t* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size)
{
    //! cb is raised when recieve is complete
    //! check status & do work
    FPFW_UNUSED(context);
    FPFW_UNUSED(data_start_ptr);
    FPFW_UNUSED(data_size);

    const char* operation_str = (var_serv_ctx->operation_type == ASYNC_GET_VARIABLE) ? "Get" : "Set";
    printf("SDL %s variable completion status = %x\n", operation_str, (int)var_serv_ctx->async_req_result);

    // variable_service_unlock_get_var_ctx(var_serv_ctx);

    FpFwCliPrint("SDL cb - read data = %d\n", (int)*data_start_ptr);
    FpFwCliPrint("SDL cb - data size = %d\n", (int)data_size);

    // Unlock the get variable context
    variable_service_unlock_get_var_ctx(var_serv_ctx);
}

STATIC FPFW_CLI_STATUS write_sdl(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    KNG_DIE_ID die_id = idsw_get_die_id();
    if (die_id == DIE_1)
    {
        return CLI_ERROR;
    }

    // var_service_shared_mem_t local_mem_ctx = {0};
    mem_ctx.payload_base = (uintptr_t)SCP_EXP_SCP_DDRSS_PPR_VARIABLE_SERVICE_PAYLOAD_BASE;
    mem_ctx.max_payload_size = SCP_EXP_SCP_DDRSS_PPR_VARIABLE_SERVICE_PAYLOAD_SIZE;

    variable_service_initialize_ctx(&sdl_set_var_ctx, &mem_ctx);

    MEMORY_DEFECT_LIST_HEADER empty_sdl_header = {0};
    empty_sdl_header.Signature = (uint32_t)PSHED_PI_DEFECT_LIST_SIGNATURE;
    empty_sdl_header.Version = MEMORY_DEFECT_VERSION_20;
    empty_sdl_header.Length = sizeof(MEMORY_DEFECT_LIST_HEADER);
    empty_sdl_header.DefectCount = 0;
    empty_sdl_header.Changed = 0;
    empty_sdl_header.Checksum = 0;

    empty_sdl_header.Checksum =
        (uint32_t)CalculateRemoteCheckSum16((uint32_t)&empty_sdl_header, sizeof(MEMORY_DEFECT_LIST_HEADER));

    req_params.variable_name_ptr = (uint16_t*)sdl_var_name;
    req_params.variable_name_size = sizeof(sdl_var_name);
    memcpy(&req_params.vendor_namespace_guid, sdl_var_guid, sizeof(req_params.vendor_namespace_guid));
    req_params.data = (uint8_t*)&empty_sdl_header;
    req_params.data_size = sizeof(empty_sdl_header);
    req_params.attributes.as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    FpFwCliPrint("Calling variable_service_async_set_variable to write empty SDL variable\n");
    int status = variable_service_async_set_variable(&sdl_set_var_ctx, &req_params, vs_sdl_cb, NULL);
    if (status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("Failed to start async variable set: 0x%x\n", status);
        return CLI_ERROR;
    }
    FpFwCliPrint("status = 0x%lx\n", status);

    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS ddr_manager_ppr_status_update(int Argc, const char** Argv)
{
    ddrs_spd_addr_info_t addr_info;
    ddrss_res_info_t res_info;

    if (Argc != 2)
    {
        FpFwCliPrint("Invalid arguments\n");
        return CLI_ERROR;
    }

    char* endptr;
    uint8_t dimm_num = (uint8_t)(strtoul(Argv[1], &endptr, 0));
    if ((*endptr != '\0') || (dimm_num > 5))
    {
        FpFwCliPrint("Invalid arguments\n");
        return CLI_ERROR;
    }

    addr_info.dimm = dimm_num;
    res_info.sel_ppr_status = (e_ppr_sel_status_t)E_PPR_STATUS_NO_REPAIR;
    res_info.spd_ppr_status = (e_ppr_spd_status_t)E_DTR_STATUS_NO_REPAIR;
    res_info.num_repair_rows = 0x10;

    ddrss_update_ppr_completion(&addr_info, &res_info);

    return CLI_SUCCESS;
}
#endif

void cli_ddr_init(void)
{
    FpFwCliRegisterTable(&cli_ddr_commands[0], FPFW_ARRAY_SIZE(cli_ddr_commands));
}
