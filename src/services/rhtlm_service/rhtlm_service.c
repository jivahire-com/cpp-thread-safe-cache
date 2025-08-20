//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file rhtlm_service_init.c
 * This file contains the Row Hammer Telemetry services
 */

/*-------------------------------- Includes ---------------------------------*/
// #include <drmctop_regs.h>

#include <ErrorHandler.h>
#include <FpFwCli.h>
#include <FpFwUtils.h>
#include <ddrss.h>
#include <ddrss_config.h>
#include <ddrss_runtime_api.h>
#include <health_monitor.h>
#include <idsw.h> // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <mu_public.h>
#include <rhtlm_service.h>
#include <stdint.h> // for uint8_t
#include <stdio.h>  // for printf
#include <tx_api.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
// rhtlm thread priority is the lowest possible but higher than watchdog value
#define RHTLM_THREAD_PRIORITY          (TX_MAX_PRIORITIES - 2)
#define RHTLM_THREAD_PREEMPT_THRESHOLD (RHTLM_THREAD_PRIORITY)
#define RHTLM_THREAD_NAME              "RHTLM Thread"
#define RHTLM_MILISECONDS_PER_SECOND   (1000U)
#define RHTLM_MS_TO_TX_TICKS(ms)       (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)
/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
static void rhtlm_thread_worker_function(ULONG thread_input);

/*------------------- Declarations (Statics and globals) --------------------*/
static rhtlm_service_ctx_t* rhtlm_context = NULL;
/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief  : Apply filter if production mode is active
 * @param  : void
 * @retval : void
 */
static void rhtlm_cca_mode_filtering()
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

    FpFwCliPrint("[rh_svc] CCA filtering mask: 0x%x\n", filter_mask.as_uint32);
    int result;
    result = ddrss_set_rhm_telemetry_record_mask(&filter_mask);

    if (result != SILIBS_SUCCESS)
    {
        FpFwCliPrint("[rh_svc] res %d exp %d ", result, SILIBS_SUCCESS);
    }
}

/**
 * @brief  : Scan all memory controllers for RH telemetry record
 *           if none detected , send a dummy one to act as a heartbeat
 * @param  : void
 * @retval : void
 */
static void rhtlm_telemtry_scan()
{
    FpFwCliPrint("[rh_svc] scan MC tlm\n");

    KNG_DIE_ID die_id = idsw_get_die_id();
    ddrss_rhm_tm_evt_t ddrss_rm_telemetry;
    acpi_err_sec_ddrss_rhm_tm_t rh_cper_section = {0};
    acpi_cper_section_t cper_section;

    int sub_sts = SILIBS_E_DATA;
    uint32_t start_mc = 0;
    uint32_t stop_mc = RHTLM_MC_DIE0_COUNT_NR;
    uint32_t rec_count = 0;

    if (config_get_intu_init_en() == 0) // DDRSS INTU disabled , scan for telemtry
    {
        if (die_id == DIE_1)
        {
            start_mc = RHTLM_MC_DIE0_COUNT_NR;
            stop_mc = RHTLM_MC_DIE1_COUNT_NR;
        }

        for (uint32_t mc_nr = start_mc; mc_nr < stop_mc; mc_nr++)
        {
            sub_sts = ddrss_get_telemetry_record(mc_nr, DDRSS_TELEMETRY_TYPE_RHM_EVT, &ddrss_rm_telemetry, sizeof(ddrss_rm_telemetry));
            if (sub_sts != SILIBS_SUCCESS)
            {
                continue;
            }

            memset(&cper_section, 0, sizeof(cper_section));
            memset(&rh_cper_section, 0, sizeof(rh_cper_section));
            prod_ddrss_convert_rh_rec_to_rh_cper(mc_nr, RHTLM_SAMPLE_EVENT, &ddrss_rm_telemetry, &rh_cper_section);
            cper_section.sec_rh_tlm = rh_cper_section;
            hm_submit_cper(ACPI_ERROR_DOMAIN_RHTLM, ACPI_ERROR_SEVERITY_INFORMATIONAL, &cper_section, sizeof(cper_section));
            rec_count++;
        }
    }

    if (rec_count == 0)
    {
        // No telemetry found—sending dummy to signal service is up.
        // Also send if DDRSS INTU is enabled

        memset(&cper_section, 0, sizeof(cper_section));
        memset(&rh_cper_section, 0, sizeof(rh_cper_section));

        rh_cper_section.mc = start_mc;
        rh_cper_section.valid = 1;
        rh_cper_section.tlm_sample_type = RHTLM_SAMPLE_SERVICE;
        cper_section.sec_rh_tlm = rh_cper_section;
        // prod_ddrss_convert_rh_rec_to_rh_cper();

        FpFwCliPrint("[rh_svc] No available RH TLM at the moment !\n");
        FpFwCliPrint("[rh_svc] Submiting a dummy one as heartbeat !\n");

        hm_submit_cper(ACPI_ERROR_DOMAIN_RHTLM, ACPI_ERROR_SEVERITY_INFORMATIONAL, &cper_section, sizeof(cper_section));
    }
}

/**
 * @brief  : Thread functionality function , that sleeps and scans for telemetry
 * @param  : context - pointer to thread context
 * @retval : void
 */
void rhtlm_telemtry_function(rhtlm_service_ctx_t* context)
{
    context->sleep_period = RHTLM_MS_TO_TX_TICKS(config_get_rh_tlm_service_period_ms());
    if (context->sleep_period > 0)
    {
        FpFwCliPrint("[rh_svc] sleep for %d ms.\n", context->sleep_period);
        tx_thread_sleep(context->sleep_period);
        // sleep for context->sleep_period
        rhtlm_telemtry_scan();
    }
    else
    {
        /* Delay is set to 0 ms. Assuming RHTLM service was meant to be disabled, so do a dummy sleep for 1 second . */
        tx_thread_sleep(RHTLM_MS_TO_TX_TICKS(RHTLM_MILISECONDS_PER_SECOND));
    }
}

static void rhtlm_thread_worker_function(ULONG thread_input)
{
    rhtlm_context = (rhtlm_service_ctx_t*)thread_input;
    while (true)
    {
        rhtlm_telemtry_function(rhtlm_context);
    }
}

/*----------------------------- Global Functions ----------------------------*/
void rhtlm_print_service_status()
{
    if (rhtlm_context == NULL)
    {
        return;
    }

    FpFwCliPrint("[rh_svc] Status %s for die nr : %d and activation period %d \n",
                 rhtlm_context->active ? "Active" : "Disabled",
                 rhtlm_context->die_id,
                 rhtlm_context->sleep_period);
}

void rhtlm_thread_init(rhtlm_service_ctx_t* context)
{
    UINT status;
    context->sleep_period = 0;
    context->active = false;

    FpFwCliPrint("[rh_svc] RH Logic is %s ", config_get_erhm_en() ? "enabled" : "disabled");

    if (config_get_rh_tlm_service_period_ms() == 0 || config_get_erhm_en() == 0)
    {
        // no need to create the task, as the rh tlm service is disabled or RH mitigation is disabled
        // feather disabled
        FpFwCliPrint("[rh_svc] Disabled");
        return;
    }

    status = tx_thread_create(&context->s_rhtlm_thread,
                              RHTLM_THREAD_NAME,
                              rhtlm_thread_worker_function,
                              (ULONG)(uintptr_t)context,
                              (void*)context->s_stack_pool_memory,
                              sizeof(context->s_stack_pool_memory),
                              RHTLM_THREAD_PRIORITY,
                              RHTLM_THREAD_PREEMPT_THRESHOLD,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        FpFwCliPrint("[rh_svc] Failed to create worker thread! TX_STATUS: %d\n", status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
        return;
    }

    context->sleep_period = RHTLM_MS_TO_TX_TICKS(config_get_rh_tlm_service_period_ms());
    FpFwCliPrint("[rh_svc] Created for die nr : %d and activation period %d \n", context->die_id, context->sleep_period);
    context->active = true;

    rhtlm_cca_mode_filtering();
}
