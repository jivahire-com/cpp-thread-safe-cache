//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_manager_unit_tests.cpp
 *       Unit tests for the SCP PCIe manager service.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <DfwkPtrTypes.h>      // for PDFWK_SCHEDULE
#include <errno.h>             // for EINVAL
#include <error_handler.h>     // for set_error_handler_return
#include <kng_soc_constants.h> // for RPSS0
#include <pcie_dfwk.h>         // for pciess_device_interface_t, pciess_dev...
#include <pcie_manager_i.h>    // for rpss_req_completion_cb, send_start_li...
#include <scp_pcie_manager.h>  // for scp_pcie_initialize, pcie_manager_con...
#include <silibs_kng_soc.h>
#include <silibs_status.h> // for SILIBS_E_TIMEOUT
#include <tx_api.h>        // for TX_NOT_DONE, TX_NO_MEMORY, TX_NO_WAIT
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pciess_device_t dev;
static pciess_device_interface_t iface;
static pcie_manager_context_t ctx = {.rpss_idx = RPSS0, .dev = &dev, .iface = &iface};

/*------------- Functions ----------------*/
TEST_FUNCTION(pcie_service_init_fail, NULL, NULL)
{
    auto* sched = (PDFWK_SCHEDULE)0xdeadbeef;
    uint16_t rpss_to_init = (RPSS0 | RPSS1 | RPSS2 | RPSS3);

    // Bad args
    expect_value(FPFwErrorRaise, error, (uint32_t)(-EINVAL));
    if (!set_error_handler_return())
    {
        scp_pcie_initialize(nullptr, rpss_to_init);
    }

    expect_any_always(__wrap__txe_event_flags_create, group_ptr);
    expect_any_always(__wrap__txe_event_flags_create, name_ptr);
    expect_any_always(__wrap__txe_event_flags_create, event_control_block_size);
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
    will_return(__wrap__txe_thread_create, TX_SUCCESS);
    will_return(__wrap__txe_event_flags_create, TX_SUCCESS);

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
        scp_pcie_initialize(sched, rpss_to_init);
    }

    // tx thread fails
    expect_value(__wrap_pcie_dfwk_init, schedule, sched);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);      // scp_pcie_config_service_initialize
    will_return(__wrap__txe_event_flags_create, TX_SUCCESS); // scp_pcie_config_service_initialize
    will_return(__wrap__txe_thread_create, TX_NOT_DONE);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NOT_DONE);
    if (!set_error_handler_return())
    {
        scp_pcie_initialize(sched, rpss_to_init);
    }
}

TEST_FUNCTION(config_service_init_fail, NULL, NULL)
{
    auto* sched = (PDFWK_SCHEDULE)0xdeadbeef;
    uint16_t rpss_to_init = (RPSS0 | RPSS1 | RPSS2 | RPSS3);

    // tx_event_flags_create fails
    expect_any_always(__wrap__txe_event_flags_create, group_ptr);
    expect_any_always(__wrap__txe_event_flags_create, name_ptr);
    expect_any_always(__wrap__txe_event_flags_create, event_control_block_size);
    will_return(__wrap__txe_event_flags_create, TX_NO_MEMORY);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NO_MEMORY);

    if (!set_error_handler_return())
    {
        scp_pcie_initialize(sched, rpss_to_init);
    }

    // tx_thread_create fails
    will_return(__wrap__txe_event_flags_create, TX_SUCCESS);
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
        scp_pcie_initialize(sched, rpss_to_init);
    }
}

TEST_FUNCTION(config_service_thread_fail, NULL, NULL)
{
    pcie_config_manager_context_t ctx;

    // event_flags_get fails
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return(__wrap__txe_event_flags_get, TX_WAIT_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_WAIT_ERROR);
    will_return(__wrap_idsw_get_die_id, SOC_D0);

    if (!set_error_handler_return())
    {
        config_variable_service_thread_fn((ULONG)&ctx);
    }
}

TEST_FUNCTION(config_service_thread_success_die0, NULL, NULL)
{
    pcie_config_manager_context_t ctx;
    kingsgate_pcie_root_bridge_config rb_config_var = {{{0}}};
    kingsgate_pcie_vab_config vab_config_var = {0};
    ctx.rb_config_var = &rb_config_var;
    ctx.vab_config_var = &vab_config_var;

    rb_config_var.rootbridge_config[0].flags.is_enabled = true;
    rb_config_var.rootbridge_config[0].mmioh.base = 0x10000000;
    rb_config_var.rootbridge_config[0].mmioh.limit = 0x20000000;
    rb_config_var.rootbridge_config[0].mmioh.base = 0x30000000;
    rb_config_var.rootbridge_config[0].mmioh.limit = 0x40000000;

    // event_flags_get fails
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    expect_any_always(__wrap__txe_event_flags_delete, group_ptr);

    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);
    will_return(__wrap__txe_event_flags_delete, TX_SUCCESS);
    will_return(__wrap_idsw_get_die_id, SOC_D0);

    if (!set_error_handler_return())
    {
        config_variable_service_thread_fn((ULONG)&ctx);
    }

    // Check that SDM/CDED config was set and is for DIE0
    assert_int_equal(rb_config_var.rootbridge_config[16].flags.is_enabled, true);
    assert_int_equal(rb_config_var.rootbridge_config[17].flags.is_enabled, true);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmioh.base, D0_SDM_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmioh.base, D0_CDED_MMIOH_START);
}

TEST_FUNCTION(config_service_thread_success_die1, NULL, NULL)
{
    pcie_config_manager_context_t ctx;
    kingsgate_pcie_root_bridge_config rb_config_var = {{{0}}};
    kingsgate_pcie_vab_config vab_config_var = {0};
    ctx.rb_config_var = &rb_config_var;
    ctx.vab_config_var = &vab_config_var;

    rb_config_var.rootbridge_config[0].flags.is_enabled = true;
    rb_config_var.rootbridge_config[0].mmioh.base = 0x10000000;
    rb_config_var.rootbridge_config[0].mmioh.limit = 0x20000000;
    rb_config_var.rootbridge_config[0].mmioh.base = 0x30000000;
    rb_config_var.rootbridge_config[0].mmioh.limit = 0x40000000;

    // event_flags_get fails
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    expect_any_always(__wrap__txe_event_flags_delete, group_ptr);

    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);
    will_return(__wrap__txe_event_flags_delete, TX_SUCCESS);
    will_return(__wrap_idsw_get_die_id, SOC_D1);

    if (!set_error_handler_return())
    {
        config_variable_service_thread_fn((ULONG)&ctx);
    }

    // Check that SDM/CDED config was set and is for DIE1
    assert_int_equal(rb_config_var.rootbridge_config[16].flags.is_enabled, true);
    assert_int_equal(rb_config_var.rootbridge_config[17].flags.is_enabled, true);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmioh.base, D1_SDM_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmioh.base, D1_CDED_MMIOH_START);
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
