//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file rtosmon_config.c
 * @brief RTOSMon configuration and initialization for Cortex-M7
 * This file configures and initializes RTOSMon plugin for the SCP core.
 *
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <cmsis_m7.h>
#include <fpfw_status.h> // for fpfw_status_t, FPFW_STATUS_SUCCESS
#include <idsw.h>
#include <idsw_kng.h>
#include <rtosmon.h> // for rtosmon_init
#include <rtosmon_config.h>
#include <string.h>         // for strnlen, memcpy
#include <systick_update.h> // for systick_get_reload_val
#include <tx_api.h>         // for TX_MINIMUM_STACK, TX_NO_TIME_S...
#include <tx_execution_profile.h>

/*-- Symbolic Constant Macros (defines) --*/

// Currently enabling only 6 threads and keeping stack to 10 KB
#define RTOSMON_STACK_SIZE      (10 * 1024U)
#define RTOSMON_THREAD_PRIORITY (10U)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t s_rtosmon_stack[RTOSMON_STACK_SIZE];
static rtosmon_data_t s_rtosmon_data;

/*------------- Functions ----------------*/
/*
 * Local portable implementation of strcpy_s for environments (e.g. arm-none-eabi/newlib)
 * that do not provide C11 Annex K bounds-checking interfaces.
 * Semantics (subset):
 *  - On success, copies source including NUL terminator into dest and returns 0.
 *  - On any error (NULL pointers, zero destsz, source length >= destsz), writes a NUL
 *    to dest[0] if possible and returns non-zero error code.
 * Error codes chosen to match common errno values (EINVAL=22, ERANGE=34) but we avoid
 * including <errno.h> to keep footprint small.
 */
#ifndef _WIN32
/**
 * @Brief: Bounds-checked string copy for environments without `strcpy_s`.
 * @param[in]: `dest` destination buffer
 * @param[in] `destsz` size of destination
 * @param[in] `src` source string
 * @param[out]: `dest` receives a NUL-terminated copy on success; `dest[0]='\0'` on error
 */
int strcpy_s(char* dest, size_t destsz, const char* src)
{
    if (dest == NULL || src == NULL)
    {
        if (dest != NULL && destsz > 0)
        {
            dest[0] = '\0';
        }
        return FPFW_STATUS_INVALID_ARGS;
    }

    if (destsz == 0)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }

    size_t src_len = strnlen(src, destsz);
    /* If strnlen reached destsz, no terminator within bounds -> range error */
    if (src_len >= destsz)
    {
        dest[0] = '\0';
        return FPFW_STATUS_OUT_OF_RANGE;
    }

    memcpy(dest, src, src_len + 1); /* include terminator */

    return FPFW_STATUS_SUCCESS;
}
#endif /* FPFW_HAS_STRCPY_S */

/**
 * @Brief: Monotonic time source based on SysTick.
 * @param[in]: None
 * @param[out]: Returns 64-bit tick count since boot
 */
static uint64_t systick_time_source(void)
{
    volatile uint32_t sys_tick_val;
    uint32_t systick_load = SysTick->LOAD;
    volatile uint64_t tx_tick;

    do
    {
        sys_tick_val = SysTick->VAL;
        tx_tick = tx_time_get();
    } while (tx_tick != tx_time_get());

    uint64_t systick = tx_tick * (systick_load + 1);
    uint64_t systick_curr = (systick_load - sys_tick_val);

    return systick + systick_curr;
}

/**
 * @Brief: Setups up the rtosmon config and initializes RTOSMon for Cortex-M7.
 * @param[in]: None
 * @param[out]: None
 */
void rtosmon_plugin_init(void)
{
    rtosmon_config_t rtosmon_config;

    if (idsw_get_cpu_type() == CPU_SCP)
    {
        rtosmon_config.core_name = "SCP";
    }
    else
    {
        rtosmon_config.core_name = "MCP";
    }

    rtosmon_config.core_type = "Cortex-M7";
    rtosmon_config.event_trace_thread_prio = RTOSMON_THREAD_PRIORITY;
    rtosmon_config.event_trace_thread_slice = TX_NO_TIME_SLICE;
    rtosmon_config.event_trace_thread_stack_buffer = s_rtosmon_stack; // Change if enabling the Event Trace thread
    rtosmon_config.event_trace_thread_stack_size = RTOSMON_STACK_SIZE; // Change if enabling the Event Trace thread
    rtosmon_config.event_trace_enable_output = true;

    rtosmon_config.monitoring_time_source = systick_time_source;
    rtosmon_config.monitoring_time_source_freq = (uint64_t)systick_get_emcpu_clock();

    rtosmon_config.tx_execution_time_source = systick_time_source;
    rtosmon_config.tx_execution_max_time_source = UINT64_MAX;
    rtosmon_config.tx_execution_time_source_freq = (uint64_t)systick_get_emcpu_clock();

    rtosmon_config.allow_event_trace_init = true;

    set_rtosmon_context_switch_work_enabled(true);
    BUG_ASSERT(rtosmon_init(&rtosmon_config, &s_rtosmon_data) == FPFW_STATUS_SUCCESS);
}
