//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_remote_die.c
 * Implements the necessary APIs to interact with the remote die
 */

/*------------- Includes -----------------*/
#include "power_events.h"
#include "power_i.h"
#include "power_loops_i.h"
#include "power_remote_die_i.h"
#include "power_stub_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <cortex_m7_atomics.h>
#include <debug.h>
#include <fpfw_icc_base.h>
#include <icc_mhu.h>
#include <icc_platform_defines.h>
#include <inttypes.h>
#include <kng_icc_shared.h>
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
    uint32_t send_recv_status;
    uint32_t d2d_send_payload_buffer[D2D_MHU_MAX_WORDS];
    uint32_t d2d_recv_payload_buffer[D2D_MHU_MAX_WORDS];
    fpfw_icc_base_send_req_t d2d_send_params;
    fpfw_icc_base_recv_req_t d2d_recv_params;
    power_ctrl_loop_signal_t ctrl_loop_signal;
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
    // update status and send completion to control loop if ready
    mark_and_send_if_ready(p_d2d_ctx, RECV_COMPLETE);

    // setup next callback
    setup_recv_request(p_d2d_ctx);
}

// use to setup a recv request; need to always have one outstanding
static void setup_recv_request(power_d2d_context_t* p_d2d_ctx)
{
    // prepare the request
    p_d2d_ctx->d2d_recv_params.payload_buffer = p_d2d_ctx->d2d_recv_payload_buffer;
    p_d2d_ctx->d2d_recv_params.buffer_size = sizeof(p_d2d_ctx->d2d_recv_payload_buffer);
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
    // TODO: pass data with signal
    //       https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1914730/
    power_loops_control_handle_event(p_d2d_ctx->ctrl_loop_signal, NULL);

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
    if (p_runconfig->p_sconfig->icc_d2d_ctx == NULL)
    {
        // single die, there is nothing to do
        send_complete_signal(p_d2d_ctx);
        return;
    }

    // if we sent previously, but don't have a received signal, clear so we can send again.
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
        //! Data to send are as follows:
        //! 1. Current power cap, die 1 consumes die 0 value, power_cap_get_vrcpu_cap
        //! 2. Snapshot of power_remote_data_t for local core
        //! 3. Throttle priority histogram, corebits_t throttle_priority[VM_THROT_COUNT]
        //! 4. Boost priority histogram, corebits_t boost_priority[VM_THROT_COUNT]
        //! 5. Resource count (assumes all cores can go to highest P state) - limit to min P state that any core supports
    }
    else // cmd_code == ICC_COMMAND_PWR_D2D_EX_COMPLETE_MSG
    {
        POWER_LOG_TRACE("ICC_COMMAND_PWR_D2D_EX_COMPLETE_MSG\n");
        //! Data to send are as follows:
        //! 1. Minimally send pid_context from pid_get_context(pid_context_t *context) ..
        //!    (on mismatch die1 will update to die0 value w/ pid_set_context AND both sides will log to power trace)
        //! 2. Could additionally send plimit selections, throttling bool, for comparison
    }

    // Build the request to send
    icc_mhu_packet_t* p_send_req = (icc_mhu_packet_t*)p_d2d_ctx->d2d_send_payload_buffer;
    p_send_req->header.msg_header.command = cmd_code;
    p_send_req->header.msg_header.payload_size = sizeof(p_d2d_ctx->d2d_send_payload_buffer) - sizeof(icc_mhu_header_t);

    // prepare the request
    p_d2d_ctx->d2d_send_params.payload_buffer = p_d2d_ctx->d2d_send_payload_buffer;
    p_d2d_ctx->d2d_send_params.buffer_size = sizeof(p_d2d_ctx->d2d_send_payload_buffer);
    p_d2d_ctx->d2d_send_params.cb = power_remote_die_icc_base_send_complete_notify;
    p_d2d_ctx->d2d_send_params.cb_ctx = p_d2d_ctx;

    // send the payload
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
