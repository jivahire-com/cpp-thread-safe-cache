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

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void wdog_timer_callback(ULONG wdog_init_counter)
{
    static bool wdog_init = false;

    if (!wdog_init)
    {
        wdog_cmsdk_apb_init(wdog_init_counter, true);
        wdog_init = true;
    }
    else
    {
        wdog_cmsdk_apb_reload();
    }
}

KNG_STATUS wdog_service_init(uint32_t timeout_in_s, uint32_t wdog_freq)
{
    if (timeout_in_s == 0 || wdog_freq == 0)
    {
        return KNG_E_INVALIDARG;
    }

    static TX_TIMER wdog_timer;

    const uint32_t wdog_init_counter = timeout_in_s * wdog_freq;
    const uint32_t wdog_margin = wdog_init_counter * 2;

    UINT ret = tx_timer_create(&wdog_timer,                              // timer_ptr
                               "wdog_timer",                             // name_ptr
                               wdog_timer_callback,                      // expiration_function
                               (ULONG)(wdog_init_counter + wdog_margin), // input
                               timeout_in_s * TX_TIMER_TICKS_PER_SECOND, // initial_ticks
                               timeout_in_s * TX_TIMER_TICKS_PER_SECOND, // reschedule_ticks
                               TX_AUTO_ACTIVATE);                        // auto_activate

    return ret == TX_SUCCESS ? KNG_SUCCESS : KNG_E_FAIL;
}