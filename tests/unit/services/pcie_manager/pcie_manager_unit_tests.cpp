//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_manager_unit_tests.cpp
 *       Unit tests for the SCP PCIe manager service.
 */

/*------------- Includes -----------------*/
extern "C" {
#include <DfwkPtrTypes.h>
#include <error_handler.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_manager_i.h>
#include <scp_pcie_manager.h>
#include <silibs_status.h>
#include <tx_api.h>
}

#include <CMockaWrapper.h>
#include <cstddef>
#include <cstdint>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pciess_device_t dev;
static pciess_device_interface_t iface;
static pcie_manager_context_t ctx = {.rpss_idx = RPSS0, .dev = &dev, .iface = &iface};

/*------------- Functions ----------------*/
TEST_FUNCTION(init_fail, NULL, NULL)
{
    auto* sched = (PDFWK_SCHEDULE)0xdeadbeef;

    // Bad args
    expect_value(FPFwErrorRaise, error, (uint32_t)(-EINVAL));
    if (!set_error_handler_return())
    {
        scp_pcie_initialize(nullptr);
    }

    // tx queue fails
    expect_value(__wrap_pcie_dfwk_init, schedule, sched);
    expect_any_always(__wrap__txe_queue_create, queue_ptr);
    expect_any_always(__wrap__txe_queue_create, name_ptr);
    expect_any_always(__wrap__txe_queue_create, message_size);
    expect_any_always(__wrap__txe_queue_create, queue_start);
    expect_any_always(__wrap__txe_queue_create, queue_size);
    expect_any_always(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_NO_MEMORY);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NO_MEMORY);
    if (!set_error_handler_return())
    {
        scp_pcie_initialize(sched);
    }

    // tx thread fails
    expect_value(__wrap_pcie_dfwk_init, schedule, sched);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    expect_any_always(__wrap__txe_thread_create, thread_ptr);
    expect_any_always(__wrap__txe_thread_create, name_ptr);
    expect_any_always(__wrap__txe_thread_create, entry_function);
    expect_any_always(__wrap__txe_thread_create, entry_input);
    expect_any_always(__wrap__txe_thread_create, stack_start);
    expect_any_always(__wrap__txe_thread_create, stack_size);
    expect_any_always(__wrap__txe_thread_create, priority);
    expect_any_always(__wrap__txe_thread_create, preempt_threshold);
    expect_any_always(__wrap__txe_thread_create, time_slice);
    expect_any_always(__wrap__txe_thread_create, auto_start);
    expect_any_always(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_NOT_DONE);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NOT_DONE);
    if (!set_error_handler_return())
    {
        scp_pcie_initialize(sched);
    }
}

/* Test case to validate async requests sent for each root port */
TEST_FUNCTION(test_link_training_req, NULL, NULL)
{
    for (auto& i : ctx.async_req)
    {
        expect_value(__wrap_DfwkAsyncRequestInititalize, Request, &i);
        expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, &i);
        expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, rpss_req_completion_cb);
        expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, &ctx);
        expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &(iface));
        expect_value(__wrap_DfwkInterfaceSendAsync, Request, &i);
    }

    send_start_link_training_requests(&ctx);
}

/* Tests to validate completion request handling */
TEST_FUNCTION(test_completion_callback, NULL, NULL)
{
    auto& async_req = ctx.async_req[0];
    async_req.status = SILIBS_E_TIMEOUT;

    /* Setup worker queue expectations */
    expect_value(__wrap__txe_queue_send, queue_ptr, &(ctx.work_queue));
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    rpss_req_completion_cb(&(ctx.async_req[0].header), &ctx);
}

/* Tests to validate completion request handling */
TEST_FUNCTION(test_completion_callback_queue_full, NULL, NULL)
{
    auto& async_req = ctx.async_req[0];
    async_req.rp_index = 2;
    async_req.status = SILIBS_E_TIMEOUT;

    /* Setup worker queue to fail */
    expect_value(__wrap__txe_queue_send, queue_ptr, &(ctx.work_queue));
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_QUEUE_FULL);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_FULL);
    if (!set_error_handler_return())
    {
        rpss_req_completion_cb(&(ctx.async_req[0].header), &ctx);
    }
}
