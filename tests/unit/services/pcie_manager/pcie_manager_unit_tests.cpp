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
#include <pcie_rp_ide.h>

extern "C" {
#include "pcie_mocks.h"

#include <DfwkPtrTypes.h> // for PDFWK_SCHEDULE
#include <atu_api.h>
#include <cper.h>
#include <errno.h>             // for EINVAL
#include <error_handler.h>     // for set_error_handler_return
#include <kng_soc_constants.h> // for RPSS0
#include <mscp_exp_rmss_memory_map.h>
#include <pcie_async_requests_i.h>
#include <pcie_dfwk.h> // for pciess_device_interface_t, pciess_dev...
#include <pcie_einj_structs.h>
#include <pcie_error_injection_i.h>
#include <pcie_lt_events.h>
#include <pcie_manager_i.h>        // for rpss_req_completion_cb, send_start_li...
#include <pcie_rp_event_handler.h> // for process_wait_for_event_data
#include <pcie_rp_ide.h>
#include <pcie_rp_rasdes.h>
#include <pcie_ss.h>
#include <pcie_sync_requests_i.h>
#include <pciess_int.h>
#include <scp_pcie_manager.h> // for scp_pcie_initialize, pcie_manager_con...
#include <setjmp.h>
#include <silibs_common.h>
#include <silibs_kng_soc.h>
#include <silibs_status.h> // for SILIBS_E_TIMEOUT
#include <startup_shutdown_ssi.h>
#include <tx_api.h> // for TX_NOT_DONE, TX_NO_MEMORY, TX_NOPCIE_IDE_MISC_GLOBAL_INT_WAIT
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
    async_req.async_data.int_mask = 0;

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

    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, rpss_req_completion_cb);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, &ctx);
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &ctx.iface->header);
    expect_any(__wrap_DfwkInterfaceSendAsync, Request);

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
    cmpl_req.async_data.int_mask = 0x0001;

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
    cmpl_req.async_data.int_mask = 0x0001;

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

/* Test RP_INT_GLOBAL_IDE key needed notifications */
TEST_FUNCTION(test_process_wait_for_event_global_ide_key_needed, NULL, NULL)
{
    /* Setup BIT1 (RP_INT_GLOBAL_IDE) in the wait for event completion */
    pciess_completion_request_t cmpl_req;
    cmpl_req.async_data.int_mask = 1 << PCIESS_RP_INT_GLOBAL_IDE;
    cmpl_req.async_data.glbl_ide_data = (uint64_t)(PCIE_IDE_MISC_GLOBAL_INT | PCIE_IDE_KEY_NEEDED_INT);

    for (uint8_t i = RPSS0; i < RPSS7; i++)
    {
        ctx.rpss_idx = (RPSS_INSTANCE)i;
        cmpl_req.rp_index = 0;
        expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, FORCE_AER_INTERNAL_UNCORR_ERROR);
        will_return(__wrap_DfwkInterfaceSendSync, nullptr);
        will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
        will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
        process_wait_for_event_data(&ctx, &cmpl_req);
    }
}

/* Test RP_INT_GLOBAL_IDE TX rekey requests */
TEST_FUNCTION(test_process_wait_for_event_global_ide_tx_rekey_req, NULL, NULL)
{
    /* Setup BIT1 (RP_INT_GLOBAL_IDE) in the wait for event completion */
    pciess_completion_request_t cmpl_req;
    cmpl_req.async_data.int_mask = 1 << PCIESS_RP_INT_GLOBAL_IDE;
    cmpl_req.async_data.glbl_ide_data = (uint64_t)(PCIE_IDE_MISC_GLOBAL_INT | PCIE_IDE_TX_REKEY_REQ_INT);

    for (uint8_t i = RPSS0; i < RPSS7; i++)
    {
        ctx.rpss_idx = (RPSS_INSTANCE)i;
        cmpl_req.rp_index = 0;
        expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, IDE_TX_REKEY);
        will_return(__wrap_DfwkInterfaceSendSync, nullptr);
        will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
        will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
        process_wait_for_event_data(&ctx, &cmpl_req);
    }
}

/* Test RP_INT_GLOBAL_IDE RX rekey requests */
TEST_FUNCTION(test_process_wait_for_event_global_ide_rx_rekey_req, NULL, NULL)
{
    /* Setup BIT1 (RP_INT_GLOBAL_IDE) in the wait for event completion */
    pciess_completion_request_t cmpl_req;
    cmpl_req.async_data.int_mask = 1 << PCIESS_RP_INT_GLOBAL_IDE;
    cmpl_req.async_data.glbl_ide_data = (uint64_t)(PCIE_IDE_MISC_GLOBAL_INT | PCIE_IDE_RX_REKEY_REQ_INT);

    for (uint8_t i = RPSS0; i < RPSS7; i++)
    {
        ctx.rpss_idx = (RPSS_INSTANCE)i;
        cmpl_req.rp_index = 0;
        expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, IDE_RX_REKEY);
        will_return(__wrap_DfwkInterfaceSendSync, nullptr);
        will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
        will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
        process_wait_for_event_data(&ctx, &cmpl_req);
    }
}

/* Test other cases which we don't currently handle */
TEST_FUNCTION(test_process_wait_for_event_unhandled, NULL, NULL)
{
    for (uint8_t i = 9; i <= 12; i++)
    {
        pciess_completion_request_t cmpl_req;
        cmpl_req.async_data.int_mask = (1 << i);

        for (uint8_t j = RPSS0; j < RPSS7; j++)
        {
            ctx.rpss_idx = (RPSS_INSTANCE)j;
            process_wait_for_event_data(&ctx, &cmpl_req);
        }
    }
}

/* Test link up routine in process_wait_for_event_data */
TEST_FUNCTION(test_process_wait_for_event_linkup, NULL, NULL)
{
    /* Setup BIT0 (link up) in the wait for event completion */
    pciess_completion_request_t cmpl_req;
    cmpl_req.async_data.int_mask = 1 << PCIESS_RP_INT_LINK_UP;

    ctx.rpss_idx = (RPSS_INSTANCE)0;
    cmpl_req.rp_index = 0;
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_LINK_STATUS);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, POST_RP_LINK_UP_INIT);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    will_return(__wrap_system_info_get_soc_position, 0);
    will_return(__wrap_config_get_overlake_rpss_index_primary_soc, 1); // not this RPSS
    expect_value(__wrap__tx_thread_sleep, timer_ticks, 1000);

    process_wait_for_event_data(&ctx, &cmpl_req);

    ctx.rpss_idx = (RPSS_INSTANCE)0;
    cmpl_req.rp_index = 1; // wrong rp index
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_LINK_STATUS);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, POST_RP_LINK_UP_INIT);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    expect_value(__wrap__tx_thread_sleep, timer_ticks, 1000);

    process_wait_for_event_data(&ctx, &cmpl_req);
}

/* Test link up routine in process_wait_for_event_data */
TEST_FUNCTION(test_process_wait_for_event_linkup_overlake_failure, NULL, NULL)
{
    /* Setup BIT0 (link up) in the wait for event completion */
    pciess_completion_request_t cmpl_req;
    cmpl_req.async_data.int_mask = 1 << PCIESS_RP_INT_LINK_UP;

    ctx.rpss_idx = (RPSS_INSTANCE)0;
    cmpl_req.rp_index = 0;
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_LINK_STATUS);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_OVERWRITTEN); // link training failure
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    will_return(__wrap_system_info_get_soc_position, 0);
    will_return(__wrap_config_get_overlake_rpss_index_primary_soc, ctx.rpss_idx); // Overlake RPSS
    will_return(__wrap_config_get_enable_overlake_sbr_workaround, 1);             // enable workaround

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SET_SECONDARY_BUS_RESET_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    expect_value(__wrap__tx_thread_sleep, timer_ticks, 1000);
    expect_value(__wrap__tx_thread_sleep, timer_ticks, 1);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, CLEAR_SECONDARY_BUS_RESET_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, POST_RP_LINK_UP_INIT);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    process_wait_for_event_data(&ctx, &cmpl_req);

    /* Secondary SoC case*/
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, GET_LINK_STATUS);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_OVERWRITTEN); // link training failure
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);
    will_return(__wrap_system_info_get_soc_position, 1); // GPIO indicates secondary SoC
    will_return(__wrap_config_get_overlake_rpss_index_secondary_soc, ctx.rpss_idx); // Overlake RPSS
    will_return(__wrap_config_get_enable_overlake_sbr_workaround, 1);               // enable workaround

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SET_SECONDARY_BUS_RESET_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    expect_value(__wrap__tx_thread_sleep, timer_ticks, 1000);
    expect_value(__wrap__tx_thread_sleep, timer_ticks, 1);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, CLEAR_SECONDARY_BUS_RESET_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, POST_RP_LINK_UP_INIT);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    process_wait_for_event_data(&ctx, &cmpl_req);
}

TEST_FUNCTION(test_sync_get_rpss_entity_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    mock_pcie_ent.id = (RPSS_INSTANCE)0;
    mock_pcie_ent.rps[0].enabled = true;
    mock_pcie_ent.rps[1].enabled = false;
    mock_pcie_ent.rps[2].enabled = false;
    mock_pcie_ent.rps[3].enabled = false;

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

    mock_pcie_ent.id = (RPSS_INSTANCE)0;
    mock_pcie_ent.rps[0].enabled = true;
    mock_pcie_ent.rps[1].enabled = false;
    mock_pcie_ent.rps[2].enabled = false;
    mock_pcie_ent.rps[3].enabled = false;

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

TEST_FUNCTION(test_send_sync_rp_set_secondary_bus_reset_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SET_SECONDARY_BUS_RESET_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    silibs_status_t sts = send_sync_rp_set_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_set_secondary_bus_reset_dfwk_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SET_SECONDARY_BUS_RESET_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, -1);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    silibs_status_t sts = send_sync_rp_set_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_clear_secondary_bus_reset_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, CLEAR_SECONDARY_BUS_RESET_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    silibs_status_t sts = send_sync_rp_clear_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_clear_secondary_bus_reset_dfwk_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, CLEAR_SECONDARY_BUS_RESET_REQUEST);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, -1);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    silibs_status_t sts = send_sync_rp_clear_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_force_aer_internal_uncorr_error_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, FORCE_AER_INTERNAL_UNCORR_ERROR);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    silibs_status_t sts =
        send_sync_rp_force_aer_internal_uncorr_error((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_force_aer_internal_uncorr_error_dfwk_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, FORCE_AER_INTERNAL_UNCORR_ERROR);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, -1);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    silibs_status_t sts =
        send_sync_rp_force_aer_internal_uncorr_error((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_tx_rekey_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, IDE_TX_REKEY);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    silibs_status_t sts = send_sync_rp_tx_rekey((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_tx_rekey_dfwk_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, IDE_TX_REKEY);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, -1);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    silibs_status_t sts = send_sync_rp_tx_rekey((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_rx_rekey_pass, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, IDE_RX_REKEY);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    silibs_status_t sts = send_sync_rp_rx_rekey((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_send_sync_rp_rx_rekey_dfwk_fail, NULL, NULL)
{
    ctx.rpss_idx = (RPSS_INSTANCE)0x01;

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, IDE_RX_REKEY);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, -1);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    silibs_status_t sts = send_sync_rp_rx_rekey((PDFWK_INTERFACE_HEADER)ctx.iface, ctx.rpss_idx, 0x01);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_all_sync_req_invalid_ids, NULL, NULL)
{
    RPSS_INSTANCE valid_rpss_id = (RPSS_INSTANCE)0x01;
    RPSS_INSTANCE invalid_rpss_id = (RPSS_INSTANCE)0xFF;
    uint8_t valid_rp_id = 0x03;
    uint8_t invalid_rp_id = 0xFF;
    ras_einj_info_t mock_einj_params;

    expect_function_calls(__wrap_crash_dump_bug_check, 23);
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
        send_sync_rpss_inject_pcie_error((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, &mock_einj_params);
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
        send_sync_rp_post_link_up_init((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_post_link_up_init((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_set_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_set_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_clear_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_clear_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_force_aer_internal_uncorr_error((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_force_aer_internal_uncorr_error((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_tx_rekey((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_tx_rekey((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_rx_rekey((PDFWK_INTERFACE_HEADER)ctx.iface, invalid_rpss_id, valid_rp_id);
    }

    if (!bugcheck_mock_return())
    {
        send_sync_rp_rx_rekey((PDFWK_INTERFACE_HEADER)ctx.iface, valid_rpss_id, invalid_rp_id);
    }
}

TEST_FUNCTION(test_init_wait_for_event_queue_on_rpss, NULL, NULL)
{
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

    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, rpss_req_completion_cb);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, &ctx);
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &ctx.iface->header);
    expect_any(__wrap_DfwkInterfaceSendAsync, Request);

    init_wait_for_event_queue_on_rpss(&ctx);
}

TEST_FUNCTION(test_pcie_error_injection_cb_aer, NULL, NULL)
{
    ras_einj_info_t mock_einj_info;
    acpi_einj_cmd_status_t status;
    pcie_einj_operation_t* op = (pcie_einj_operation_t*)&(mock_einj_info.status_operation.value);
    pcie_einj_params_t* pcie_params = (pcie_einj_params_t*)&(mock_einj_info.param_type.error_parameters[0]);
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;

    op->error_type = PCIE_ERROR_TYPE_AER;
    pcie_params->error_data.aer = PCIE_SS_APP_ERR_MALFORMED_TLP;
    op->error_count = 1;

    for (uint8_t i = RPSS0; i <= RPSS7; i++)
    {
        ctx.rpss_idx = (RPSS_INSTANCE)i;
        pcie_params->sbdf.bus = (i * 30) + 15;
        will_return(__wrap_idsw_get_die_id, SOC_D0);
        will_return(__wrap_scp_pcie_get_manager_context, &ctx);
        expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, INJECT_PCIE_ERROR);
        will_return(__wrap_DfwkInterfaceSendSync, nullptr);
        will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
        will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

        status = pcie_error_injection_cb(&mock_einj_info, nullptr);
        assert_int_equal(status, ACPI_EINJ_SUCCESS);
    }
}

TEST_FUNCTION(test_pcie_error_injection_cb_internal, NULL, NULL)
{
    ras_einj_info_t mock_einj_info;
    acpi_einj_cmd_status_t status;
    pcie_einj_operation_t* op = (pcie_einj_operation_t*)&(mock_einj_info.status_operation.value);
    pcie_einj_params_t* pcie_params = (pcie_einj_params_t*)&(mock_einj_info.param_type.error_parameters[0]);
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;

    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_CRC;
    pcie_params->error_data.crc = PCIE_RASDES_INJ_ECRC;
    op->error_count = 10;

    for (uint8_t i = RPSS0; i <= RPSS7; i++)
    {
        ctx.rpss_idx = (RPSS_INSTANCE)i;
        pcie_params->sbdf.bus = (i * 30) + 15;
        will_return(__wrap_idsw_get_die_id, SOC_D0);
        will_return(__wrap_scp_pcie_get_manager_context, &ctx);
        expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, INJECT_PCIE_ERROR);
        will_return(__wrap_DfwkInterfaceSendSync, nullptr);
        will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
        will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

        status = pcie_error_injection_cb(&mock_einj_info, nullptr);
        assert_int_equal(status, ACPI_EINJ_SUCCESS);
    }
}

TEST_FUNCTION(test_pcie_error_injection_silibs_error, NULL, NULL)
{
    ras_einj_info_t mock_einj_info;
    acpi_einj_cmd_status_t status;
    pcie_einj_operation_t* op = (pcie_einj_operation_t*)&(mock_einj_info.status_operation.value);
    pcie_einj_params_t* pcie_params = (pcie_einj_params_t*)&(mock_einj_info.param_type.error_parameters[0]);
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;

    op->error_type = PCIE_ERROR_TYPE_AER;
    pcie_params->error_data.aer = PCIE_SS_APP_ERR_MALFORMED_TLP;
    op->error_count = 1;

    for (uint8_t i = RPSS0; i <= RPSS7; i++)
    {
        ctx.rpss_idx = (RPSS_INSTANCE)i;
        pcie_params->sbdf.bus = (i * 30) + 15;
        will_return(__wrap_idsw_get_die_id, SOC_D0);
        will_return(__wrap_scp_pcie_get_manager_context, &ctx);
        expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, INJECT_PCIE_ERROR);
        will_return(__wrap_DfwkInterfaceSendSync, nullptr);
        will_return(__wrap_DfwkInterfaceSendSync, SILIBS_E_DEVICE);
        will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

        status = pcie_error_injection_cb(&mock_einj_info, nullptr);
        assert_int_equal(status, ACPI_EINJ_UNKNOWN_FAILURE);
    }
}

TEST_FUNCTION(test_pcie_error_injection_cb_bad_einj_buffer, NULL, NULL)
{
    ras_einj_info_t mock_einj_info;
    acpi_einj_cmd_status_t status;
    pcie_einj_operation_t* op = (pcie_einj_operation_t*)&(mock_einj_info.status_operation.value);
    pcie_einj_params_t* pcie_params = (pcie_einj_params_t*)&(mock_einj_info.param_type.error_parameters[0]);

    /* Null buffer */
    status = pcie_error_injection_cb(nullptr, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad component instance */
    mock_einj_info.component_instance = 0;
    will_return(__wrap_idsw_get_die_id, SOC_D1);
    will_return(__wrap_idsw_get_die_id, SOC_D1);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad component group */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_DDR;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad segment */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 1;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad bus number */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 248;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad error type */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 247;
    op->error_type = PCIE_ERROR_TYPE_MAX;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad AER type */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 247;
    op->error_type = PCIE_ERROR_TYPE_AER;
    pcie_params->error_data.aer = (PCIE_SS_RP_APP_ERROR)0x8000000;
    op->error_count = 10;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad internal CRC error type  */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 247;
    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_CRC;
    pcie_params->error_data.crc = (PCIE_RASDES_INJ_CRC_TYPE)0x8000000;
    op->error_count = 10;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad internal seqnum error type  */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 247;
    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_SEQNUM;
    pcie_params->error_data.seqnum = (PCIE_RASDES_INJ_SEQNUM_TYPE)0x8000000;
    op->error_count = 10;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad internal dllp error type  */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 247;
    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_DLLP;
    pcie_params->error_data.dllp = (PCIE_RASDES_INJ_DLLP_TYPE)0x8000000;
    op->error_count = 10;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad internal symbol error type  */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 247;
    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_SYMBOL;
    pcie_params->error_data.symbol = (PCIE_RASDES_INJ_SYMBOL_TYPE)0x8000000;
    op->error_count = 10;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad internal fc error type  */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 247;
    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_FC;
    pcie_params->error_data.fc = (PCIE_RASDES_INJ_FC_TYPE)0x8000000;
    op->error_count = 10;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Bad internal retry TLP error type  */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 247;
    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_RETRY_TLP;
    pcie_params->error_data.retry_tlp = (PCIE_RASDES_INJ_RETRY_TLP_TYPE)0x8000000;
    op->error_count = 10;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    status = pcie_error_injection_cb(&mock_einj_info, nullptr);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);

    /* Test dfwk error  */
    mock_einj_info.component_instance = 0;
    mock_einj_info.component_group = ACPI_ERROR_DOMAIN_PCIE;
    pcie_params->sbdf.segment = 0;
    pcie_params->sbdf.bus = 247;
    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_RETRY_TLP;
    pcie_params->error_data.retry_tlp = PCIE_RASDES_INJ_DUPLICATE_TLP;
    op->error_count = 10;
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return(__wrap_scp_pcie_get_manager_context, &ctx);
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, INJECT_PCIE_ERROR);
    will_return(__wrap_DfwkInterfaceSendSync, nullptr);
    will_return(__wrap_DfwkInterfaceSendSync, SILIBS_SUCCESS);
    will_return(__wrap_DfwkInterfaceSendSync, -1);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);
    if (!bugcheck_mock_return())
    {
        pcie_error_injection_cb(&mock_einj_info, nullptr);
    }
}

TEST_FUNCTION(test_ltssm_ssi_events, NULL, NULL)
{
    ssi_startup_notification_request_t ltssm_req;
    cache_ssi_ltssm_startup_request(&ltssm_req);

    bool pci_disabled = scp_pcie_is_disabled();
    assert_int_equal(pci_disabled, false);

    bool ltssm_en_set = scp_is_pcie_ltssm_en_set();
    assert_int_equal(ltssm_en_set, false);

    expect_any(__wrap_DfwkAsyncRequestComplete, Request);
    complete_ssi_ltssm_startup_req(RPSS0);
    complete_ssi_ltssm_startup_req(RPSS1);

    ltssm_en_set = scp_is_pcie_ltssm_en_set();
    assert_int_equal(ltssm_en_set, true);
}
