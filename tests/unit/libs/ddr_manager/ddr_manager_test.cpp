//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_test.cpp
 * DDR Manager tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h>     // for FPFW_UNUSED
#include <ddr_manager.h>   // for ddr_manager_init, ddr_service_context_t
#include <ddr_manager_i.h> // for ddr_poll_dimms, ddr_worker_thread_func
#include <error_handler.h> // for set_error_handler_return
#include <fpfw_icc_base.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <kingsgate_hsp_mailbox_commands.h>
#include <mscp_exp_rmss_memory_map.h>
#include <tx_api.h> // for TX_SUCCESS, ULONG, TX_NOT_DONE, TX_NO_MEMORY

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
#define UNUSED(x) (void)(x)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* icc_ctx;

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit, UINT mutex_control_block_size)
{
    assert_non_null(mutex_ptr); // Ensure the mutex pointer is not NULL

    assert_non_null(name_ptr);
    FPFW_UNUSED(inherit);
    FPFW_UNUSED(mutex_control_block_size);

    function_called();

    return 0;
}

//! Mocks for mailbox primitives called inside hsp_send_ddr_init_notify()
fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(buffer_size);
    check_expected_ptr(payload_buffer);
    check_expected_ptr(output_recv_bytes);

    kng_hsp_mailbox_msg* msg = (kng_hsp_mailbox_msg*)payload_buffer;
    if (msg->header.cmd == HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY)
    {
        msg->header.cmd = HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY_RSP;
        msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
        *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);
    }

    kng_hsp_cmd_load_fw_mailbox_msg* msg2 = (kng_hsp_cmd_load_fw_mailbox_msg*)payload_buffer;
    if (msg2->load_fw_req.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_REQ)
    {
        msg2->load_fw_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_RSP;
        msg2->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
        *output_recv_bytes = sizeof(kng_hsp_cmd_load_fw_mailbox_msg);
        ;
    }
    return mock_type(fpfw_status_t);
}

bool __wrap_system_info_is_hsp_present()
{
    return true;
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

static int setup_rvp_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_RVP_EVT_SILICON);
    return 0;
}
static int setup_undefined_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_UNDEFINED);
    return 0;
}

} // extern "C"

//
// Tests
//
TEST_FUNCTION(ddr_manager_init_fail, NULL, NULL)
{
    ddr_service_context_t ddr_service_context = {};
    ddr_service_config_t ddr_service_config = {};

    // tx queue fails
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
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    // tx queue send DDR MEMORY MAP EVENT fails
    expect_any_always(__wrap__txe_queue_send, queue_ptr);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_any_always(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_ERROR);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // tx queue send DDR BDAT EVENT fails
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_ERROR);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // tx queue send DDR SMBIOS EVENT fails
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_ERROR);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // tx thread create fails
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
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    // tx timer create fails
    will_return(__wrap__txe_timer_create, TX_TIMER_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)(TX_TIMER_ERROR));

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }
}

#define DDR_TEST_STACK_SIZE             (1024)
#define DDR_TEST_THREAD_PRIORITY        (10)
#define DDR_TEST_TIMER_INITIAL_TICKS    (10)
#define DDR_TEST_TIMER_RESCHEDULE_TICKS (15)
TEST_FUNCTION(ddr_manager_init_check_params, NULL, NULL)
{
    uint8_t ddr_test_stack[DDR_TEST_STACK_SIZE];
    static uint32_t ddr_queue_pool[10];
    static ddr_service_context_t ddr_service_ctx = {};

    ddr_service_config_t config = {.thread_config =
                                       {
                                           .p_stack = ddr_test_stack,
                                           .stack_size = sizeof(ddr_test_stack),
                                           .priority = DDR_TEST_THREAD_PRIORITY,
                                           .time_slice_option = TX_NO_TIME_SLICE,
                                       },
                                   .timer_config =
                                       {
                                           .initial_ticks = DDR_TEST_TIMER_INITIAL_TICKS,
                                           .reschedule_ticks = DDR_TEST_TIMER_RESCHEDULE_TICKS,
                                       },
                                   .queue_config = {
                                       .p_queue = ddr_queue_pool,
                                       .msg_size = sizeof(ddr_queue_pool[0]) / sizeof(uint32_t),
                                       .queue_num_words = sizeof(ddr_queue_pool) / sizeof(uint32_t),
                                   }};

    expect_value(__wrap__txe_queue_create, queue_ptr, &ddr_service_ctx.work_queue);
    expect_value(__wrap__txe_queue_create, name_ptr, DDR_WORK_QUEUE_NAME);
    expect_value(__wrap__txe_queue_create, message_size, config.queue_config.msg_size);
    expect_value(__wrap__txe_queue_create, queue_start, config.queue_config.p_queue);
    expect_value(__wrap__txe_queue_create, queue_size, config.queue_config.queue_num_words * sizeof(uint32_t));
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    expect_value(__wrap__txe_thread_create, thread_ptr, &ddr_service_ctx.work_thread);
    expect_value(__wrap__txe_thread_create, name_ptr, DDR_WORK_THREAD_NAME);
    expect_value(__wrap__txe_thread_create, entry_function, ddr_worker_thread_func);
    expect_value(__wrap__txe_thread_create, entry_input, (ULONG)&ddr_service_ctx);
    expect_value(__wrap__txe_thread_create, stack_start, config.thread_config.p_stack);
    expect_value(__wrap__txe_thread_create, stack_size, config.thread_config.stack_size);
    expect_value(__wrap__txe_thread_create, priority, config.thread_config.priority);
    expect_value(__wrap__txe_thread_create, preempt_threshold, config.thread_config.priority);
    expect_value(__wrap__txe_thread_create, time_slice, config.thread_config.time_slice_option);
    expect_value(__wrap__txe_thread_create, auto_start, TX_DONT_START);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    size_t output_recv_bytes = 0;
    kng_hsp_mailbox_msg msg = {.header = {.cmd = HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY}};

    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, payload_buffer, &msg, sizeof(msg));
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, output_recv_bytes, &output_recv_bytes, sizeof(output_recv_bytes));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Telemetry init
    expect_function_call(__wrap__txe_mutex_create);

    ddr_manager_init(&ddr_service_ctx, &config, icc_ctx);
}

TEST_FUNCTION(ddr_timer_cb_success, NULL, NULL)
{
    ddr_service_context_t ddr_service_ctx = {};

    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    ddr_timer_cb((ULONG)&ddr_service_ctx);
}

TEST_FUNCTION(ddr_worker_thread_func_success, NULL, NULL)
{
    ddr_service_context_t ddr_service_ctx = {};
    ddr_service_ctx.work_queue.tx_queue_start = (ULONG*)0x1234;

    expect_value(__wrap__txe_queue_receive, queue_ptr, &(ddr_service_ctx.work_queue));
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, (ULONG)TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_ERROR);

    expect_value(FPFwErrorRaise, error, (uint32_t)(TX_QUEUE_ERROR));

    if (!set_error_handler_return())
    {
        ddr_worker_thread_func((ULONG)&ddr_service_ctx);
    }
}

// Add coverage to functions that currently do nothing
// TODO: Replace this test as these functions begin to get written
TEST_FUNCTION(ddr_worker_thread_func_test_message_types, NULL, NULL)
{
    ddr_create_bdat();
    ddr_create_smbios_tables();
}

TEST_FUNCTION(ddr_manager_load_PHY_bin, setup_rvp_platform, setup_undefined_platform)
{
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    kng_hsp_cmd_load_fw_mailbox_msg msg = {
        .load_fw_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .load_fw_req.id = HSP_FIRMWARE_ID_DDR_PHY,
        .load_fw_req.address = SCP_EXP_DDR_PHY_DATA_BASE,
        .load_fw_req.size = SCP_EXP_DDR_PHY_DATA_SIZE,
    };

    size_t output_recv_bytes = 0;
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, payload_buffer, &msg, sizeof(msg));
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, output_recv_bytes, &output_recv_bytes, sizeof(output_recv_bytes));

    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    hsp_send_recv_load_fw_ddr_phy_req(icc_ctx);
}