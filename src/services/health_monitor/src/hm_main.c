//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_main.c
 * Implements health monitor main functionality.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <bug_check.h>
#include <fpfw_init.h>
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
void hm_pre_ddr_init(hm_config_t* hm_config)
{
    health_monitor_config = hm_config;
}

void hm_post_ddr_init()
{
    ddr_subsystem_up = true;

    hm_config_t* hm_config = get_hm_config();
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

    if (hm_config->is_primary == true)
    {
        initialize_semaphore(hm_config->semaphore_id);
    }

    // Construct GHES table
    if (idhw_is_single_die_boot_en() == true)
    {
        // Single die boot
        construct_mscp_ghes_table();
    }
    else
    {
        if (hm_config->is_primary == true)
        {
            // Primary scp start construct GHES table
            construct_mscp_ghes_table();
        }
        else
        {
            // Secondary scp wait for primary scp to construct GHES table
            HM_LOG_INFO("Waiting GHES completion");

            // Temporary code block until sharable memory is available on ARSM
            // ToDo - ADO 2347758 : Define RAS related memory block on DIE 0 ARSM
            construct_mscp_ghes_table();
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
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);
    BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);
    hm_config->icc_ctx[intercore_type] = icc_ctx;

    switch (intercore_type)
    {
    case HM_INTERCORE_SCP:
        hm_prepare_error_injection_listener(icc_ctx);
        break;
    default:
        break;
    }
}

hm_config_t* get_hm_config()
{
    return health_monitor_config;
}

bool ddr_subsystem_enabled()
{
    return ddr_subsystem_up;
}

void wait_for_ghes_construction()
{
    d2d_sync_point.value = D2D_HM_SIGNATURE;

    int d2d_sync_status = mscp_exp_spi_synchronize_dies(d2d_sync_point, idsw_get_die_id());
    BUG_ASSERT_PARAM(d2d_sync_status == SILIBS_SUCCESS, d2d_sync_status, SILIBS_SUCCESS);

    set_ghes_table_ready();
}
