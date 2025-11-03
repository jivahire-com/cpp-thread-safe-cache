//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_rhtlm_service.c
 * This file contains the Row Hammer Telemetry services
 */

/*-------------------------------- Includes ---------------------------------*/
#include "ddr_manager_i.h"

#include <DbgPrint.h>
#include <ErrorHandler.h>
#include <FpFwUtils.h>
#include <ddr_rhtlm_service.h>
#include <ddrss.h>
#include <ddrss_config.h>
#include <ddrss_runtime_api.h>
#include <health_monitor.h>
#include <idsw.h> // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <mu_public.h>
#include <stdint.h> // for uint8_t
#include <stdio.h>  // for printf
#include <tx_api.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/
static rhtlm_service_ctx_t* rhtlm_context = NULL;
/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief  : Apply filter if production mode is active
 * @param  : void
 * @retval : void
 */
void rhtlm_cca_mode_filtering()
{
    ddrss_rhm_tm_rpt_mask_t filter_mask;

    if (config_get_rh_tlm_cca_mode() == 0)
    { // no action needed , we are not in production MODE
        // TODO : is there an runtime API that can determine if we are in production mode or not ?
        return;
    }

    // fbuilding mask filter
    filter_mask.as_uint32 = config_get_rm_tm_rec_mask();
    filter_mask.way_info_addr = 0;
    filter_mask.way_info_count = 0;
    filter_mask.way_info_lock = 0;
    filter_mask.lower_row = 0;
    filter_mask.upper_row = 0;
    filter_mask.type_drfm_drop = 0;
    filter_mask.type_drfm_req = 0;

    FPFW_DBGPRINT_INFO("[rh_svc] CCA filtering mask: 0x%x\n", filter_mask.as_uint32);
    int result;
    result = ddrss_set_rhm_telemetry_record_mask(&filter_mask);

    if (result != SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_WARNING("[rh_svc] res %d exp %d ", result, SILIBS_SUCCESS);
    }
}

void rhtlm_cfg_scan(void)
{
    FPFW_DBGPRINT_INFO("[rh_svc] scan MC cfg\n");

    KNG_DIE_ID die_id = idsw_get_die_id();
    ddrss_rhm_tm_cfg_t ddr_current_cfg;
    acpi_err_sec_ddrss_rhm_cfg_t rh_cper_section = {0};
    acpi_cper_section_t cper_section;

    int sub_sts = SILIBS_E_DATA;
    uint32_t start_mc = 0;
    uint32_t stop_mc = RHTLM_MC_DIE0_COUNT_NR;
    uint32_t rec_count = 0;

    if (die_id == DIE_1)
    {
        start_mc = RHTLM_MC_DIE0_COUNT_NR;
        stop_mc = RHTLM_MC_DIE1_COUNT_NR;
    }

    for (uint32_t mc_nr = start_mc; mc_nr < stop_mc; mc_nr++)
    {
        sub_sts = ddrss_get_telemetry_record(mc_nr, DDRSS_TELEMETRY_TYPE_RHM_CFG, &ddr_current_cfg, sizeof(ddr_current_cfg));
        if (sub_sts != SILIBS_SUCCESS)
        {
            continue;
        }

        memset(&cper_section, 0, sizeof(cper_section));
        memset(&rh_cper_section, 0, sizeof(rh_cper_section));
        prod_ddrss_convert_rh_cfg_rec_to_rh_cper(mc_nr, RHTLM_CFG, &ddr_current_cfg, &rh_cper_section);
        cper_section.sec_rh_cfg = rh_cper_section;
        cper_section.sec_rh_cfg.mc = mc_nr;
        hm_submit_cper(ACPI_ERROR_DOMAIN_RHTLM, ACPI_ERROR_SEVERITY_INFORMATIONAL, &cper_section, sizeof(cper_section));
        rec_count++;
    }

    if (rec_count == 0)
    {
        // No telemetry found—sending dummy to signal service is up.
        // Also send if DDRSS INTU is enabled

        memset(&cper_section, 0, sizeof(cper_section));
        memset(&rh_cper_section, 0, sizeof(rh_cper_section));

        rh_cper_section.mc = start_mc;
        rh_cper_section.tlm_sample_type = RHTLM_SAMPLE_SERVICE;
        cper_section.sec_rh_cfg = rh_cper_section;

        FPFW_DBGPRINT_INFO("[rh_svc] No available RH CFG at the moment !\n");
        FPFW_DBGPRINT_INFO("[rh_svc] Submiting a dummy one as heartbeat !\n");

        hm_submit_cper(ACPI_ERROR_DOMAIN_RHTLM, ACPI_ERROR_SEVERITY_INFORMATIONAL, &cper_section, sizeof(cper_section));
    }
}

/*----------------------------- Global Functions ----------------------------*/
void rhtlm_print_service_status()
{
    if (rhtlm_context == NULL)
    {
        return;
    }

    FPFW_DBGPRINT_INFO("[rh_svc] Status %s for die nr : %d and activation period %d \n",
                       rhtlm_context->active ? "Active" : "Disabled",
                       rhtlm_context->die_id,
                       rhtlm_context->sleep_period);
}
