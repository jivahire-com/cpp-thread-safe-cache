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
#include <ddr_manager_events.h> // for DDR_MANAGER_ET_FATAL, DDR_MANAGER_ET_TYPE_CURVE_H...
#include <ddrss.h>              // for ddr_manager_init
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h>        // for fpfw_icc_base_ctx_t
#include <hsp_firmware_headers.h> // for mbox command codes
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

/*-- Declarations (Statics and globals) --*/
static uint32_t ddrss_interrupt_id[6] = {0, 1, 2, 3, 4, 5};
static void enable_i3c_dimm_polling_timer(void);
ddr_service_context_t* ddr_service_ctx;
ddr_service_config_t* ddr_service_config;

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
            case DDR_CREATE_MEMORY_MAP_EVENT:
                ddr_create_memory_map();
                break;

            case DDR_CREATE_BDAT_EVENT:
                ddr_create_bdat();
                break;

            case DDR_CREATE_SMBIOS_TABLES_EVENT:
                ddr_create_smbios_tables();

                if (config_get_ddrmanager_bwl_polling_en())
                {
                    printf("Enabling DDR BWL polling timer\n");
                    enable_i3c_dimm_polling_timer();
                }
                break;

            case DDR_COPY_PRM_ADDR_TRANS_CONFIG_EVENT:
                // Copy the PRM address translation config to the reserved region
                ddr_publish_prm_addr_trans_cfg();
                break;

            case DDR_POLL_DIMMS_I3C_EVENT:
                ddr_poll_dimms();
                check_dimm_temp_thresholds();
                ddr_telemetry_report();
                break;

            case DDR_I3C_DATA_READY_EVENT:
                // This should be triggered by an async. completion notification from the DDR I3C driver / dfwk
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
    ULONG reschedule_ticks = ((TX_TIMER_TICKS_PER_SECOND * config_get_ddrmanager_bwl_polling_period_ms()) / 1000UL);

    if (plat_id == PLATFORM_RVP_EVT_SILICON)
    {
        ddr_service_context_t* pddr_service_ctx = ddr_get_service_context();

        int status = tx_timer_create((TX_TIMER*)&pddr_service_ctx->ddr_polling_timer, /* TX_TIMER *timer_ptr */
                                     (char*)DDR_TIMER_NAME,                           /* CHAR *name_ptr */
                                     ddr_timer_cb,            /* VOID (*expiration_function)(ULONG input) */
                                     (ULONG)pddr_service_ctx, /* ULONG expiration_input */
                                     (ULONG)DDR_TIMER_INITIAL_TICKS, /* ULONG initial_ticks >= 1 */
                                     reschedule_ticks,               /* ULONG reschedule_ticks */
                                     TX_AUTO_ACTIVATE);              /* UINT auto_activate) */

        if (status != TX_SUCCESS)
        {
            printf("DDR Polling Timer creation failed with status: %d\n", status);
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_TIMER_CREATE_ERROR, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }
    }
}

// Timer CB for DIMM temperature sensors & PMIC power register polling
void ddr_timer_cb(ULONG pddr_service_ctx)
{
    ddr_service_context_t* ddr_service_ctx = (ddr_service_context_t*)pddr_service_ctx;
    uint32_t msg_dimm_polling = DDR_POLL_DIMMS_I3C_EVENT;

    tx_queue_send(&ddr_service_ctx->work_queue, &msg_dimm_polling, TX_NO_WAIT);
}

static void hsp_send_ddr_init_notify(fpfw_icc_base_ctx_t* icc_ctx)
{
    size_t recv_msg_size_bytes = 0;
    kng_hsp_mailbox_msg msg = {
        .header.cmd = HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY,
    };
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
        work_queue_msg = DDR_CREATE_MEMORY_MAP_EVENT;
        status = tx_queue_send((TX_QUEUE*)&pddr_service_ctx->work_queue, &work_queue_msg, TX_NO_WAIT);
        if (status != TX_SUCCESS)
        {
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_CREATE_MMAP, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }

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
    prod_ddrss_lib_init(die_id);

    ddr_init_telemetry();
    if (system_info_is_hsp_present() && (!system_info_is_warm_start()))
    {
        hsp_send_ddr_init_notify(icc_ctx);
    }
    else
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_NO_HSP_DETECTED, ET_NOPARAM);
    }

    // Add crash dump pre-dump callback to check DDR RAS for UE
    crash_dump_register_pre_dump_callback(crash_dump_predump_cb, NULL, CRASH_DUMP_TYPE_FULL);
    DDR_LOG_CRIT("DDR init, die_num: [%u] Done\n", die_id);
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