//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager.c
 * This file contains the implementation for the DDR Manager
 */

/*------------- Includes -----------------*/
#include "ddr_manager.h"   // for (anonymous struct)::(anonymous), ddr_serv...
#include "ddr_manager_i.h" // for ddr_create_bdat, ddr_create_memory_map

#include <ErrorHandler.h> // for FPFwErrorRaise
#include <bug_check.h>
#include <crash_dump.h>
#include <ddr_manager_dfwk.h>
#include <ddr_manager_events.h> // for DDR_MANAGER_ET_FATAL, DDR_MANAGER_ET_TYPE_CURVE_H...
#include <ddrss.h>              // for ddr_manager_init
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h>        // for fpfw_icc_base_ctx_t
#include <gtimer_prodfw.h>        // for gtimer_prodfw_get_counter
#include <hsp_firmware_headers.h> // for mbox command codes
#include <ift_fw.h>               // for ift_is_enabled
#include <stdint.h>               // for uint32_t
#include <stdio.h>                // for printf
#include <system_info.h>
#include <tx_api.h> // for TX_SUCCESS, ULONG, TX_AUTO_ACTIVATE, TX_A...

// Tell cspell to ignore .. why are we using cSpell?
/* cSpell:ignore DIMM DIMMS */

/*-- Symbolic Constant Macros (defines) --*/
#define DDR_TIMER_INITIAL_TICKS ((TX_TIMER_TICKS_PER_SECOND * 10))

/*-------------- Typedefs ----------------*/

/*-------- Function Prototypes -----------*/
static void crash_dump_predump_cb(void* ctx);
static void hsp_send_ddr_init_notify(fpfw_icc_base_ctx_t* icc_ctx);

/*-- Declarations (Statics and globals) --*/
static uint32_t ddrss_interrupt_id[6] = {0, 1, 2, 3, 4, 5};
static void enable_i3c_dimm_polling_timer(void);
ddr_service_context_t* ddr_service_ctx;
ddr_service_config_t* ddr_service_config;
fpfw_icc_base_ctx_t* ddr_icc_ctx = NULL;
fpfw_icc_base_send_recv_req_t icc_params;

/*-------------- Functions ---------------*/
ddr_service_context_t* ddr_get_service_context(void)
{
    return ddr_service_ctx;
}

ddr_service_config_t* ddr_get_service_config(void)
{
    return ddr_service_config;
}

void ddr_worker_thread_func(ULONG pddr_service_ctx)
{
    ddr_service_context_t* service_ctx = (ddr_service_context_t*)pddr_service_ctx;
    uint32_t received_message;
    UINT status;

    if (ift_is_enabled())
    {
        return;
    }

    while (1)
    {
        status = tx_queue_receive(&service_ctx->work_queue, &received_message, TX_WAIT_FOREVER);
        if (status != TX_SUCCESS)
        {
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_TX_QUEUE_RECEIVE, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }
        else
        {
            // Process received message
            DDR_MANAGER_ET_DEBUG_PARAM(DDR_MANAGER_ET_TYPE_Q_MESSAGE_RECEIVED, (int)received_message);

            switch ((DDR_MANAGER_MESSAGE_TYPE)received_message)
            {
            case DDR_CREATE_BDAT_EVENT:
                ddr_create_bdat();
                break;

            case DDR_CREATE_SMBIOS_TABLES_EVENT:
                ddr_create_smbios_tables();
                break;

            case DDR_START_POLLING_TIMER_EVENT:
                if (config_get_ddrmanager_bwl_polling_en())
                {
                    DDR_LOG_INFO("Enabling DDR BWL polling timer\n");
                    enable_i3c_dimm_polling_timer();
                }

                // Begin the HW Bug workaround for ECC CE polling
                if (config_get_ras_init_en())
                {
                    DDR_LOG_CRIT("Enabling ECC CE polling timer\n");
                    enable_ecc_ce_polling_timer();
                }

                if ((config_get_rh_tlm_service_period_ms() != 0) && (config_get_erhm_en() != 0))
                {
                    DDR_LOG_INFO("Enabling Row Hammer cfg TLM polling timer\n");
                    enable_row_hammer_tlm_cfg_polling_timer();
                    rhtlm_cca_mode_filtering();
                }
                break;

            case DDR_COPY_PRM_ADDR_TRANS_CONFIG_EVENT:
                // Copy the PRM address translation config to the reserved region
                ddr_publish_prm_addr_trans_cfg();
                break;

            case DDR_POLL_DIMMS_I3C_TEMPERATURE_AND_POWER_EVENT:
                ddr_read_dimm_temperatures();
                check_dimm_temp_thresholds();
                ddr_read_dimm_power();
                ddr_telemetry_report();
                break;

            case DDR_POLL_DIMMS_I3C_POWER_EVENT:
                ddr_read_dimm_power();
                ddr_telemetry_report();
                break;

            case DDR_POLL_ECC_CE_EVENT:
                ddr_poll_ecc_ce_errors();
                break;

            case DDR_RH_CFG_TLM_SERVICE_EVENT:
                rhtlm_cfg_scan();
                break;

            default:
                DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_UNKNOWN_MESSAGE_TYPE, (int)received_message);
                break;
            }
        }
    }
}

void enable_i3c_dimm_polling_timer(void)
{
    idsw_plat_id_t plat_id = idsw_get_platform_sdv();
    ULONG reschedule_ticks = ((TX_TIMER_TICKS_PER_SECOND * config_get_ddrmanager_bwl_polling_period_ms() + 999) / 1000UL);

    if ((plat_id == PLATFORM_RVP_EVT_SILICON) && (reschedule_ticks != 0))
    {
        ddr_service_context_t* pddr_service_ctx = ddr_get_service_context();

        int status = tx_timer_create((TX_TIMER*)&pddr_service_ctx->ddr_i3c_polling_timer, /* TX_TIMER *timer_ptr */
                                     (char*)DDR_I3C_TIMER_NAME,                           /* CHAR *name_ptr */
                                     ddr_timer_cb,            /* VOID (*expiration_function)(ULONG input) */
                                     (ULONG)pddr_service_ctx, /* ULONG expiration_input */
                                     (ULONG)DDR_TIMER_INITIAL_TICKS, /* ULONG initial_ticks >= 1 */
                                     reschedule_ticks,               /* ULONG reschedule_ticks */
                                     TX_AUTO_ACTIVATE);              /* UINT auto_activate) */

        if (status != TX_SUCCESS)
        {
            DDR_LOG_INFO("DDR Polling Timer creation failed with status: %d", status);
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_TIMER_CREATE_ERROR, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }
    }
}

void enable_ecc_ce_polling_timer(void)
{
    idsw_plat_id_t plat_id = idsw_get_platform_sdv();
    ULONG reschedule_ticks =
        ((TX_TIMER_TICKS_PER_SECOND * config_get_ddrmanager_ecc_ce_polling_timer_ms() + 999) / 1000UL);

    if ((plat_id == PLATFORM_RVP_EVT_SILICON) && (reschedule_ticks != 0))
    {
        ddr_service_context_t* pddr_service_ctx = ddr_get_service_context();

        int status = tx_timer_create((TX_TIMER*)&pddr_service_ctx->ecc_ce_polling_timer, /* TX_TIMER *timer_ptr */
                                     (char*)DDR_ECC_CE_TIMER_NAME,                       /* CHAR *name_ptr */
                                     ecc_ce_timer_cb,         /* VOID (*expiration_function)(ULONG input) */
                                     (ULONG)pddr_service_ctx, /* ULONG expiration_input */
                                     (ULONG)DDR_TIMER_INITIAL_TICKS, /* ULONG initial_ticks >= 1 */
                                     reschedule_ticks,               /* ULONG reschedule_ticks */
                                     TX_AUTO_ACTIVATE);              /* UINT auto_activate) */

        if (status != TX_SUCCESS)
        {
            DDR_LOG_INFO("ECC CE Polling Timer creation failed with status: %d", status);
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_TIMER_CREATE_ERROR, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }
    }
}

void enable_row_hammer_tlm_cfg_polling_timer(void)
{
    idsw_plat_id_t plat_id = idsw_get_platform_sdv();
    // Send the RH cfg once per day.
    // timer reference clock is 100Mhz (10ns)
    // (24 * 3600 * 1000000000ULL / 10); // 24 hours, so set Knob name="rh_tlm_service_period_ms" type="uint64_t" default="8640000000"

    ULONG reschedule_ticks = ((TX_TIMER_TICKS_PER_SECOND * config_get_rh_tlm_service_period_ms() + 999) / 1000UL);

    if ((plat_id == PLATFORM_RVP_EVT_SILICON) && (reschedule_ticks != 0))
    {
        ddr_service_context_t* pddr_service_ctx = ddr_get_service_context();

        int status = tx_timer_create((TX_TIMER*)&pddr_service_ctx->rh_tlm_polling_timer, /* TX_TIMER *timer_ptr */
                                     (char*)DDR_RH_TLM_TIMER_NAME,                       /* CHAR *name_ptr */
                                     rh_tlm_timer_cb,         /* VOID (*expiration_function)(ULONG input) */
                                     (ULONG)pddr_service_ctx, /* ULONG expiration_input */
                                     reschedule_ticks,        /* ULONG initial_ticks >= 1 */
                                     reschedule_ticks,        /* ULONG reschedule_ticks */
                                     TX_AUTO_ACTIVATE);       /* UINT auto_activate) */

        if (status != TX_SUCCESS)
        {
            DDR_LOG_INFO("RH TLM Polling Timer creation failed with status: %d", status);
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_TIMER_CREATE_ERROR, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }
    }
}

// Timer CB for DIMM temperature sensors & PMIC power register polling
void ddr_timer_cb(ULONG pddr_service_ctx)
{
    static int every_other = 0;
    uint32_t ddr_msg = DDR_POLL_DIMMS_I3C_POWER_EVENT;

    ddr_service_context_t* ddr_service_ctx = (ddr_service_context_t*)pddr_service_ctx;

    if (every_other)
    {
        // Poll temperatures every other interval
        ddr_msg = DDR_POLL_DIMMS_I3C_TEMPERATURE_AND_POWER_EVENT;
        tx_queue_send(&ddr_service_ctx->work_queue, &ddr_msg, TX_NO_WAIT);
    }
    else
    {
        // Poll DIMM power every interval
        ddr_msg = DDR_POLL_DIMMS_I3C_POWER_EVENT;
        tx_queue_send(&ddr_service_ctx->work_queue, &ddr_msg, TX_NO_WAIT);
    }

    every_other ^= 1;
}

// Timer CB for ECC CE polling
void ecc_ce_timer_cb(ULONG pddr_service_ctx)
{
    ddr_service_context_t* ddr_service_ctx = (ddr_service_context_t*)pddr_service_ctx;
    uint32_t msg_ecc_ce_polling = DDR_POLL_ECC_CE_EVENT;

    tx_queue_send(&ddr_service_ctx->work_queue, &msg_ecc_ce_polling, TX_NO_WAIT);
}

void rh_tlm_timer_cb(ULONG pddr_service_ctx)
{
    ddr_service_context_t* ddr_service_ctx = (ddr_service_context_t*)pddr_service_ctx;
    uint32_t msg_rh_tlm_service = DDR_RH_CFG_TLM_SERVICE_EVENT;

    tx_queue_send(&ddr_service_ctx->work_queue, &msg_rh_tlm_service, TX_NO_WAIT);
}

static void hsp_send_ddr_init_notify(fpfw_icc_base_ctx_t* icc_ctx)
{
    size_t recv_msg_size_bytes = 0;
    kng_hsp_mailbox_msg msg = {
        .header.cmd = HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY,
    };

    if ((system_info_is_hsp_present() && !system_info_is_warm_start()))
    {
        printf("Sending DDR init done notify to HSP...\n");
        //! Send the message to HSP & get response, blocking call
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SENDING_DDR_MESSAGE_TO_HSP);

        fpfw_status_t icc_status =
            fpfw_icc_base_send_recv_sync(icc_ctx, &msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);

        //! Verify sync return status & response message
        BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
        BUG_ASSERT(recv_msg_size_bytes > 0);
        BUG_ASSERT(msg.header.cmd == HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY_RSP);
        BUG_ASSERT(msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_DDR_MESSAGE_TO_HSP_SENT);
    }
    else
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_NO_HSP_DETECTED, ET_NOPARAM);
    }
}

/**
 * Initialization API for DDR Manager
 *    assigns the core buffer completion callback.
 *    This function is meant to be called from initialization. Not thread safe.
 *
 *    @param[in]  pddr_service_ctx
 *        ThreadX objects for TX_THREAD, TX_QUEUE, TX_TIMER
 *
 *    @param[in] pconfig
 *        Configuration. Gives initial parameters to each of the ThreadX objects
 *
 *   @param[in] icc_ctx
 *       ICC context for sending DDR init notify to HSP
 *
 *    @retval
 *        None
 */
void ddr_manager_init(ddr_service_context_t* pddr_service_ctx, ddr_service_config_t* pconfig, fpfw_icc_base_ctx_t* icc_ctx)
{
    // Copy to globals so that we can access in other places
    ddr_service_ctx = pddr_service_ctx;
    ddr_service_config = pconfig;
    KNG_DIE_ID die_id = idsw_get_die_id();

    uint32_t work_queue_msg = 0;
    int status = tx_queue_create((TX_QUEUE*)&pddr_service_ctx->work_queue, /* TX_QUEUE *queue_ptr */
                                 (char*)DDR_WORK_QUEUE_NAME,               /* CHAR *name_ptr */
                                 pconfig->queue_config.msg_size, /* UINT message_size in 32-bit word */
                                 pconfig->queue_config.p_queue,  /* VOID *queue_start */
                                 sizeof(uint32_t) * pconfig->queue_config.queue_num_words); /* ULONG queue_size in bytes */

    if (status != TX_SUCCESS)
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_QUEUE_CREATE_ERROR, status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    if (!system_info_is_warm_start())
    {
        work_queue_msg = DDR_CREATE_BDAT_EVENT;
        status = tx_queue_send((TX_QUEUE*)&pddr_service_ctx->work_queue, &work_queue_msg, TX_NO_WAIT);

        if (status != TX_SUCCESS)
        {
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_CREATE_BDAT, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }

        work_queue_msg = DDR_CREATE_SMBIOS_TABLES_EVENT;
        status = tx_queue_send((TX_QUEUE*)&pddr_service_ctx->work_queue, &work_queue_msg, TX_NO_WAIT);
        if (status != TX_SUCCESS)
        {
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_CREATE_SMBIOS_TABLES, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }

        work_queue_msg = DDR_COPY_PRM_ADDR_TRANS_CONFIG_EVENT;
        status = tx_queue_send((TX_QUEUE*)&pddr_service_ctx->work_queue, &work_queue_msg, TX_NO_WAIT);
        if (status != TX_SUCCESS)
        {
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_COPY_PRM_CONFIG, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }
    }
    else
    {
        DEBUG_PRINT("DDR warm start, skipping tables/memory map creation\n");
    }

    // I3C polling timer and ECC CE polling timer need to be started on both cold/warm reset path.
    work_queue_msg = DDR_START_POLLING_TIMER_EVENT;
    status = tx_queue_send((TX_QUEUE*)&pddr_service_ctx->work_queue, &work_queue_msg, TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_START_POLLING_TIMER, status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    status = tx_thread_create((TX_THREAD*)&pddr_service_ctx->work_thread, /* TX_THREAD *thread_ptr */
                              (char*)DDR_WORK_THREAD_NAME,                /* CHAR *name_ptr */
                              ddr_worker_thread_func,                     /* VOID (*entry_function)(ULONG) */
                              (ULONG)pddr_service_ctx,                    /* ULONG entry_input */
                              pconfig->thread_config.p_stack,             /* VOID *stack_start */
                              pconfig->thread_config.stack_size,          /* ULONG stack_size */
                              pconfig->thread_config.priority,            /* UINT priority */
                              pconfig->thread_config.priority,            /* UINT preempt_threshold */
                              pconfig->thread_config.time_slice_option,   /* ULONG time_slice */
                              TX_AUTO_START);                             /* UINT auto_start */

    if (status != TX_SUCCESS)
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_THREAD_CREATE_ERROR, status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    ddr_manager_i3c_init();

    DDR_LOG_CRIT("DDR init, die_num: [%u]\n", die_id);
    // Record start timestamp
    uint64_t start_timestamp = gtimer_prodfw_get_counter();
    prod_ddrss_lib_init(die_id);

    // Calculate DDRSS init duration in milliseconds
    uint64_t ddr_training_time_ms =
        ((gtimer_prodfw_get_counter() - start_timestamp) * 1000) / gtimer_prodfw_get_frequency();
    DDR_LOG_CRIT("DDR training time: %llu ms\n", ddr_training_time_ms);
    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_DDR_TRAINING_TIME_MS, (int)ddr_training_time_ms);

    // PPR will write updated SDL back to flash if it has run during prod_ddrss_lib_init
    // load the SDL from flash to DDR reserved region
    if (config_get_ddrmanager_sdl_en() == true)
    {
        DDR_LOG_INFO("Loading SDL from flash to DDR\n");
        ddr_load_shared_defect_list();

        DDR_LOG_INFO("Publishing SDL DDR location to UEFI variable\n");
        ddr_publish_sdl_addr();
    }
    else
    {
        DDR_LOG_INFO("SDL support disabled by config knob\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_SKIPPED_CFG_KNOB_DISABLED);
    }

    ddr_create_memory_map(); // Includes publishing the memory map to SDS
    hsp_send_ddr_init_notify(icc_ctx);

    ddr_init_telemetry();

    // Add crash dump pre-dump callback to check DDR RAS for UE
    crash_dump_register_pre_dump_callback(crash_dump_predump_cb, NULL, CRASH_DUMP_TYPE_FULL);

    // Calculate DDR init duration in milliseconds
    uint64_t duration_ms = ((gtimer_prodfw_get_counter() - start_timestamp) * 1000) / gtimer_prodfw_get_frequency();

    DDR_LOG_CRIT("DDR init, die_num: [%u] Done\n", die_id);
    DDR_LOG_CRIT("DDR init duration: %llu ms\n", duration_ms);
    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_DDR_INIT_DURATION_MS, (int)duration_ms);
}

static void crash_dump_predump_cb(void* ctx)
{
    FPFW_UNUSED(ctx);

    // Check for any uncorrectable errors in the DDR RAS handler
    for (int ddrss = 0; ddrss < NUM_DIMM_PER_DIE; ddrss++)
    {
        if (prod_ddrss_interrupt_pending((void*)&ddrss_interrupt_id[ddrss]))
        {
            prod_ddrss_interrupt_handler((void*)&ddrss_interrupt_id[ddrss]);
        }
    }
}