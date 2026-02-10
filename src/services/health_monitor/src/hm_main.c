//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_main.c
 * Implements health monitor main functionality.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <atu_api.h>
#include <bug_check.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <idhw.h>
#include <idsw.h>
#include <mscp_exp_spi_synchronize_dies.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
// ASCII of 'C''P''E''R'
#define D2D_HM_SIGNATURE 0x43504552

/*-------- Function Prototypes -----------*/
void setup_intercore_error_handling(hm_intercore_type_t intercore_type, fpfw_icc_base_ctx_t* icc_ctx);
void wait_for_ghes_construction();

/*-- Declarations (Statics and globals) --*/
static hm_config_t* health_monitor_config = NULL;
static bool ddr_subsystem_up = false;

/*------------- Functions ----------------*/
static void setup_ghes_and_reset_last_cper_status()
{
    construct_mscp_ghes_table();

    // No CPER retry transfer after warm reset, handled by HSP
    hm_set_pldm_transfer_status(HM_PLDM_TRANSFER_STATUS_IDLE, true);
    hm_set_pldm_transfer_status(HM_PLDM_TRANSFER_STATUS_IDLE, false);
}

hm_config_t* get_hm_config()
{
    return health_monitor_config;
}

void hm_pre_ddr_init(hm_config_t* hm_config)
{
    health_monitor_config = hm_config;
}

void hm_post_ddr_init()
{
    ddr_subsystem_up = true;

    hm_config_t* hm_config = get_hm_config();
    HM_ET_INFO(HM_ET_TYPE_MAIN_GET_CONFIG);
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

    if (hm_config->is_mcp)
    {
        ddr_subsystem_up = true;
        set_ghes_table_ready();
        return;
    }

    // De-assert FATAL_ERROR GPIO
    hm_report_error_event(HM_ERROR_REPORT_GPIO, false);

    // Construct GHES table
    if (idhw_is_single_die_boot_en() == true)
    {
        setup_ghes_and_reset_last_cper_status();
    }
    else
    {
        if (hm_config->is_primary == true)
        {
            setup_ghes_and_reset_last_cper_status();
        }
        else
        {
            // Secondary scp wait for primary scp to construct GHES table
            HM_LOG_INFO("Waiting GHES completion");
        }

        wait_for_ghes_construction();
    }

    // Activate any queued error domain.
    hm_register_cached_error_domain();

    // process pre-mesh CPER error report
    hm_submit_cached_cper();
}

void hm_post_intercore_init(hm_intercore_type_t intercore_type, fpfw_icc_base_ctx_t* icc_ctx)
{
    hm_config_t* hm_config = get_hm_config();
    HM_ET_INFO(HM_ET_TYPE_MAIN_GET_CONFIG);
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);
    BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);
    hm_config->icc_ctx[intercore_type] = icc_ctx;

    switch (intercore_type)
    {
    case HM_INTERCORE_LOCAL:
        if (hm_config->is_mcp)
        {
            // ICC set up for PLDM CPER transfer on MCPs
            if (hm_config->is_primary == true)
            {
                hm_cper_transfer_listener_from_secondary_mcp(icc_ctx);
            }
        }
        else
        {
            // ICC set up for error injection routing on SCPs
            hm_prepare_error_injection_listener(icc_ctx);
        }
        break;
    case HM_INTERCORE_REMOTE:
        if (hm_config->is_mcp)
        {
            // listener for PLDM CPER transfer request from SCP
            hm_cper_transfer_listener_from_scp(icc_ctx);
        }
        else
        {
            // listener for MCP error domain and readiness
            hm_prepare_mscp_listener(icc_ctx);
        }
        break;
    case HM_INTERCORE_SDM:
        hm_prepare_sdm_listener(icc_ctx);
        break;
    case HM_INTERCORE_CDED:
        hm_prepare_cded_sdm_listener(icc_ctx);
        break;
    case HM_INTERCORE_HSP:
        hm_prepare_hsp_listener(icc_ctx);
        break;
    case HM_INTERCORE_APCORE:
        hm_prepare_ap_listener(icc_ctx);
        break;
    default:
        HM_ET_ERROR_PARAM(HM_ET_TYPE_MAIN_INVALID_PARAMS, intercore_type);
        break;
    }
}

bool ddr_subsystem_enabled()
{
    return ddr_subsystem_up;
}

void wait_for_ghes_construction()
{
    d2d_sync_point.value = D2D_HM_SIGNATURE;

    int d2d_sync_status = mscp_exp_spi_synchronize_dies(d2d_sync_point, idsw_get_die_id());
    HM_ET_INFO_PARAM(HM_ET_TYPE_MAIN_SYNC_DIES, d2d_sync_status);
    BUG_ASSERT_PARAM(d2d_sync_status == SILIBS_SUCCESS, d2d_sync_status, SILIBS_SUCCESS);

    set_ghes_table_ready();
}

bool hm_allow_ras_reporting(void)
{
    return !(idhw_is_single_die_boot_en() && config_get_ras_disable_single_die());
}