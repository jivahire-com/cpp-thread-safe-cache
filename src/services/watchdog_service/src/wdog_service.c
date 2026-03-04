//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file wdog_service.c
 * Watchdog service implementation.
 */

/*------------- Includes -----------------*/
#include <cmsdk_wd.h>
#include <tx_api.h>
#include <wdog.h>

/*-- Symbolic Constant Macros (defines) --*/
#define WDOG_SERVICE_THREAD_STACK_SIZE (1024U)
#define WDOG_SERVICE_THREAD_PRIORITY   (TX_MAX_PRIORITIES - 1)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void wdog_heartbeat(uint32_t wdog_init_counter);

/*-- Declarations (Statics and globals) --*/
static TX_THREAD wdog_thread;
static uint8_t wdog_thread_stack[WDOG_SERVICE_THREAD_STACK_SIZE] = {0};
static ULONG g_wdog_period_ticks = 0;

/*------------- Functions ----------------*/
static void wdog_service_thread_entry(ULONG thread_input)
{
    const uint32_t wdog_init_counter = (uint32_t)thread_input;

    while (true)
    {
        wdog_heartbeat(wdog_init_counter);
    }
}

void wdog_heartbeat(uint32_t wdog_init_counter)
{
    static bool is_wdog_initialized = false;

    if (!is_wdog_initialized)
    {
        wdog_cmsdk_apb_init(wdog_init_counter, true);
        is_wdog_initialized = true;
    }
    else
    {
        wdog_cmsdk_apb_reload();
    }

    tx_thread_sleep(g_wdog_period_ticks);
}

KNG_STATUS wdog_service_init(uint32_t timeout_in_s, uint32_t wdog_freq)
{
    if (timeout_in_s == 0 || wdog_freq == 0)
    {
        return KNG_E_INVALIDARG;
    }

    const uint32_t wdog_init_counter = timeout_in_s * wdog_freq;
    const uint32_t wdog_margin = wdog_init_counter * 2;
    const ULONG wdog_thread_input = (ULONG)(wdog_init_counter + wdog_margin);
    g_wdog_period_ticks = (ULONG)(timeout_in_s * TX_TIMER_TICKS_PER_SECOND);

    UINT ret = tx_thread_create(&wdog_thread,
                                "wdog_thread",
                                wdog_service_thread_entry,
                                wdog_thread_input,
                                (void*)wdog_thread_stack,
                                sizeof(wdog_thread_stack),
                                WDOG_SERVICE_THREAD_PRIORITY,
                                WDOG_SERVICE_THREAD_PRIORITY,
                                TX_NO_TIME_SLICE,
                                TX_AUTO_START);

    return ret == TX_SUCCESS ? KNG_SUCCESS : KNG_E_FAIL;
}