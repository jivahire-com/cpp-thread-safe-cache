//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file rtosmon_service.c
 *
 * This file defines the APIs for integrating with RTOSMon as well
 * as event trace thread and helper functions to support RTOSMon functionality.
 *
 */

/*-------------------------------- Includes ---------------------------------*/

#include <FpFwUtils.h>       // for FPFW_UNUSED - depends on both above
#include <cmsis_m7.h>        // for SysTick
#include <fpfw_init.h>       // for FPFW_INIT_STATUS_SUCCESS
#include <fpfw_status.h>     // for fpfw_status_t
#include <gtimer_prodfw.h>   // for including tx_api.h before rtosmon.h
#include <idsw.h>            // for idsw_get_cpu_type
#include <idsw_kng.h>        // for CPU_CDED_SDM, CPU_SDM
#include <inttypes.h>        // for PRIu32, PRIu64
#include <rtosmon.h>         // for rtosmon_init
#include <rtosmon_events.h>  // for event traces
#include <rtosmon_service.h> // rtosmon_service_init
#include <stddef.h>          // for NULL
#include <stdio.h>           // for printf
#include <string.h>          // for memcpy, strnlen
#include <systick_update.h>  // for systick_get_emcpu_clock

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// Transmit observations every 30 seconds
#define RTOSMON_THREAD_OBS_INTERVAL (TX_TIMER_TICKS_PER_SECOND * 30)

#define RTOSMON_SERVICE_THREAD_STACK_SIZE (2048)
#define RTOSMON_SERVICE_THREAD_PRIORITY   (10U)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// Thread Configuration (optional to specify memory location)
static TX_THREAD s_rtosmon_service_thread;
static uint8_t s_rtosmon_service_thread_stack[RTOSMON_SERVICE_THREAD_STACK_SIZE];

// RTOSMon Configuration and Data (optional to specify memory location)
static rtosmon_config_t s_rtosmon_config;
static rtosmon_histo_data_t s_rtosmon_data;
static rtosmon_snapshot_t s_snapshot_buffer_a;
static rtosmon_snapshot_t s_snapshot_buffer_b;
static rtosmon_report_t s_et_report;

/*--------------------------------- Externs ---------------------------------*/

// Threadx execution time source variables - defined in tx_el.h included via tx_api.h
#ifdef _WIN32
extern uint64_t (*_tx_execution_time_source)(void);
#endif
extern uint64_t _tx_execution_max_time_source;
extern uint64_t _tx_execution_time_source_freq;

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

/**
 * @brief Initialize the rtosmon library with configuration
 *
 * @return fpfw_status_t Status of initialization
 */

fpfw_status_t rtosmon_lib_init()
{
    // Setup the core name and type based on CPU and die
    idsw_cpu_type_t cpu_type = idsw_get_cpu_type();
    KNG_DIE_ID die_id = idsw_get_die_id();
    if (die_id == 0)
    {
        if (cpu_type == CPU_SCP)
        {
            s_rtosmon_config.core_name = "SCP0";
        }
        else if (cpu_type == CPU_MCP)
        {
            s_rtosmon_config.core_name = "MCP0";
        }
    }
    else if (die_id == 1)
    {
        if (cpu_type == CPU_SCP)
        {
            s_rtosmon_config.core_name = "SCP1";
        }
        else if (cpu_type == CPU_MCP)
        {
            s_rtosmon_config.core_name = "MCP1";
        }
    }
    else
    {
        s_rtosmon_config.core_name = "UnknownCore";
    }
    s_rtosmon_config.core_type = "Cortex-M7";

    // Set time sources and their frequencies for execution profiling
    s_rtosmon_config.tx_execution_time_source = gtimer_prodfw_get_counter;
    s_rtosmon_config.tx_execution_time_source_freq = gtimer_prodfw_get_frequency();
    s_rtosmon_config.tx_execution_max_time_source = UINT64_MAX;

    // Set monitoring time source and its frequency for event trace
    s_rtosmon_config.monitoring_time_source = gtimer_prodfw_get_counter;
    s_rtosmon_config.monitoring_time_source_freq = gtimer_prodfw_get_frequency();

    // Setup allocated buffers and pointer to time get function
    s_rtosmon_config.snapshot_buffer_a = &s_snapshot_buffer_a;
    s_rtosmon_config.snapshot_buffer_b = &s_snapshot_buffer_b;
    s_rtosmon_config.report_buffer = &s_et_report;
    s_rtosmon_config.time_get = tx_time_get;

    // Setup report intervals
    s_rtosmon_config.report_interval_ticks = RTOSMON_THREAD_OBS_INTERVAL;
    s_rtosmon_config.report_flush_delay_ticks = 0;

    // Setup a thread worker function that is called for every rtosmon report event
    s_rtosmon_config.thread_hook = NULL;
    ;

    return rtosmon_init(&s_rtosmon_config, &s_rtosmon_data);
}

fpfw_status_t rtosmon_service_init()
{
    fpfw_status_t status = rtosmon_lib_init();
    if (status != FPFW_INIT_STATUS_SUCCESS)
    {
        return status;
    }

    UINT tx_status = tx_thread_create(&s_rtosmon_service_thread,
                                      "RTOSMON_EVT_THREAD",
                                      rtosmon_event_thread,
                                      (uint32_t)&s_rtosmon_config,
                                      s_rtosmon_service_thread_stack,
                                      RTOSMON_SERVICE_THREAD_STACK_SIZE,
                                      RTOSMON_SERVICE_THREAD_PRIORITY,
                                      RTOSMON_SERVICE_THREAD_PRIORITY,
                                      TX_NO_TIME_SLICE,
                                      TX_AUTO_START);
    if (tx_status != TX_SUCCESS)
    {
        status = FPFW_STATUS_FAIL;
    }
    return status;
}
