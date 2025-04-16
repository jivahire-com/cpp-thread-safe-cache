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
#include "pcie_mocks.h"

#include <DfwkPtrTypes.h> // for PDFWK_SCHEDULE
#include <atu_api.h>
#include <errno.h>             // for EINVAL
#include <error_handler.h>     // for set_error_handler_return
#include <kng_soc_constants.h> // for RPSS0
#include <mscp_exp_rmss_memory_map.h>
#include <pcie_dfwk.h>             // for pciess_device_interface_t, pciess_dev...
#include <pcie_manager_i.h>        // for rpss_req_completion_cb, send_start_li...
#include <pcie_rp_event_handler.h> // for process_wait_for_event_data
#include <pcie_sync_requests_i.h>
#include <scp_pcie_manager.h> // for scp_pcie_initialize, pcie_manager_con...
#include <setjmp.h>
#include <silibs_kng_soc.h>
#include <silibs_status.h> // for SILIBS_E_TIMEOUT
#include <tx_api.h>        // for TX_NOT_DONE, TX_NO_MEMORY, TX_NO_WAIT
}

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_COMPLETION_ROUTINE ((DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE)0x00ef6430)
#define TEST_COMPLETION_CONTEXT ((void*)0x00f36000)
#define BUGCHECK_MOCK_RETURN    (setjmp(mock_jump_buf))
#define bugcheck_mock_return()  BUGCHECK_MOCK_RETURN

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pciess_device_t dev;
static pciess_device_interface_t iface;
static pcie_manager_context_t ctx = {.rpss_idx = RPSS0, .dev = &dev, .iface = &iface};
static pcie_ss_entity_t mock_pcie_ent;
bool memcpy_mock = false;
static jmp_buf mock_jump_buf;
static bool should_return;

/*------------- Functions ----------------*/
extern "C" {
void __wrap_crash_dump_bug_check(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(p0);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    function_called();

    /* Handle noreturn, allowing control to return to test */
    if (!should_return)
    {
        longjmp(mock_jump_buf, 1);
    }
}
}

TEST_FUNCTION(pcie_service_init_fail, NULL, NULL)
{
    auto* sched = (PDFWK_SCHEDULE)0xdeadbeef;
    uint16_t rpss_to_init = ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3));

    // Bad args
    expect_value(FPFwErrorRaise, error, (uint32_t)(-EINVAL));
    if (!set_error_handler_return())
    {
        scp_pcie_initialize(nullptr, rpss_to_init, DIE_0);
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
    will_return_always(__wrap__txe_event_flags_create, TX_SUCCESS);

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
        scp_pcie_initialize(sched, rpss_to_init, DIE_0);
    }

    // tx thread create fails
    will_return(__wrap__txe_thread_create, TX_NOT_DONE); // scp_pcie_start_config_service_thread
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NOT_DONE);
    if (!set_error_handler_return())
    {
        scp_pcie_start_config_service_thread();
    }
}

TEST_FUNCTION(pcie_service_init_success_die1, NULL, NULL)
{
    auto* sched = (PDFWK_SCHEDULE)0xdeadbeef;
    uint16_t rpss_to_init = ((1 << RPSS4) | (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7));

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
    expect_value_count(__wrap_pcie_dfwk_init, schedule, sched, 4);
    will_return_always(__wrap__txe_thread_create, TX_SUCCESS);
    will_return_always(__wrap__txe_event_flags_create, TX_SUCCESS);
    will_return_always(__wrap__txe_queue_create, TX_SUCCESS);
    expect_any_always(__wrap__txe_queue_create, queue_ptr);
    expect_any_always(__wrap__txe_queue_create, name_ptr);
    expect_any_always(__wrap__txe_queue_create, message_size);
    expect_any_always(__wrap__txe_queue_create, queue_start);
    expect_any_always(__wrap__txe_queue_create, queue_size);
    expect_any_always(__wrap__txe_queue_create, queue_control_block_size);
    scp_pcie_initialize(sched, rpss_to_init, DIE_1);
}

TEST_FUNCTION(config_service_init_event_create_fail, NULL, NULL)
{
    auto* sched = (PDFWK_SCHEDULE)0xdeadbeef;
    uint16_t rpss_to_init = ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3));

    // tx_event_flags_create fails
    expect_any_always(__wrap__txe_event_flags_create, group_ptr);
    expect_any_always(__wrap__txe_event_flags_create, name_ptr);
    expect_any_always(__wrap__txe_event_flags_create, event_control_block_size);
    will_return(__wrap__txe_event_flags_create, TX_NOT_DONE);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NOT_DONE);

    if (!set_error_handler_return())
    {
        scp_pcie_initialize(sched, rpss_to_init, DIE_0);
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
    will_return(__wrap_accel_is_isolation_enabled, false);
    will_return(__wrap_accel_is_isolation_enabled, false);

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

    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE);
    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_END);
    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE);
    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_END);

    will_return(__wrap_variable_service_async_set_variable, 0);
    will_return(__wrap_variable_service_async_set_variable, 0);

    will_return(__wrap_accel_is_isolation_enabled, false);
    will_return(__wrap_accel_is_isolation_enabled, false);

    if (!set_error_handler_return())
    {
        memcpy_mock = true;
        config_variable_service_thread_fn((ULONG)&ctx);
        memcpy_mock = false;
    }

    // Check that SDM/CDED config was set and is for DIE0
    assert_int_equal(rb_config_var.rootbridge_config[16].flags.is_enabled, true);
    assert_int_equal(rb_config_var.rootbridge_config[17].flags.is_enabled, true);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmioh.base, D0_SDM_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmioh.base, D0_CDED_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmiol.base, UINT32_MAX);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmiol.base, UINT32_MAX);
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
    will_return(__wrap_idsw_get_die_id, SOC_D0);

    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE);
    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_END);
    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE);
    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_END);

    will_return(__wrap_variable_service_async_set_variable, 0);
    will_return(__wrap_variable_service_async_set_variable, 0);

    will_return(__wrap_accel_is_isolation_enabled, false);
    will_return(__wrap_accel_is_isolation_enabled, false);

    if (!set_error_handler_return())
    {
        memcpy_mock = true;
        config_variable_service_thread_fn((ULONG)&ctx);
        memcpy_mock = false;
    }

    // Check that SDM/CDED config was set and is for DIE0
    assert_int_equal(rb_config_var.rootbridge_config[16].flags.is_enabled, true);
    assert_int_equal(rb_config_var.rootbridge_config[17].flags.is_enabled, true);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmioh.base, D0_SDM_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmioh.base, D0_CDED_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmiol.base, UINT32_MAX);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmiol.base, UINT32_MAX);
}

TEST_FUNCTION(config_service_thread_success_die0_accel_isolation, NULL, NULL)
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

    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE);
    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_END);
    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE);
    will_return(__wrap_variable_service_initialize_ctx, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_END);

    will_return(__wrap_variable_service_async_set_variable, 0);
    will_return(__wrap_variable_service_async_set_variable, 0);

    will_return(__wrap_accel_is_isolation_enabled, true);
    will_return(__wrap_accel_is_isolation_enabled, true);

    if (!set_error_handler_return())
    {
        memcpy_mock = true;
        config_variable_service_thread_fn((ULONG)&ctx);
        memcpy_mock = false;
    }

    // Check that SDM/CDED config was set and is for DIE0
    assert_int_equal(rb_config_var.rootbridge_config[16].flags.is_enabled, false);
    assert_int_equal(rb_config_var.rootbridge_config[17].flags.is_enabled, false);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmioh.base, D0_SDM_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmioh.base, D0_CDED_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmiol.base, UINT32_MAX);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmiol.base, UINT32_MAX);
}

TEST_FUNCTION(config_service_thread_success_die1_accel_isolation, NULL, NULL)
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

    will_return(__wrap_variable_service_initialize_ctx, MSCP_ATU_AP_WINDOW_VAR_SVC_PCIE_PAYLOAD_BASE);
    will_return(__wrap_variable_service_initialize_ctx, MSCP_ATU_AP_WINDOW_VAR_SVC_END_ADDR);
    will_return(__wrap_variable_service_initialize_ctx, MSCP_ATU_AP_WINDOW_VAR_SVC_PCIE_PAYLOAD_BASE);
    will_return(__wrap_variable_service_initialize_ctx, MSCP_ATU_AP_WINDOW_VAR_SVC_END_ADDR);

    will_return(__wrap_variable_service_async_set_variable, 0);
    will_return(__wrap_variable_service_async_set_variable, 0);

    will_return(__wrap_accel_is_isolation_enabled, true);
    will_return(__wrap_accel_is_isolation_enabled, true);

    if (!set_error_handler_return())
    {
        memcpy_mock = true;
        config_variable_service_thread_fn((ULONG)&ctx);
        memcpy_mock = false;
    }

    // Check that SDM/CDED config was set and is for DIE1
    assert_int_equal(rb_config_var.rootbridge_config[16].flags.is_enabled, false);
    assert_int_equal(rb_config_var.rootbridge_config[17].flags.is_enabled, false);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmioh.base, D1_SDM_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmioh.base, D1_CDED_MMIOH_START);
    assert_int_equal(rb_config_var.rootbridge_config[16].mmiol.base, UINT32_MAX);
    assert_int_equal(rb_config_var.rootbridge_config[17].mmiol.base, UINT32_MAX);
}

/* Tests to validate completion request handling */
TEST_FUNCTION(test_completion_callback, NULL, NULL) // abe
{
    auto& async_req = ctx.async_req[0];
    async_req.status = SILIBS_E_TIMEOUT;
    async_req.async_data.data = 0;

    mock_pcie_ent.id = (RPSS_INSTANCE)0;
    mock_pcie_ent.rps[0].enabled = true;
    mock_pcie_ent.rps[1].enabled = false;
    mock_pcie_ent.rps[2].enabled = false;
    mock_pcie_ent.rps[3].enabled = false;
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_RPSS_ENTITY_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, &mock_pcie_ent);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, rpss_req_completion_cb);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, &ctx);
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &ctx.iface->header);
    expect_any(__wrap_DfwkInterfaceSendAsync, Request);

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

/* Test Link down when the root port is ready */
TEST_FUNCTION(test_process_wait_for_event_linkdown_rp_ready, NULL, NULL)
{
    /* Setup BIT0 (link down) in the wait for event completion */
    pciess_completion_request_t cmpl_req;
    cmpl_req.async_data.data = 0x0001;

    for (uint8_t i = RPSS0; i < RPSS7; i++)
    {
        ctx.rpss_idx = (RPSS_INSTANCE)i;
        cmpl_req.rp_index = 0;
        expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_RP_READY_REQUEST);
        will_return(__wrap_DfwkInterfaceSendSync, nullptr);
        will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
        will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
        will_return(__wrap__txe_timer_delete, TX_SUCCESS);
        will_return(__wrap__txe_timer_create, TX_SUCCESS);
        expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, INITIATE_LINK_TRAINING);
        will_return(__wrap_DfwkInterfaceSendSync, nullptr);
        will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
        will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
        process_wait_for_event_data(&ctx, &cmpl_req);
    }
}

/* Test Link down when the root port is not ready - we should not re-train in this case */
TEST_FUNCTION(test_process_wait_for_event_linkdown_rp_not_ready, NULL, NULL)
{
    /* Setup BIT0 (link down) in the wait for event completion */
    pciess_completion_request_t cmpl_req;
    cmpl_req.async_data.data = 0x0001;

    for (uint8_t i = RPSS0; i < RPSS7; i++)
    {
        ctx.rpss_idx = (RPSS_INSTANCE)i;
        cmpl_req.rp_index = 0;
        expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_RP_READY_REQUEST);
        will_return(__wrap_DfwkInterfaceSendSync, nullptr);
        will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_BUSY);
        will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
        process_wait_for_event_data(&ctx, &cmpl_req);
    }
}

/* Test other cases which we don't currently handle */
TEST_FUNCTION(test_process_wait_for_event_dpc_handler, NULL, NULL)
{
    /* Setup BIT14 (DPC) in the wait for event completion */
    pciess_completion_request_t cmpl_req;
    cmpl_req.async_data.data = (1 << 14);

    for (uint8_t i = RPSS0; i < RPSS7; i++)
    {
        ctx.rpss_idx = (RPSS_INSTANCE)i;
        process_wait_for_event_data(&ctx, &cmpl_req);
    }
}

/* Test other cases which we don't currently handle */
TEST_FUNCTION(test_process_wait_for_event_unhandled, NULL, NULL)
{
    for (uint8_t i = 1; i <= 12; i++)
    {
        pciess_completion_request_t cmpl_req;
        cmpl_req.async_data.data = (1 << i);

        for (uint8_t i = RPSS0; i < RPSS7; i++)
        {
            ctx.rpss_idx = (RPSS_INSTANCE)i;
            process_wait_for_event_data(&ctx, &cmpl_req);
        }
    }
}

TEST_FUNCTION(test_sync_get_rpss_entity_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_RPSS_ENTITY_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, &mock_pcie_ent);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    pcie_ss_entity_t* rpss_ent = send_sync_rpss_get_entity((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx);
    assert_ptr_equal(rpss_ent, &mock_pcie_ent);
}

TEST_FUNCTION(test_sync_get_rpss_entity_silibs_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_RPSS_ENTITY_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, &mock_pcie_ent);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_OVERWRITTEN);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);
    should_return = false;

    if (!bugcheck_mock_return())
    {
        send_sync_rpss_get_entity((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx);
    }
}

TEST_FUNCTION(test_send_sync_rpss_initial_config_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, INITIAL_CONFIG_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    silibs_status_t sts = send_sync_rpss_initial_config((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rpss_initial_config_silibs_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, INITIAL_CONFIG_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_TIMEOUT);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    should_return = false;

    if (!bugcheck_mock_return())
    {
        send_sync_rpss_initial_config((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx);
    }
}

TEST_FUNCTION(test_send_sync_rpss_pre_rp_init_request_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, PRE_RP_INIT_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    silibs_status_t sts = send_sync_rpss_pre_rp_init_request((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rpss_pre_rp_init_request_silibs_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, PRE_RP_INIT_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_TIMEOUT);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    should_return = false;

    if (!bugcheck_mock_return())
    {
        send_sync_rpss_pre_rp_init_request((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx);
    }
}

TEST_FUNCTION(test_send_sync_rpss_post_rp_init_request_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, POST_RP_INIT_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    silibs_status_t sts = send_sync_rpss_post_rp_init_request((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rpss_post_rp_init_request_silibs_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, POST_RP_INIT_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_TIMEOUT);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    should_return = false;

    if (!bugcheck_mock_return())
    {
        send_sync_rpss_post_rp_init_request((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx);
    }
}

TEST_FUNCTION(test_send_sync_rp_is_ready_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_RP_READY_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    bool rp_ready = send_sync_rp_is_ready((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(rp_ready, true);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_RP_READY_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_BUSY);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    rp_ready = send_sync_rp_is_ready((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(rp_ready, false);
}

TEST_FUNCTION(test_send_sync_rp_is_ready_dfwk_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_RP_READY_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, -1);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);
    should_return = false;
    if (!bugcheck_mock_return())
    {
        send_sync_rp_is_ready((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    }
}

TEST_FUNCTION(test_send_sync_rp_initiate_link_training_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, INITIATE_LINK_TRAINING);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    silibs_status_t sts = send_sync_rp_initiate_link_training((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_initiate_link_training_silibs_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, INITIATE_LINK_TRAINING);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_TIMEOUT);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    should_return = false;

    if (!bugcheck_mock_return())
    {
        send_sync_rp_initiate_link_training((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    }
}

TEST_FUNCTION(test_send_sync_rp_get_link_status_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_LINK_STATUS);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_OVERWRITTEN);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    silibs_status_t sts = send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_E_OVERWRITTEN);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_LINK_STATUS);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    sts = send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_LINK_STATUS);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_BUSY);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    sts = send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_E_BUSY);
}

TEST_FUNCTION(test_send_sync_rp_get_link_status_dfwk_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_LINK_STATUS);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, -1);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    should_return = false;

    if (!bugcheck_mock_return())
    {
        send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    }
}

TEST_FUNCTION(test_all_sync_req_invalid_ids, NULL, NULL)
{
    RPSS_INSTANCE valid_rpss_id = (RPSS_INSTANCE)0x01;
    RPSS_INSTANCE invalid_rpss_id = (RPSS_INSTANCE)0xFF;
    uint8_t valid_rp_id = 0x03;
    uint8_t invalid_rp_id = 0xFF;

    expect_function_calls(__wrap_crash_dump_bug_check, 10);
    should_return = false;

    if (!bugcheck_mock_return())
    {
        send_sync_rpss_get_entity((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rpss_initial_config((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rpss_pre_rp_init_request((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rpss_post_rp_init_request((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_is_ready((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_is_ready((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_initiate_link_training((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_initiate_link_training((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }
}
