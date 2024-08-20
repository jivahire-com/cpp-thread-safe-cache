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
#include <stdint.h>       // for uint32_t
#include <stdio.h>        // for printf
#include <tx_api.h>       // for TX_SUCCESS, ULONG, TX_AUTO_ACTIVATE, TX_A...

// Tell cspell to ignore .. why are we using cSpell?
/* cSpell:ignore DIMM DIMMS */

/*-------------- Typedefs ----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

void ddr_worker_thread_func(ULONG pddr_service_ctx)
{
    ddr_service_context_t* ddr_service_ctx = (ddr_service_context_t*)pddr_service_ctx;
    uint32_t received_message;
    UINT status;

    printf("DDR worker thread begin\n");

    while (1)
    {
        status = tx_queue_receive(&ddr_service_ctx->work_queue, &received_message, TX_WAIT_FOREVER);
        if (status != TX_SUCCESS)
        {
            printf("DDR worker: Error with tx_queue_receive\n");
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }
        else
        {
            // Process received message
            // TODO: Task #1778297: Replace printf with debug level Event Trace Event
            printf("DDR Q Message received: %d\n", (int)received_message);

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
                break;

            case DDR_POLL_DIMMS_I3C_EVENT:
                ddr_poll_dimms();
                break;

            case DDR_I3C_DATA_READY_EVENT:
                // This should be triggered by an async. completion notification from the DDR I3C driver / dfwk
                break;

            default:
                printf("Unknown message type\n");
                break;
            }
        }
    }
}

// Timer CB for DIMM temperature sensors & PMIC power register polling
void ddr_timer_cb(ULONG pddr_service_ctx)
{
    ddr_service_context_t* ddr_service_ctx = (ddr_service_context_t*)pddr_service_ctx;
    uint32_t msg_dimm_polling = DDR_POLL_DIMMS_I3C_EVENT;

    printf("Sending a message for test to DDR worker thread..\n");
    tx_queue_send(&ddr_service_ctx->work_queue, &msg_dimm_polling, TX_NO_WAIT);
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
 *    @retval
 *        None
 */
void ddr_manager_init(ddr_service_context_t* pddr_service_ctx, ddr_service_config_t* pconfig)
{
    uint32_t work_queue_msg = 0;
    int status = tx_queue_create((TX_QUEUE*)&pddr_service_ctx->work_queue, /* TX_QUEUE *queue_ptr */
                                 (char*)DDR_WORK_QUEUE_NAME,               /* CHAR *name_ptr */
                                 pconfig->queue_config.msg_size, /* UINT message_size in 32-bit word */
                                 pconfig->queue_config.p_queue,  /* VOID *queue_start */
                                 sizeof(uint32_t) * pconfig->queue_config.queue_num_words); /* ULONG queue_size in bytes */

    if (status != TX_SUCCESS)
    {
        printf("tx_queue_create err %d\n", status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    work_queue_msg = DDR_CREATE_MEMORY_MAP_EVENT;
    status = tx_queue_send((TX_QUEUE*)&pddr_service_ctx->work_queue, &work_queue_msg, TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        printf("tx_queue_send DDR_CREATE_MEMORY_MAP_EVENT err %d\n", status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    work_queue_msg = DDR_CREATE_BDAT_EVENT;
    status = tx_queue_send((TX_QUEUE*)&pddr_service_ctx->work_queue, &work_queue_msg, TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        printf("tx_queue_send DDR_CREATE_BDAT_EVENT err %d\n", status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    work_queue_msg = DDR_CREATE_SMBIOS_TABLES_EVENT;
    status = tx_queue_send((TX_QUEUE*)&pddr_service_ctx->work_queue, &work_queue_msg, TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        printf("tx_queue_send DDR_CREATE_SMBIOS_TABLES_EVENT err %d\n", status);
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
        printf("thread create err\n");
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    status = tx_timer_create((TX_TIMER*)&pddr_service_ctx->ddr_polling_timer, /* TX_TIMER *timer_ptr */
                             (char*)DDR_TIMER_NAME,                           /* CHAR *name_ptr */
                             ddr_timer_cb,            /* VOID (*expiration_function)(ULONG input) */
                             (ULONG)pddr_service_ctx, /* ULONG expiration_input */
                             pconfig->timer_config.initial_ticks,    /* ULONG initial_ticks >= 1 */
                             pconfig->timer_config.reschedule_ticks, /* ULONG reschedule_ticks */
                             TX_AUTO_ACTIVATE);                      /* UINT auto_activate) */

    if (status != TX_SUCCESS)
    {
        printf("timer create err %d\n", status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    ddr_manager_i3c_init();
    ddr_init_telemetry();
}
