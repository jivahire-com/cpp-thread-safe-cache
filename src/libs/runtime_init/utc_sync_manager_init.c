//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utc_sync_manager_init.c
 * Instantiates UTC Sync Manager Service
 */

/*------------- Includes -----------------*/

#include <FpFwUtils.h>
#include <fpfw_init.h>
#include <gtimer_prodfw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <tx_api.h>
#include <utc_sync_manager_lib.h>
#include <utc_sync_manager_service.h>

/*-- Symbolic Constant Macros (defines) --*/

// clang-format off
#define SECONDS_TO_MS(s) ((s) * 1000)
#define MINUTES_TO_MS(m) (SECONDS_TO_MS((m) * 60))
#define UTC_SYNC_MANAGER_UPDATE_PERIOD_MS SECONDS_TO_MS(5)

#define UTC_SYNC_MANAGER_STACK_SIZE ((TX_MINIMUM_STACK) + ((1) * (FPFW_KB)))
#define UTC_SYNC_MANAGER_THREAD_PRI (15)
// clang-format on

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t s_utc_sync_manager_stack[UTC_SYNC_MANAGER_STACK_SIZE];

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(utc_mngr_svc_mcp, FPFW_INIT_DEPENDENCIES("mts_svc", "gtimer", "mctp"))
{

    fpfw_mctp* p_mctp_ctx = (fpfw_mctp*)fpfw_init_get_handle("mctp");

    // If MCTP is not initialized, skip UTC Sync Manager initialization
    if (p_mctp_ctx == NULL)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    // Only Die 0 has the MCTP connection to the BMC
    if (idsw_get_die_id() == DIE_0)
    {
        uint32_t frequency_hz = gtimer_prodfw_get_frequency();

        utc_sync_manager_config_t config = {.thread_config = {.p_stack = s_utc_sync_manager_stack,
                                                              .stack_size = sizeof(s_utc_sync_manager_stack),
                                                              .priority = UTC_SYNC_MANAGER_THREAD_PRI,
                                                              .time_slice_option = TX_NO_TIME_SLICE},
                                            .time_provider_cb = utc_sync_manager_request_utc_timestamp,
                                            .get_current_system_count = gtimer_prodfw_get_counter,
                                            .update_period_ms = UTC_SYNC_MANAGER_UPDATE_PERIOD_MS,
                                            .system_counter_freq_hz = frequency_hz};

        // Initialize the MCTP component before the UTC Manager
        utc_sync_manager_mctp_init(p_mctp_ctx);

        // Initialize the UTC Manager
        fpfw_status_t sc = utc_sync_manager_init(&config);

        if (FPFW_STATUS_FAILED(sc))
        {
            return (fpfw_init_result_t){sc, NULL};
        }
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
