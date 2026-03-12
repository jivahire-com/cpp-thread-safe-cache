//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cfg_mgr_init.c
 * This file contains the config/initialization for the fpfw configuration manager.
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <boot_status.h>
#include <config_manager.h>
#include <config_manager_cli.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <mscp_exp_rmss_memory_map.h>
#include <system_info.h>
#include <utils.h>
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/
// Config profile IDs are defined by the order of FPFW_CFG_MGR_PROFILES
#define MSCP_CONFIG_PROFILE_PLATFORM         0
#define MSCP_CONFIG_PROFILE_FPGA             1
#define MSCP_CONFIG_PROFILE_SVP              2
#define MSCP_CONFIG_PROFILE_SVP_MIN_CONFIG   3
#define MSCP_CONFIG_PROFILE_EMU              4
#define MSCP_CONFIG_BMC_PROFILE_COMPUTE_OVL2 5 // override / compute / AE - OVL2 SKU
#define MSCP_CONFIG_BMC_PROFILE_COMPUTE_OVL3 6 // override / compute / AG - OVL3 SKU

// BMC Profiles
#define BMC_PROFILE_COMPUTE_OVL2 0x01 // override / compute / AE - OVL2 SKU
#define BMC_PROFILE_COMPUTE_OVL3 0x02 // override / compute / AG - OVL3 SKU

/*-- Declarations (Statics and globals) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-------------- Functions ---------------*/
static PLACED_CODE uint8_t get_profile_id()
{
    switch (system_info_get_bmc_profile())
    {
    case BMC_PROFILE_COMPUTE_OVL2:
        return MSCP_CONFIG_BMC_PROFILE_COMPUTE_OVL2;
    case BMC_PROFILE_COMPUTE_OVL3:
        return MSCP_CONFIG_BMC_PROFILE_COMPUTE_OVL3;
    default:
        break;
    }

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
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_2D_8C:
        return MSCP_CONFIG_PROFILE_EMU;
    default:
        break;
    }

    return MSCP_CONFIG_PROFILE_PLATFORM;
}

/**
 * @brief Initialize cfg_mgr.
 *
 */
// TODO: The UART initialization component is now dependent on the configuration manager. To resolve
//       early boot stage debugging via UART see ado: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2856417
PLACED_CODE FPFW_INIT_COMPONENT(
    cfg_mgr,
    FPFW_INIT_DEPENDENCIES("dfwk", "atu_svc", "var_serv", "hw_ver", "spi_bridge", "sysinfo", "icc_d2dmbx", "gtimer", "systick_upd", "boot_stat"))
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

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_CFGMGR_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_CFGMGR_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

/**
 * @brief Initialize cfg_mgr CLI.
 *
 */
PLACED_CODE FPFW_INIT_COMPONENT(cfg_mgr_cli, FPFW_INIT_DEPENDENCIES("cfg_mgr", "cli"))
{
    cfg_mgr_cli_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}