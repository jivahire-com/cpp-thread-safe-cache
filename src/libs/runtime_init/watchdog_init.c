//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file watchdog_init.c
 * Initialization of the watchdog service.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h> // for FPFW_DBGPRINT_INFO
#include <bug_check.h>
#include <fpfw_cfg_mgr.h> // for knobs
#include <fpfw_init.h>    // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <idhw.h>         // for idhw_get_cpu_type, idhw_get_die_id
#include <idsw.h>         // for idsw_get_cpu_type, idsw_get_die_id
#include <idsw_kng.h>     // for DIE_0, DIE_1
#include <kng_error.h>    // for KNG_SUCCESS
#include <stddef.h>       // for NULL
#include <wdog.h>         // for wdog_service_init

/*-- Symbolic Constant Macros (defines) --*/
#ifndef MHz
    #define MHz 1000000
#endif
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(wdog, FPFW_INIT_DEPENDENCIES("hw_ver", "cfg_mgr"))
{
    if (config_get_wdog_en() && idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON)
    {
        const uint32_t wdog_timeout_s = config_get_wdog_timeout_s();
        const uint32_t wdog_freq_hz = config_get_wdog_counter_freq_Mhz() * MHz;

        KNG_STATUS status = wdog_service_init(wdog_timeout_s, wdog_freq_hz);
        BUG_ASSERT_PARAM(status == KNG_SUCCESS, status, 0);

        FPFW_DBGPRINT_INFO("Wdog init with timeout %d s and freq %d Hz\n", wdog_timeout_s, wdog_freq_hz);
    }
    else
    {
        FPFW_DBGPRINT_INFO("Wdog disabled\n");
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
