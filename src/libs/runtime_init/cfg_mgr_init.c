//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cfg_mgr_init.c
 * This file contains the config/initialization for the fpfw configuration manager.
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <config_manager.h>
#include <config_manager_cli.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <mscp_exp_rmss_memory_map.h>
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/
// Config profile IDs are defined by the order of FPFW_CFG_MGR_PROFILES
#define MSCP_CONFIG_PROFILE_PLATFORM       0
#define MSCP_CONFIG_PROFILE_FPGA           1
#define MSCP_CONFIG_PROFILE_SVP            2
#define MSCP_CONFIG_PROFILE_SVP_MIN_CONFIG 3

/*-- Declarations (Statics and globals) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-------------- Functions ---------------*/
static uint8_t get_profile_id()
{
    switch (idsw_get_platform_sdv())
    {
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        return MSCP_CONFIG_PROFILE_FPGA;
    case PLATFORM_SVP_SIM:
        return MSCP_CONFIG_PROFILE_SVP;
    case PLATFORM_SVP_MIN_CONFIG_SIM:
        return MSCP_CONFIG_PROFILE_SVP_MIN_CONFIG;
    default:
        break;
    }

    return MSCP_CONFIG_PROFILE_PLATFORM;
}

/**
 * @brief Initialize cfg_mgr.
 *
 */
FPFW_INIT_COMPONENT(cfg_mgr, FPFW_INIT_DEPENDENCIES("std_io", "dfwk", "atu_svc", "var_serv", "hw_ver", "sysinfo", "icc_d2dmbx"))
{
    // This struct is only used to initialize a fpfw_cfg_mgr_db_t struct
    fpfw_cfg_mgr_config_t cfg_mgr_config = {.mission_mode = false,
                                            .profile_id = get_profile_id(),
                                            .read_knob_fn = read_knob_from_default_db_cb,
                                            .write_knob_fn = update_knob_in_cached_db_cb,
                                            .db_ctx = (void*)1};

    var_service_shared_mem_t var_svc_mem_ctx = {
        .payload_base = (uintptr_t)MSCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_BASE,
        .max_payload_size = MSCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_SIZE,
    };

    cfg_mgr_init(&cfg_mgr_config, &var_svc_mem_ctx);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

/**
 * @brief Initialize cfg_mgr CLI.
 *
 */
FPFW_INIT_COMPONENT(cfg_mgr_cli, FPFW_INIT_DEPENDENCIES("cfg_mgr", "cli"))
{
    cfg_mgr_cli_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
