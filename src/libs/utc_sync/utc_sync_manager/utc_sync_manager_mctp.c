//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utc_sync_manager_mctp.c
 * This file implements MCTP communication used by the UTC Sync Manager.
 */

/*-------------------------------- Includes ---------------------------------*/

#include <FpFwAssert.h>
#include <fpfw_mctp.h>
#include <fpfw_status.h>
#include <gtimer_prodfw.h>
#include <stddef.h>
#include <stdint.h>
#include <utc_common.h>
#include <utc_sync_manager_lib.h>
#include <utc_sync_manager_mctp_events_i.h>
#include <utc_sync_manager_mctp_i.h>
#include <utc_sync_manager_service.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/* Reference points used for estimating current time based on the last sync with the BMC
 * T_now = T_ref + (SysCounter_now - SysCounter_ref) / Freq_hz
 *
 * where:
 * T_now: current estimated time (ms since UNIX epoch)
 * T_ref: last synced time from BMC (ms since UNIX epoch)
 * SysCounter_now: current system counter value (ticks)
 * SysCounter_ref: system counter value at last sync with the BMC (ticks)
 * Freq_hz: frequency of the system counter (Hz)
 */
typedef struct _NTP_SYNC_CONTEXT
{
    uint64_t last_synced_unix_time_epoch_ms; // Last synced unix time in ms since epoch
    uint64_t last_synced_sys_counter;        // System counter at last sync
} ntp_sync_context_t;

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// MCTP context passed in on initialization, used when interfacing with MCTP
static fpfw_mctp* s_mctp_ctx = NULL;

// NTP Time + System Counter from the last successful sync
static ntp_sync_context_t s_ntp_sync_ctx = {0};

// System counter frequency in Hz
static uint32_t s_system_counter_frequency_hz = 0;

// We only want one MCTP read pending at a time, keep track of it here
static bool s_is_read_pending = false;

// The only thing that will change in the NTP Request to the BMC will be T0
static utc_sync_manager_mctp_message_send_t s_send_message_buffer = {
    .base_header =
        {
            .msg_type = MCTP_MVDP_MSG_TYPE,
            .pci_vendor_id = {MCTP_MVDP_PCI_VENDOR_ID_0, MCTP_MVDP_PCI_VENDOR_ID_1},
            .escape_seq1 = MCTP_MVDP_ESCAPE_SEQ_0,
            .escape_seq2 = MCTP_MVDP_ESCAPE_SEQ_1,
            .command_set = MCTP_MVDP_1P_BMC_CMD_SET_ID,
            .protocol_version = {MCTP_MVDP_1P_BMC_CMD_SET_PROTO_VER_0, MCTP_MVDP_1P_BMC_CMD_SET_PROTO_VER_1},
            .command = MCTP_MVDP_1P_BMC_CMD_SET_NTP_CMD_ID,
        },
    .t0 = 0 // Initialize timestamp to zero
};

// The NTP Request message information for the MCTP Lib
static fpfw_mctp_send_msg_info s_local_send_msg_info = {
    .dest_eid = MCTP_BMC_EID,
    .msg_tag = FPFW_MCTP_MSG_TAG_0,
    .tag_owner = true,
    .msg = (void*)(&s_send_message_buffer),
    .msg_size = sizeof(s_send_message_buffer),
    .cb = on_mctp_send_msg_complete, // Callback function
    .cb_ctx = NULL                   // Optional context for the callback
};

// The NTP Response from the BMC
static utc_sync_manager_mctp_message_receive_t s_receive_message_buffer = {0};

// The NTP Response message information for the MCTP lib
static fpfw_mctp_recv_msg_info s_local_receive_msg_info = {
    .msg = (void*)(&s_receive_message_buffer),
    .msg_buffer_size = sizeof(s_receive_message_buffer),
    .msg_type = MCTP_MVDP_MSG_TYPE,
    .cb = on_mctp_msg_receive, // Callback function
    .cb_ctx = NULL             // Optional context for the callback
};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/* Wrappers */
static uint64_t get_system_counter(void)
{
    return gtimer_prodfw_get_counter();
}

/**
 * @brief Calculates the synchronized NTP time using four timestamps.
 *
 * @param[in] t_0 Time when the UTC Sync Manager sends the request to the BMC.
 * @param[in] t_1 Time when the BMC receives the request.
 * @param[in] t_2 Time when the BMC responds to the request.
 * @param[in] t_3 Time when the UTC Sync Manager receives the response from the BMC.
 *
 * @return Synchronized NTP time in ms since UNIX epoch.
 */
static uint64_t get_synchronized_ntp_time_epoch_ms(uint64_t t_0, uint64_t t_1, uint64_t t_2, uint64_t t_3)
{
    // t_0: the time the UTC Sync Manager sends the request to the BMC
    // t_1: the time the BMC receives the request from the Manager
    // t_2: the time the BMC responds to the request from the Manager
    // t_3: the time the Manager receives the response from the BMC

    int64_t delta = (((int64_t)t_1 - (int64_t)t_0) + ((int64_t)t_2 - (int64_t)t_3)) / 2;

    // Synchronized Time = t_3 + delta
    // t_3 should not overflow until 292,277,028,567 AD
    return (uint64_t)((int64_t)t_3 + delta);
}

/**
 * @brief Converts the system counter delta to elapsed time in ms.
 *
 * @param[in] sys_counter_now   Current system counter value (ticks).
 * @param[in] sys_counter_then  Reference system counter value (ticks).
 *
 * @return Elapsed system counter time in ms.
 */
static uint64_t sys_counter_delta_to_time_epoch_ms(uint64_t sys_counter_now, uint64_t sys_counter_then)
{
    if (s_system_counter_frequency_hz == 0)
    {
        return 0; // avoid division by zero
    }

    // Calculate elapsed mS between sys_counter_then and sys_counter_now
    // We multiply by 1000 first to reduce lost precision when dividing by frequency
    return ((sys_counter_now - sys_counter_then) * 1000ULL) / s_system_counter_frequency_hz;
}

/**
 * @brief Estimates the current UNIX time in ms using: the last known time UNIX time, the system counter of
 * when that UNIX time was known, and the current system counter value.
 *
 * @param[in] time_ref          Last synced UNIX time (ms).
 * @param[in] sys_counter_now   Current system counter (ticks).
 * @param[in] sys_counter_then  System counter at last sync (ticks).
 *
 * @return Estimated UNIX time in ms.
 */
static uint64_t estimate_unix_time_epoch_ms(uint64_t time_ref, uint64_t sys_counter_now, uint64_t sys_counter_then)
{
    return time_ref + sys_counter_delta_to_time_epoch_ms(sys_counter_now, sys_counter_then);
}

/**
 * @brief Wrapper to get the current estimated UNIX time in ms.
 *
 * @return Estimated UNIX time in ms.
 */
static uint64_t get_current_unix_time_estimate_epoch_ms()
{
    return estimate_unix_time_epoch_ms(s_ntp_sync_ctx.last_synced_unix_time_epoch_ms,
                                       get_system_counter(),
                                       s_ntp_sync_ctx.last_synced_sys_counter);
}

/*----------------------------- Global Functions ----------------------------*/

void utc_sync_manager_mctp_init(fpfw_mctp* mctp_ctx)
{
    FPFW_RUNTIME_ASSERT(NULL != mctp_ctx);

    s_mctp_ctx = mctp_ctx;

    s_system_counter_frequency_hz = gtimer_prodfw_get_frequency();
}

fpfw_status_t utc_sync_manager_mctp_send(fpfw_mctp_send_msg_info* p_send_msg_info)
{
    FPFW_RUNTIME_ASSERT(NULL != p_send_msg_info);

    // Fill in T0 timestamp. T1 and T2 will be filled by the BMC
    ((utc_sync_manager_mctp_message_send_t*)(p_send_msg_info->msg))->t0 = get_current_unix_time_estimate_epoch_ms();

    fpfw_status_t status = fpfw_mctp_send_msg(s_mctp_ctx, p_send_msg_info);

    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(UtcSyncManagerMctpSendMsgFailed,
                    status,
                    (uintptr_t)p_send_msg_info,
                    (uintptr_t)p_send_msg_info->msg,
                    p_send_msg_info->msg_size,
                    p_send_msg_info->dest_eid,
                    p_send_msg_info->msg_tag,
                    p_send_msg_info->tag_owner);
    }

    return status;
}

fpfw_status_t utc_sync_manager_mctp_read(fpfw_mctp_recv_msg_info* p_recv_msg_info)
{
    FPFW_RUNTIME_ASSERT(NULL != p_recv_msg_info);

    if (s_is_read_pending == true)
    {
        FPFW_ET_LOG(UtcSyncManagerMctpRecvMsgAlreadyPending);
        return FPFW_STATUS_PENDING;
    }

    fpfw_status_t status = fpfw_mctp_recv_msg(s_mctp_ctx, p_recv_msg_info);

    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(UtcSyncManagerMctpRecvMsgFailed,
                    status,
                    (uintptr_t)p_recv_msg_info,
                    (uintptr_t)p_recv_msg_info->msg,
                    p_recv_msg_info->msg_buffer_size,
                    p_recv_msg_info->msg_type);
        return status;
    }

    s_is_read_pending = true;

    return status;
}

void on_mctp_msg_receive(fpfw_mctp_recv_msg_info* p_recv_msg_info, fpfw_status_t status)
{
    FPFW_RUNTIME_ASSERT(NULL != p_recv_msg_info);

    //
    // On an invalid recv we don't queue up another read. When another request is sent
    // a pending read will be sent to the MCTP lib.
    //
    s_is_read_pending = false;

    // Validate the MCTP Recv status
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(UtcSyncManagerMctpRecvMsgFailed,
                    status,
                    (uintptr_t)p_recv_msg_info,
                    (uintptr_t)p_recv_msg_info->msg,
                    p_recv_msg_info->msg_buffer_size,
                    p_recv_msg_info->msg_type);
        return;
    }

    utc_sync_manager_mctp_message_receive_t* p_message =
        (utc_sync_manager_mctp_message_receive_t*)p_recv_msg_info->msg;

    // Validate the Command Set, Command ID, and Completion Code from the MDVP message
    if (p_message->header.base_header.command_set != MCTP_MVDP_1P_BMC_CMD_SET_ID ||
        p_message->header.base_header.command != MCTP_MVDP_1P_BMC_CMD_SET_NTP_CMD_ID ||
        p_message->header.completion_code != MCTP_MVDP_COMPLETION_CODE_SUCCESS)
    {
        FPFW_ET_LOG(UtcSyncManagerMctpRecvMsgInvalidMsg,
                    (uintptr_t)p_recv_msg_info,
                    (uintptr_t)p_recv_msg_info->msg,
                    p_message->header.base_header.command_set,
                    p_message->header.base_header.command,
                    p_message->header.completion_code);
        return;
    }

    // Fill in T3 timestamp. T1 and T2 were filled by the BMC
    uint64_t t3 = get_current_unix_time_estimate_epoch_ms();

    utc_timestamp_bundle_t timestamp_bundle = {
        .timestamp = get_synchronized_ntp_time_epoch_ms(p_message->t0, p_message->t1, p_message->t2, t3),
        .system_counter = get_system_counter(),
    };

    // Hand off new timestamp to the UTC Sync Manager
    utc_sync_manager_notify_timestamp_ready(&timestamp_bundle);

    // update reference system counter and timestamps used for estimating current time
    s_ntp_sync_ctx.last_synced_sys_counter = timestamp_bundle.system_counter;
    s_ntp_sync_ctx.last_synced_unix_time_epoch_ms = timestamp_bundle.timestamp;
}

void on_mctp_send_msg_complete(fpfw_mctp_send_msg_info* p_send_msg_info, fpfw_status_t status)
{

    //
    // We don't kick off a new send if this one failed. The UTC Manager will trigger
    // that path based on it's synchronization cadence with the BMC.
    //

    FPFW_RUNTIME_ASSERT(NULL != p_send_msg_info);

    FPFW_ET_LOG(UtcSyncManagerMctpSendMsgComplete,
                status,
                (uintptr_t)p_send_msg_info->msg,
                p_send_msg_info->msg_size,
                p_send_msg_info->dest_eid,
                p_send_msg_info->msg_tag,
                p_send_msg_info->tag_owner);
}

fpfw_status_t utc_sync_manager_request_utc_timestamp()
{

    //
    // This is kick off by the UTC Manager Service based on it's synchronization
    // cadence with the BMC.
    //

    // Pend a receive request to be ready for the next send
    fpfw_status_t sc = utc_sync_manager_mctp_read(&s_local_receive_msg_info);

    // Log an error here. A pending status indicates we have the recv side already setup
    if (sc != FPFW_STATUS_SUCCESS && sc != FPFW_STATUS_PENDING)
    {
        FPFW_ET_LOG(UtcSyncManagerRequestMtsUtcStampFailed, sc);
        return sc;
    }

    // Only setup the send if the read setup was successful
    sc = utc_sync_manager_mctp_send(&s_local_send_msg_info);

    FPFW_ET_LOG(UtcSyncManagerMctpRequestTimestampComplete, sc);

    return sc;
}