//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utc_sync_client_init.c
 * Instantiates the UTC Sync Client service.
 */

/*-------------------------------- Includes ---------------------------------*/

#include <FpFwUtils.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <gtimer_prodfw.h>
#include <tx_api.h>
#include <tx_port.h>
#include <utc_sync_client_service.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

#define UTC_CLIENT_STACK_SIZE ((TX_MINIMUM_STACK) + ((2) * (FPFW_KB)))

#define UTC_CLIENT_UPDATE_PERIOD_MS (1000U)

#define UTC_CLIENT_THREAD_PRIORITY (15u)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static uint8_t s_utc_client_stack[UTC_CLIENT_STACK_SIZE];

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

FPFW_INIT_COMPONENT(utc_client_svc, FPFW_INIT_DEPENDENCIES("gtimer", "mts_svc"))
{
    utc_sync_client_config_t config = {
        .thread_config =
            {
                .p_stack = s_utc_client_stack,
                .stack_size = sizeof(s_utc_client_stack),
                .priority = UTC_CLIENT_THREAD_PRIORITY,
                .time_slice_option = TX_NO_TIME_SLICE,
            },
        .get_current_system_count = gtimer_prodfw_get_counter,
        .update_period_ms = UTC_CLIENT_UPDATE_PERIOD_MS,
        .system_counter_freq_hz = gtimer_prodfw_get_frequency(),
    };

    fpfw_status_t sc = utc_sync_client_init(&config);

    if (FPFW_STATUS_FAILED(sc))
    {
        sc = FPFW_INIT_STATUS_E_POINTER;
    }

    return (fpfw_init_result_t){sc, NULL};
}