//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_remote_die.c
 * Implements the necessary APIs to interact with the remote die
 */

/*------------- Includes -----------------*/
#include "pid_resource.h"
#include "power_events.h"
#include "power_i.h"
#include "power_loops_i.h"
#include "power_remote_die_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <atu_api.h>
#include <bug_check.h>
#include <cortex_m7_atomics.h>
#include <debug.h>
#include <fpfw_icc_base.h>
#include <icc_mhu.h>
#include <icc_platform_defines.h>
#include <idsw_kng.h>
#include <inttypes.h>
#include <kng_icc_shared.h>
#include <kng_scp_tfa_shared.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
// bit values for send/recv status
#define SEND_STARTED  (1)
#define RECV_COMPLETE (2)
#define SEND_COMPLETE (4)
#define READY_STATUS  (SEND_STARTED | SEND_COMPLETE | RECV_COMPLETE)

// define the max size of the mailbox message in words
#define D2D_MHU_MIN_MESG_SIZE_BYTES (sizeof(icc_mhu_packet_t))
#define D2D_MHU_MAX_MESG_SIZE_BYTES (512)
#define D2D_MBOX_MAX_WORDS          (D2D_FIFO_MBOX_MAX_MESG_SIZE_BYTES / sizeof(uint32_t))
#define D2D_MHU_MIN_WORDS           (D2D_MHU_MIN_MESG_SIZE_BYTES / sizeof(uint32_t))
#define D2D_MHU_MAX_WORDS           (D2D_MHU_MAX_MESG_SIZE_BYTES / sizeof(uint32_t))

/*------------- Typedefs -----------------*/
// power_d2d_context_t: context structure for D2D communication
typedef struct _power_d2d_context
{
    uint32_t send_recv_status;                       //! flag or status governing the control loop for sync
    uint32_t d2d_send_mhu_buffer[D2D_MHU_MAX_WORDS]; //! Buffer for MHU send packet
    uint32_t d2d_recv_mhu_buffer[D2D_MHU_MAX_WORDS]; //! Buffer for MHU receive packet
    fpfw_icc_base_send_req_t d2d_send_params;
    fpfw_icc_base_recv_req_t d2d_recv_params;
    power_ctrl_loop_signal_t ctrl_loop_signal;
    uintptr_t d2d_send_arsm_payload;                           //!< data to be sent to the remote die
    uintptr_t d2d_recv_arsm_payload;                           //!< data to be received from the remote die
    uint8_t event_data_buffer[D2D_PWR_MESG_MAX_EXCHANGE_SIZE]; //!< local buffer for storing the arsm payload
    uint32_t transaction_counter; //!< counter for tracking transaction sequence numbers
} power_d2d_context_t;

// context structure for remote die interface
typedef struct _power_remote_die_context
{
    power_d2d_context_t ex_inputs;
    power_d2d_context_t ex_complete;
    power_runconfig_t* p_runconfig;
} power_remote_die_context_t;

/*-------- Function Prototypes -----------*/
static void setup_recv_request(power_d2d_context_t* p_d2d_ctx);
static void power_remote_die_clear_previous_send(power_d2d_context_t* p_d2d_ctx);
static void send_complete_signal(power_d2d_context_t* p_d2d_ctx);

/*-- Declarations (Statics and globals) --*/
static power_remote_die_context_t s_power_remote_die_ctx = {0};

/*------------- Functions ----------------*/
//! APIs added as test seams to inject addr for arsm shared memory
power_d2d_arsm_data_t* get_arsm_mem_to_write(void* d2d_ctx)
{
    FPFW_RUNTIME_ASSERT(d2d_ctx != NULL);
    power_d2d_context_t* p_d2d_ctx = (power_d2d_context_t*)d2d_ctx;
    return (power_d2d_arsm_data_t*)p_d2d_ctx->d2d_send_arsm_payload;
}

power_d2d_arsm_data_t* get_arsm_mem_to_read(void* d2d_ctx)
{
    FPFW_RUNTIME_ASSERT(d2d_ctx != NULL);
    power_d2d_context_t* p_d2d_ctx = (power_d2d_context_t*)d2d_ctx;
    return (power_d2d_arsm_data_t*)p_d2d_ctx->d2d_recv_arsm_payload;
}

void clear_sync_flag_in_arsm_send(power_d2d_context_t* p_d2d_ctx)
{
    power_d2d_arsm_data_t* p_arsm_data = get_arsm_mem_to_write(p_d2d_ctx);
    p_arsm_data->d2d_pwr_data_sync = false;
    cortex_m7_atomic_call_data_synchronization_barrier();
}

void clear_sync_flag_in_arsm_recv(power_d2d_context_t* p_d2d_ctx)
{
    power_d2d_arsm_data_t* p_arsm_data = get_arsm_mem_to_read(p_d2d_ctx);
    p_arsm_data->d2d_pwr_data_sync = false;
    cortex_m7_atomic_call_data_synchronization_barrier();
}

static void mark_and_send_if_ready(power_d2d_context_t* p_d2d_ctx, uint32_t type)
{
    // expect we're not already in progress; if we are, we still leave this function in that state
    if ((cortex_m7_atomic_or(&p_d2d_ctx->send_recv_status, type) & READY_STATUS) == READY_STATUS)
    {
        send_complete_signal(p_d2d_ctx);
    }
}

// callback for icc send completion
static void power_remote_die_icc_base_send_complete_notify(void* context, fpfw_status_t status)
{
    POWER_LOG_TRACE("power_remote_die_icc_base_send_complete_notify\n");
    FPFW_RUNTIME_ASSERT(context != NULL);
    power_d2d_context_t* p_d2d_ctx = (power_d2d_context_t*)context;

    // check/send if we're ready to send signal to control loop
    mark_and_send_if_ready(p_d2d_ctx, SEND_COMPLETE);

    if (status != DFWK_SUCCESS)
    {
        //! Clear the sync flag on arsm region to indicate data was not sent
        clear_sync_flag_in_arsm_send(p_d2d_ctx);
        // if we're failing, just log the status -- other side not receiving will cause an error there
        POWER_ET_STATUS_PARAM(POWER_ET_D2D_SEND_FAIL_CB, status);
    }
}

// callback for icc receive completion
static void power_remote_die_icc_base_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    POWER_LOG_TRACE("power_remote_die_icc_base_recv_complete_notify\n");
    FPFW_UNUSED(output_size_bytes);

    FPFW_RUNTIME_ASSERT(context != NULL);
    power_d2d_context_t* p_d2d_ctx = (power_d2d_context_t*)context;

    if (status != DFWK_SUCCESS)
    {
        // if we're failing, just log the status -- other side not receiving will cause an error there
        POWER_ET_STATUS_PARAM(POWER_ET_D2D_RECV_FAIL_CB, status);
    }

    //! Fetch data from arsm shared memory
    power_d2d_arsm_data_t* p_arsm_data = get_arsm_mem_to_read(p_d2d_ctx);
    bool sync_flag_before = p_arsm_data->d2d_pwr_data_sync;
    //! Ensure sync flag is set before and after copying; discard if it changes to false
    if (sync_flag_before)
    {
        //! Log transaction count from received data
        POWER_ET_LOG_TRACE_PARAM(POWER_ET_D2D_RECV_TRANSACTION_COUNT, p_arsm_data->transaction_count);

        //! Clear local buffer and copy data from shared memory
        memset(p_d2d_ctx->event_data_buffer, 0, sizeof(p_d2d_ctx->event_data_buffer));
        memcpy(p_d2d_ctx->event_data_buffer, (uint8_t*)p_arsm_data->pwr_data_addr, p_arsm_data->pwr_data_size);

        //! Verify that sync flag is still set
        if (p_arsm_data->d2d_pwr_data_sync)
        {
            //! Post the signal that recv is complete
            mark_and_send_if_ready(p_d2d_ctx, RECV_COMPLETE);
            //! Clear the sync flag to indicate data has been consumed
            clear_sync_flag_in_arsm_recv(p_d2d_ctx);
        }
        else
        {
            //! Unlikely! If sync flag changed to false while copying, sender has reset it, data is corrupted, do not post signal
            POWER_ET_STATUS_PARAM(POWER_ET_D2D_REMOTE_DATA_CORRUPTED, p_d2d_ctx->d2d_recv_params.recv_cmd_code);
        }
    }

    //! setup next callback everytime
    setup_recv_request(p_d2d_ctx);
}

// use to setup a recv request; need to always have one outstanding
static void setup_recv_request(power_d2d_context_t* p_d2d_ctx)
{
    // prepare the request, only header, no payload
    p_d2d_ctx->d2d_recv_params.payload_buffer = p_d2d_ctx->d2d_recv_mhu_buffer;
    p_d2d_ctx->d2d_recv_params.buffer_size = sizeof(p_d2d_ctx->d2d_recv_mhu_buffer);
    p_d2d_ctx->d2d_recv_params.cb = power_remote_die_icc_base_recv_complete_notify;
    p_d2d_ctx->d2d_recv_params.cb_ctx = p_d2d_ctx;

    // receive the payload
    fpfw_status_t status =
        fpfw_icc_base_recv(s_power_remote_die_ctx.p_runconfig->p_sconfig->icc_d2d_ctx, &p_d2d_ctx->d2d_recv_params);

    // if we fail to setup receive callback, we're in an uncorrectable state
    BUG_ASSERT_PARAM(status == DFWK_SUCCESS, status, 0);
}

// this API is called when we're ready to indicate to control loop that exchange is complete
static void send_complete_signal(power_d2d_context_t* p_d2d_ctx)
{
    power_loops_control_handle_event(p_d2d_ctx->ctrl_loop_signal, p_d2d_ctx->event_data_buffer);

    // if we complete sending of signal, ensure status is cleared
    uint32_t expected = 0;
    uint32_t actual = READY_STATUS;
    while (expected != actual)
    {
        // looping to be safe; only expect one interation (except for single die)
        expected = actual;
        actual = cortex_m7_atomic_compare_exchange(&p_d2d_ctx->send_recv_status, 0, expected);
    }
}

// common function used for both input and complete exchange types
static void power_remote_die_exchange(power_runconfig_t* p_runconfig, power_d2d_context_t* p_d2d_ctx, uint32_t cmd_code)
{
    POWER_LOG_TRACE("power_remote_die_exchange\n");
    if (p_runconfig->p_sconfig->icc_d2d_ctx == NULL)
    {
        // single die, there is nothing to do
        send_complete_signal(p_d2d_ctx);
        return;
    }
    //! Get ref to the control loop context to get hold of the latest updated data by the control loop
    power_ctrl_loop_detail_t* ctrl_loop_ctx = get_s_ctrl_loop();

    //! Get the arsm shared memory to write to
    power_d2d_arsm_data_t* p_arsm_data = get_arsm_mem_to_write(p_d2d_ctx);

    //! Check if arsm memory to write to is free to use, i.e. sync flag, this is cleared in recv cb of remote die
    if (p_arsm_data->d2d_pwr_data_sync)
    {
        //! previous data not yet consumed by remote die, debug log
        POWER_ET_LOG_TRACE_STATUS(POWER_ET_D2D_PREV_DATA_NOT_CONSUMED);
        return;
    }

    //! if local die sent previously, but haven't received from remote die yet ie local die's recv cb not
    //! raised, clear status so we can send again.
    power_remote_die_clear_previous_send(p_d2d_ctx);

    // if we've only started to send but haven't gotten a callback, we're in an unexpected state
    if ((p_d2d_ctx->send_recv_status & SEND_STARTED) != 0)
    {
        // unexpected state; just log and return
        POWER_ET_STATUS_PARAM(POWER_ET_D2D_UNEXPECTED_STATE, p_d2d_ctx->send_recv_status);
        return;
    }

    //! Prepare data to send as per command code
    if (cmd_code == ICC_COMMAND_PWR_D2D_EX_INPUT_MSG)
    {
        POWER_LOG_TRACE("ICC_COMMAND_PWR_D2D_EX_INPUT_MSG\n");
        //! clean out the entire arsm region for the die & cmd including sync flag
        memset(p_arsm_data, 0, D2D_PWR_MESG_INPUT_EXCHANGE_SIZE);
        //! Populate the arsm metadata
        p_arsm_data->pwr_data_size = sizeof(power_d2d_data_ex_input_t);
        //! Populate the power event data to be sent to the remote die
        power_d2d_data_ex_input_t* p_input_data = (power_d2d_data_ex_input_t*)p_arsm_data->pwr_data_addr;
        //! Populate the power cap if current die is die 0
        p_input_data->vrcpu_cap_die0 = 0;
        if (idsw_get_die_id() == DIE_0)
        {
            p_input_data->vrcpu_cap_die0 =
                FPFW_MIN(get_current_soc_power_cap(), p_runconfig->derived.soc_maximum_thermal_watts_limit);
            POWER_LOG_TRACE("VRCPU cap = %d\n", p_input_data->vrcpu_cap_die0);
        }
        //! Populate with the latest local power calculated, priority histogram, boost priority histogram & max resources from control loop
        memcpy((uint8_t*)&p_input_data->remote_data_snapshot, (uint8_t*)&ctrl_loop_ctx->local, sizeof(power_remote_data_t));
    }
    else // cmd_code == ICC_COMMAND_PWR_D2D_EX_COMPLETE_MSG
    {
        POWER_LOG_TRACE("ICC_COMMAND_PWR_D2D_EX_COMPLETE_MSG\n");
        //! clean out the entire arsm region for the die & cmd
        memset(p_arsm_data, 0, D2D_PWR_MESG_COMPLETE_EXCHANGE_SIZE);
        //! Populate the arsm metadata
        p_arsm_data->pwr_data_size = sizeof(power_d2d_data_ex_complete_t);
        //! Populate the power event data to be sent to the remote die
        power_d2d_data_ex_complete_t* p_complete_data = (power_d2d_data_ex_complete_t*)p_arsm_data->pwr_data_addr;
        //! Populate the pid context from the control loop
        pid_get_context(&p_complete_data->pid_context);
        //! Populate the plimit selection stats from the control loop
        memcpy((uint8_t*)&p_complete_data->plimit_stats, (uint8_t*)&ctrl_loop_ctx->plimit, sizeof(power_loop_plimit_stats_t));
        //! Populate the throttling status from the control loop
        p_complete_data->is_currently_throttling = ctrl_loop_ctx->throttling;
    }

    //! Increment transaction counter and store in ARSM for receiver to track sequence
    p_d2d_ctx->transaction_counter++;
    p_arsm_data->transaction_count = p_d2d_ctx->transaction_counter;
    POWER_ET_LOG_TRACE_PARAM(POWER_ET_D2D_SEND_TRANSACTION_COUNT, p_arsm_data->transaction_count);

    //! Set the shared memory sync flag to indicate data is ready
    p_arsm_data->d2d_pwr_data_sync = true;
    //! Add barrier here to ensure all writes to the shared memory is completed before raising icc send
    cortex_m7_atomic_call_data_synchronization_barrier();

    //! Build the icc mhu request to send, icc used for signalling only
    //! No payload only header for mhu message, We will be using the ARSM region for the payload
    //! Individual payload regions per die is already known
    icc_mhu_packet_t* p_send_req = (icc_mhu_packet_t*)p_d2d_ctx->d2d_send_mhu_buffer;
    p_send_req->header.msg_header.command = cmd_code;
    p_send_req->header.msg_header.payload_size = sizeof(icc_mhu_header_t);

    //! prepare the request
    p_d2d_ctx->d2d_send_params.payload_buffer = p_d2d_ctx->d2d_send_mhu_buffer;
    p_d2d_ctx->d2d_send_params.buffer_size = sizeof(icc_mhu_header_t);
    p_d2d_ctx->d2d_send_params.cb = power_remote_die_icc_base_send_complete_notify;
    p_d2d_ctx->d2d_send_params.cb_ctx = p_d2d_ctx;

    // send the payload, will raise an interrupt on the recv side
    fpfw_status_t status = fpfw_icc_base_send(p_runconfig->p_sconfig->icc_d2d_ctx, &p_d2d_ctx->d2d_send_params);

    if (status != DFWK_SUCCESS)
    {
        // unexpected failure, just trace; other side will go to error/recovery
        POWER_ET_STATUS_PARAM(POWER_ET_D2D_SEND_FAIL, status);
    }
    else
    {
        POWER_LOG_TRACE("power_remote_die_exchange send complete\n");
        // update status and send completion to control loop if ready
        mark_and_send_if_ready(p_d2d_ctx, SEND_STARTED);
    }
}

static void power_remote_die_clear_previous_send(power_d2d_context_t* p_d2d_ctx)
{
    // clear matched send/completes
    cortex_m7_atomic_compare_exchange(&p_d2d_ctx->send_recv_status, 0, (SEND_STARTED | SEND_COMPLETE));
}

// initialization function for remote die
void power_remote_die_init(power_runconfig_t* p_runconfig)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    // setup the ctrl loop signals for our two exchange types
    s_power_remote_die_ctx.ex_inputs.ctrl_loop_signal = POWER_CTRL_LOOP_SIGNAL_EXCHANGE_INPUTS;
    s_power_remote_die_ctx.ex_complete.ctrl_loop_signal = POWER_CTRL_LOOP_SIGNAL_EXCHANGE_COMPLETE;

    s_power_remote_die_ctx.ex_inputs.d2d_recv_params.recv_cmd_code = ICC_COMMAND_PWR_D2D_EX_INPUT_MSG;
    s_power_remote_die_ctx.ex_complete.d2d_recv_params.recv_cmd_code = ICC_COMMAND_PWR_D2D_EX_COMPLETE_MSG;

    // save off runconfig for future recv request setup
    s_power_remote_die_ctx.p_runconfig = p_runconfig;

    // if we're single die, there is no need to setup receive requests
    if (p_runconfig->p_sconfig->icc_d2d_ctx == NULL)
    {
        return;
    }

    //! Verify size checks before assigning the payload addresses
    FPFW_RUNTIME_ASSERT(D1_ARSM_MSCP_D2D_POWER_DATA_END > D2D_PWR_MESG_COMPLETE_EXCHANGE_END_D1); //! No exceeding the allocation
    //! Check that size of each individual chunck of arsm is = or > than metadata + payload
    FPFW_RUNTIME_ASSERT(sizeof(power_d2d_arsm_data_t) + sizeof(power_d2d_data_ex_input_t) <= D2D_PWR_MESG_INPUT_EXCHANGE_SIZE);
    FPFW_RUNTIME_ASSERT(sizeof(power_d2d_arsm_data_t) + sizeof(power_d2d_data_ex_complete_t) <=
                        D2D_PWR_MESG_COMPLETE_EXCHANGE_SIZE);

    //! Populate the send & recv payload buffers with the base address of the ARSM region for die 2 die exchange
    if (p_runconfig->p_sconfig->is_primary_die)
    {
        //! Current die is die 0
        //! Die 0 writes to this addr for die 1
        s_power_remote_die_ctx.ex_inputs.d2d_send_arsm_payload =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D2D_PWR_MESG_INPUT_EXCHANGE_BASE_D0);
        //! Die 1 writes to this addr for die 0
        s_power_remote_die_ctx.ex_inputs.d2d_recv_arsm_payload =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D2D_PWR_MESG_INPUT_EXCHANGE_BASE_D1);
        //! Die 0 writes to this addr for die 1
        s_power_remote_die_ctx.ex_complete.d2d_send_arsm_payload =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D2D_PWR_MESG_COMPLETE_EXCHANGE_BASE_D0);
        //! Die 1 writes to this addr for die 0
        s_power_remote_die_ctx.ex_complete.d2d_recv_arsm_payload =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D2D_PWR_MESG_COMPLETE_EXCHANGE_BASE_D1);
    }
    else
    {
        //! Current die is die 1
        //! Die 1 writes to this addr for die 0
        s_power_remote_die_ctx.ex_inputs.d2d_send_arsm_payload =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D2D_PWR_MESG_INPUT_EXCHANGE_BASE_D1);
        //! Die 0 writes to this addr for die 1
        s_power_remote_die_ctx.ex_inputs.d2d_recv_arsm_payload =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D2D_PWR_MESG_INPUT_EXCHANGE_BASE_D0);
        //! Die 1 writes to this addr for die 0
        s_power_remote_die_ctx.ex_complete.d2d_send_arsm_payload =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D2D_PWR_MESG_COMPLETE_EXCHANGE_BASE_D1);
        //! Die 0 writes to this addr for die 1
        s_power_remote_die_ctx.ex_complete.d2d_recv_arsm_payload =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D2D_PWR_MESG_COMPLETE_EXCHANGE_BASE_D0);
    }

    // Print all the d2d arsm payload addresses for debugging purposes
    POWER_LOG_INFO("ex_inputs d2d_send_arsm_payload: 0x%" PRIxPTR "\n", s_power_remote_die_ctx.ex_inputs.d2d_send_arsm_payload);
    POWER_LOG_INFO("ex_inputs d2d_recv_arsm_payload:  0x%" PRIxPTR "\n", s_power_remote_die_ctx.ex_inputs.d2d_recv_arsm_payload);
    POWER_LOG_INFO("ex_complete d2d_send_arsm_payload:  0x%" PRIxPTR "\n",
                   s_power_remote_die_ctx.ex_complete.d2d_send_arsm_payload);
    POWER_LOG_INFO("ex_complete d2d_recv_arsm_payload:  0x%" PRIxPTR "\n",
                   s_power_remote_die_ctx.ex_complete.d2d_recv_arsm_payload);

    //! Explicitly ensure all sync flags are cleared at init, each die clears it's sync flag in it's local arsm send payload region (which is remote's recv )
    clear_sync_flag_in_arsm_send(&s_power_remote_die_ctx.ex_inputs);
    clear_sync_flag_in_arsm_send(&s_power_remote_die_ctx.ex_complete);

    // setup the receive requests
    setup_recv_request(&s_power_remote_die_ctx.ex_inputs);
    setup_recv_request(&s_power_remote_die_ctx.ex_complete);
}

// use to clear status at beginning of idle to ensure we don't get out of sync
void power_remote_die_idle_reset()
{
    // clear unexpected RECV_COMPLETE status
    cortex_m7_atomic_compare_exchange(&s_power_remote_die_ctx.ex_inputs.send_recv_status, 0, RECV_COMPLETE);
    cortex_m7_atomic_compare_exchange(&s_power_remote_die_ctx.ex_complete.send_recv_status, 0, RECV_COMPLETE);

    // clear unexpected completed SEND status
    power_remote_die_clear_previous_send(&s_power_remote_die_ctx.ex_inputs);
    power_remote_die_clear_previous_send(&s_power_remote_die_ctx.ex_complete);
}

void power_remote_die_error_reset(int last_state)
{
    //! If retries exhausted during either of the d2d exchanges, clear the sync flag in arsm send region to enable sending again
    switch (last_state)
    {
    case POWER_CONTROL_STATE_COLLECT_INPUTS:
        clear_sync_flag_in_arsm_send(&s_power_remote_die_ctx.ex_inputs);
        break;
    case POWER_CONTROL_STATE_EXCHANGE_INPUTS:
        clear_sync_flag_in_arsm_send(&s_power_remote_die_ctx.ex_complete);
        break;
    default:
        // No action needed for other states
        break;
    }
}

void power_remote_die_exchange_inputs(power_runconfig_t* p_runconfig)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    // common function, pass exchange context and expected completion signal
    power_remote_die_exchange(p_runconfig, &s_power_remote_die_ctx.ex_inputs, ICC_COMMAND_PWR_D2D_EX_INPUT_MSG);
}

void power_remote_die_exchange_complete(power_runconfig_t* p_runconfig)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    // common function, pass exchange context and expected completion signal
    power_remote_die_exchange(p_runconfig, &s_power_remote_die_ctx.ex_complete, ICC_COMMAND_PWR_D2D_EX_COMPLETE_MSG);
}
