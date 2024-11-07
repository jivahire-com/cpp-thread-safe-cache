//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cfg_mgr_init.c
 * This file contains the config/initialization for the fpfw configuration manager.
 */

/*------------- Includes -----------------*/
#include <config_manager.h>
#include <config_manager_cli.h>
#include <fpfw_init.h>         
#include <mscp_exp_rmss_memory_map.h>
#include <variable_services.h>

/**
 * @brief Initialize cfg_mgr.
 *
 */
FPFW_INIT_COMPONENT(cfg_mgr, FPFW_INIT_DEPENDENCIES("std_io", "dfwk", "var_serv", "hw_ver", "sysinfo"))
{
    // This struct is only used to initialize a fpfw_cfg_mgr_db_t struct
    fpfw_cfg_mgr_config_t cfg_mgr_config = {
        .mission_mode = false,
        .profile_id = 0,
        .read_knob_fn = read_knob_from_default_db_cb,
        .write_knob_fn = update_knob_in_cached_db_cb,
        .db_ctx = (void*)1
    };

    // to do - https://dev.azure.com/ms-tsd/Kingsgate/_git/silibs/pullrequest/221563
    // until then, we will use the following test purpose memory
    var_service_shared_mem_t var_svc_mem_ctx = {
        .payload_base = (uintptr_t)SCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_BASE,
        .max_payload_size = SCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_SIZE,
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
