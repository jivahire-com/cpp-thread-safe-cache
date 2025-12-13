//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utc_sync_client_init.c
 * Instantiates the UTC Sync Client service.
 */

/*-------------------------------- Includes ---------------------------------*/

#include <FpFwUtils.h>
#include <bug_check.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <gtimer_prodfw.h>
#include <hsp_firmware_headers.h>
#include <mscp_exp_rmss_memory_map.h>
#include <tx_api.h>
#include <tx_port.h>
#include <utc_sync_client_service.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

#define UTC_CLIENT_STACK_SIZE ((TX_MINIMUM_STACK) + ((2) * (FPFW_KB)))

#define UTC_CLIENT_UPDATE_PERIOD_MS (1000U)

#define UTC_CLIENT_THREAD_PRIORITY (15u)

/*-------------------------------- Typedefs ---------------------------------*/

//
// This is a duplicate of the HSP UTC Payload that is not currently shared:
// HSP Repo: https://azurecsi.visualstudio.com/Woodinville/_git/kingsgate.hsp?path=/sprt/src/telemetry_manager/telemetry_manager.h&version=GBmain&_a=contents&line=81&lineStyle=plain&lineEnd=95&lineStartColumn=1&lineEndColumn=3
//
typedef struct
{
    /* utc time value address  high 32 bits */
    uint32_t utc_high;

    /* utc time value address  low 32 bits */
    uint32_t utc_low;

    /* Timer value address high 32 bits
       Timer value is used for calculating the diff from mailbox fire to receive.*/
    uint32_t timer_high;

    /* Timer value address low 32 bits */
    uint32_t timer_low;
} hsp_utc_payload_t;

// Check that we have enough storage allocated for the UTC Timestamp Exchange
static_assert(sizeof(hsp_utc_payload_t) <= SCP_EXP_MCP_HSP_UTC_TIMESTAMP_EXCHANGE_SIZE,
              "hsp_utc_payload_t size too large for shared memory region");
static_assert(sizeof(utc_timestamp_bundle_t) == sizeof(hsp_utc_payload_t),
              "utc_timestamp_bundle_t not the same size as hsp_utc_payload_t");

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static uint8_t s_utc_client_stack[UTC_CLIENT_STACK_SIZE];

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

#ifdef MCP_RUNTIME_INIT
static void hsp_utc_timestamp_cb(utc_timestamp_bundle_t* p_utc_timestamp, void* ctx)
{
    BUG_ASSERT(p_utc_timestamp != NULL);
    BUG_ASSERT(ctx != NULL);

    // Send the UTC Timestamp Bundle to the HSP via ICC HSP Mailbox
    fpfw_icc_base_ctx_t* icc_hspmbx_ctx = (fpfw_icc_base_ctx_t*)ctx;

    // Update the payload in shared memory
    hsp_utc_payload_t* payload = (hsp_utc_payload_t*)SCP_EXP_MCP_HSP_UTC_TIMESTAMP_EXCHANGE_BASE;
    payload->utc_low = (uint32_t)(p_utc_timestamp->timestamp & 0xFFFFFFFF);
    payload->utc_high = (uint32_t)((p_utc_timestamp->timestamp >> 32) & 0xFFFFFFFF);
    payload->timer_low = (uint32_t)(p_utc_timestamp->system_counter & 0xFFFFFFFF);
    payload->timer_high = (uint32_t)((p_utc_timestamp->system_counter >> 32) & 0xFFFFFFFF);

    // Build the HSP Message
    kng_hsp_mailbox_msg utc_time_set_msg;
    utc_time_set_msg.header.cmd = HSP_MAILBOX_CMD_SET_UTC_TIME_REQ;
    utc_time_set_msg.utc_time_set_req.address_low = SCP_EXP_MCP_HSP_UTC_TIMESTAMP_EXCHANGE_BASE;
    utc_time_set_msg.utc_time_set_req.address_high = 0;

    // Send the HSP Message
    // HSP ICC Base Context on MSCP does not have a retry limit, so this will block until successful
    fpfw_status_t icc_base_status =
        fpfw_icc_base_send_sync(icc_hspmbx_ctx, &utc_time_set_msg, sizeof(utc_time_set_msg));

    BUG_ASSERT_PARAM(icc_base_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_base_status, 0);
}
#endif

/*----------------------------- Global Functions ----------------------------*/

//
// The UTC Client on the MCP also sends the UTC Timestamp Bundle to the HSP
//

#ifdef MCP_RUNTIME_INIT
FPFW_INIT_COMPONENT(utc_client_svc, FPFW_INIT_DEPENDENCIES("gtimer_stg_2", "icc_hspmbx", "mts_svc"))
#else
FPFW_INIT_COMPONENT(utc_client_svc, FPFW_INIT_DEPENDENCIES("gtimer_stg_2", "mts_svc"))
#endif
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
#ifdef MCP_RUNTIME_INIT
        .platform_handler =
            {
                .cb = hsp_utc_timestamp_cb,
                .ctx = fpfw_init_get_handle("icc_hspmbx"),
            },
#endif
    };

    fpfw_status_t sc = utc_sync_client_init(&config);

    if (FPFW_STATUS_FAILED(sc))
    {
        sc = FPFW_INIT_STATUS_E_POINTER;
    }

    return (fpfw_init_result_t){sc, NULL};
}

#ifdef MCP_RUNTIME_INIT
    #include <utc_cli_service.h>
FPFW_INIT_COMPONENT(utc_cli, FPFW_INIT_DEPENDENCIES("utc_client_svc"))
{
    utc_cli_svc_initialize();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
#endif
