//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_test.cpp
 * Test for SCP AVS
 */

/*------------- Includes -----------------*/
#include "inc\scp_avs_test_mocks.h" // IWYU pragma: keep

#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <DfwkClient.h>
#include <DfwkDriver.h>
#include <DfwkHost.h>  // for DfwkDeviceInitialize
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <avs_lib.h>   // for AVS_VOLTAGE_RW, AVS_CMD_READ, AVS_IRQ_...
#include <interrupts.h>
#include <padring_southeast_regs.h>
#include <scp_avs.h>
#include <scp_avs_cli.h>
#include <scp_avs_driver.h>
#include <scp_avs_driver_i.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static DFWK_SCHEDULE s_schedule;
static scp_avs_device_t test_avs_device = {
    .config = {.avs_irq = HW_INT_AVS_CTRL_0_INT},
    .avs_bus_num = AVS_BUS0,
};

static scp_avs_interface_t test_avs_interface;
static scp_avs_request_t test_avs_Request;
static scp_avs_get_request_t test_avs_get_Request;
static scp_avs_isr_request_t test_avs_isr_Request;

extern "C" {
extern scp_avs_error_count_t avs_error_count;
}
/*----------- Mock Functions -------------*/

/*------------- Functions ----------------*/
static void test_scp_request_completion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    check_expected(Request);
    check_expected(CompletionContext);
}

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    DfwkDeviceInitialize(&test_avs_device.Header, &s_schedule);

    test_avs_interface.Device = &test_avs_device;

    int32_t result = DfwkClientInterfaceOpen(&test_avs_interface.Header);
    if (DFWK_FAILED(result))
    {
        return result;
    }
    DfwkAsyncRequestInitialize(&test_avs_Request.Header, sizeof(test_avs_Request));
    test_avs_Request.Header.OwningInterface = &test_avs_interface.Header;
    DfwkInterfaceInitialize(&test_avs_interface.Header, &test_avs_device.Header, &test_avs_device.avs_queue, scp_avs_dispatch_sync);

    return 0;
}

static int test_cleanup(void** pContext)
{
    (void)pContext;
    DfwkClientInterfaceClose(&test_avs_interface.Header);
    return 0;
}

TEST_FUNCTION(scp_avs0_driver_init_test, test_setup, test_cleanup)
{
    padring_southeast_southeast_afm_csr_avs0_clk avs_clk = {};
    padring_southeast_southeast_afm_csr_avs0_mdata avs_mdata = {};

    scp_avs_device_t test_avs_device = {
        .config =
            {
                .avs_irq = HW_INT_AVS_CTRL_0_INT,
                .afm_csr_avs_clk_addr = (uintptr_t)&avs_clk,
                .afm_csr_mdata_addr = (uintptr_t)&avs_mdata,
            },
        .avs_bus_num = AVS_BUS0,
    };

    expect_value(__wrap_avs_init, avs_base_addr, test_avs_device.avs_bus_num);

    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_AVS_CTRL_0_INT);

    expect_any(__wrap_nvic_irq_set_isr_with_param, isr);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (uintptr_t)&test_avs_device);

    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_AVS_CTRL_0_INT);

    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_AVS_CTRL_0_INT);

    expect_value(__wrap_avs_enable_interrupt, avs_id, test_avs_device.avs_bus_num);
    expect_value(__wrap_avs_enable_interrupt, intr, AVS_IRQ_CMD_DONE);

    scp_avs_driver_initialize(&test_avs_device);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
    assert_int_equal(avs_clk.ds, 3);
    assert_int_equal(avs_mdata.ds, 3);
}

TEST_FUNCTION(scp_avs1_driver_init_test, test_setup, test_cleanup)
{
    padring_southeast_southeast_afm_csr_avs1_clk avs_clk = {};
    padring_southeast_southeast_afm_csr_avs1_mdata avs_mdata = {};

    scp_avs_device_t test_avs_device = {
        .config =
            {
                .avs_irq = HW_INT_AVS_CTRL_1_INT,
                .afm_csr_avs_clk_addr = (uintptr_t)&avs_clk,
                .afm_csr_mdata_addr = (uintptr_t)&avs_mdata,
            },
        .avs_bus_num = AVS_BUS1,
    };

    expect_value(__wrap_avs_init, avs_base_addr, test_avs_device.avs_bus_num);

    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_AVS_CTRL_1_INT);

    expect_any(__wrap_nvic_irq_set_isr_with_param, isr);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (uintptr_t)&test_avs_device);

    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_AVS_CTRL_1_INT);

    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_AVS_CTRL_1_INT);

    expect_value(__wrap_avs_enable_interrupt, avs_id, test_avs_device.avs_bus_num);
    expect_value(__wrap_avs_enable_interrupt, intr, AVS_IRQ_CMD_DONE);

    scp_avs_driver_initialize(&test_avs_device);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS1);
    assert_int_equal(avs_clk.ds, 3);
    assert_int_equal(avs_mdata.ds, 3);
}

TEST_FUNCTION(scp_avs2_driver_init_test, test_setup, test_cleanup)
{
    padring_southeast_southeast_afm_csr_avs2_clk avs_clk = {};
    padring_southeast_southeast_afm_csr_avs2_mdata avs_mdata = {};

    scp_avs_device_t test_avs_device = {
        .config =
            {
                .avs_irq = HW_INT_AVS_CTRL_2_INT,
                .afm_csr_avs_clk_addr = (uintptr_t)&avs_clk,
                .afm_csr_mdata_addr = (uintptr_t)&avs_mdata,
            },
        .avs_bus_num = AVS_BUS2,
    };

    expect_value(__wrap_avs_init, avs_base_addr, test_avs_device.avs_bus_num);

    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_AVS_CTRL_2_INT);

    expect_any(__wrap_nvic_irq_set_isr_with_param, isr);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (uintptr_t)&test_avs_device);

    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_AVS_CTRL_2_INT);

    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_AVS_CTRL_2_INT);

    expect_value(__wrap_avs_enable_interrupt, avs_id, test_avs_device.avs_bus_num);
    expect_value(__wrap_avs_enable_interrupt, intr, AVS_IRQ_CMD_DONE);

    scp_avs_driver_initialize(&test_avs_device);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS2);
    assert_int_equal(avs_clk.ds, 3);
    assert_int_equal(avs_mdata.ds, 3);
}

TEST_FUNCTION(scp_avs3_driver_init_test, test_setup, test_cleanup)
{
    padring_southeast_southeast_afm_csr_avs3_clk avs_clk = {};
    padring_southeast_southeast_afm_csr_avs3_mdata avs_mdata = {};

    scp_avs_device_t test_avs_device = {
        .config =
            {
                .avs_irq = HW_INT_AVS_CTRL_3_INT,
                .afm_csr_avs_clk_addr = (uintptr_t)&avs_clk,
                .afm_csr_mdata_addr = (uintptr_t)&avs_mdata,
            },
        .avs_bus_num = AVS_BUS3,
    };

    expect_value(__wrap_avs_init, avs_base_addr, test_avs_device.avs_bus_num);

    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_AVS_CTRL_3_INT);

    expect_any(__wrap_nvic_irq_set_isr_with_param, isr);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (uintptr_t)&test_avs_device);

    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_AVS_CTRL_3_INT);

    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_AVS_CTRL_3_INT);

    expect_value(__wrap_avs_enable_interrupt, avs_id, test_avs_device.avs_bus_num);
    expect_value(__wrap_avs_enable_interrupt, intr, AVS_IRQ_CMD_DONE);

    scp_avs_driver_initialize(&test_avs_device);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS3);
    assert_int_equal(avs_clk.ds, 3);
    assert_int_equal(avs_mdata.ds, 3);
}

TEST_FUNCTION(scp_avs_interface_init_test, test_setup, test_cleanup)
{
    scp_avs_interface_initialize(&test_avs_device, &test_avs_interface);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

TEST_FUNCTION(scp_avs_client_read_test, test_setup, test_cleanup)
{
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_read(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_READ_DATA);
}

TEST_FUNCTION(scp_avs_client_write_test, test_setup, test_cleanup)
{
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_write(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_WRITE_DATA);
}

TEST_FUNCTION(scp_avs_client_read_all_test, test_setup, test_cleanup)
{
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_read_all(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_READ_ALL_VCT);
}

TEST_FUNCTION(scp_avs_client_read_multi_test, test_setup, test_cleanup)
{
    uint8_t test_count = 3;

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_read_multi(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr, test_count);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_READ_MULTI);
}

TEST_FUNCTION(scp_avs_client_write_multi_test, test_setup, test_cleanup)
{
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_write_multi(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_WRITE_MULTI);
}

TEST_FUNCTION(scp_avs_dispatch_test_read, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS0;

    test_avs_Request.avs_params.avs_cmd_array[0].cmd_type = AVS_VOLTAGE_RW;
    test_avs_Request.avs_params.avs_cmd_array[0].rail_id = 1;
    test_avs_Request.Header.RequestType = AVS_REQUEST_READ_DATA;

    expect_value(__wrap_avs_send_cmd_frame, avs_id, AVS_BUS0);
    expect_value(__wrap_avs_send_cmd_frame, cmd_num, 1);

    avs_master_command_t expected_master_command = {
        .command_data_type = AVS_VOLTAGE_RW,
        .command_type = 1, // rail id
        .command_control = AVS_CMD_READ,
        .command_group = AVS_CGROUP,
    };
    expect_memory(__wrap_avs_send_cmd_frame, cmd_mem, &expected_master_command, sizeof(expected_master_command));

    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

TEST_FUNCTION(scp_avs_isr_dispatch_test_read, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS0;
    test_avs_isr_Request.outstanding_client_request = &test_avs_Request;

    test_avs_isr_Request.outstanding_client_request->Header.RequestType = AVS_REQUEST_READ_DATA;

    expect_value(__wrap_avs_get_cmd_resp_data, avs_base_or_id, test_avs_device.avs_bus_num);
    expect_value(__wrap_avs_get_cmd_resp_data, resp_idx, 0);
    expect_value(__wrap_avs_get_cmd_resp_data, resp_num, 1);

    expect_any(__wrap_avs_get_cmd_resp_data, resp_buf);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_avs_device.outstanding_request->Header);

    scp_avs_isr_dispatch(&test_avs_isr_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
}

TEST_FUNCTION(scp_avs_isr_dispatch_test_read_all, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS0;
    test_avs_isr_Request.outstanding_client_request = &test_avs_Request;

    test_avs_isr_Request.outstanding_client_request->Header.RequestType = AVS_REQUEST_READ_ALL_VCT;

    expect_value(__wrap_avs_get_cmd_resp_data, avs_base_or_id, test_avs_device.avs_bus_num);
    expect_value(__wrap_avs_get_cmd_resp_data, resp_idx, 0);
    expect_value(__wrap_avs_get_cmd_resp_data, resp_num, 3); // should be 3 for 3 values read (VCT)

    expect_any(__wrap_avs_get_cmd_resp_data, resp_buf);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_avs_device.outstanding_request->Header);

    scp_avs_isr_dispatch(&test_avs_isr_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
}

TEST_FUNCTION(scp_avs_isr_dispatch_test_read_multi, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS1;
    test_avs_isr_Request.outstanding_client_request = &test_avs_Request;
    test_avs_Request.avs_params.cmd_count = 2;

    test_avs_isr_Request.outstanding_client_request->Header.RequestType = AVS_REQUEST_READ_MULTI;

    expect_value(__wrap_avs_get_cmd_resp_data, avs_base_or_id, test_avs_device.avs_bus_num);
    expect_value(__wrap_avs_get_cmd_resp_data, resp_idx, 0);
    expect_value(__wrap_avs_get_cmd_resp_data, resp_num, 2);

    expect_any(__wrap_avs_get_cmd_resp_data, resp_buf);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_avs_device.outstanding_request->Header);

    scp_avs_isr_dispatch(&test_avs_isr_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
}

TEST_FUNCTION(scp_avs_isr_dispatch_test_write, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS0;
    test_avs_isr_Request.outstanding_client_request = &test_avs_Request;

    test_avs_isr_Request.outstanding_client_request->Header.RequestType = AVS_REQUEST_WRITE_DATA;

    expect_value(__wrap_avs_get_cmd_resp_data, avs_base_or_id, test_avs_device.avs_bus_num);
    expect_value(__wrap_avs_get_cmd_resp_data, resp_idx, 0);
    expect_value(__wrap_avs_get_cmd_resp_data, resp_num, 2); // should be 2, since two commands sent.  The first the write the second the read.

    expect_any(__wrap_avs_get_cmd_resp_data, resp_buf);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &test_avs_device.outstanding_request->Header);

    scp_avs_isr_dispatch(&test_avs_isr_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

TEST_FUNCTION(scp_avs_dispatch_test_write, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS0;
    test_avs_Request.avs_params.avs_cmd_array[0].cmd_type = AVS_VOLTAGE_RW;
    test_avs_Request.avs_params.avs_cmd_array[0].rail_id = 1;
    test_avs_Request.avs_params.avs_data = 995;
    test_avs_Request.Header.RequestType = AVS_REQUEST_WRITE_DATA;

    expect_value(__wrap_avs_send_cmd_frame, avs_id, AVS_BUS0);
    expect_value(__wrap_avs_send_cmd_frame, cmd_num, 2);

    avs_master_command_t avs_test_buffer[2] = {};

    avs_test_buffer[0].command_data = 995;
    avs_test_buffer[0].command_data_type = AVS_VOLTAGE_RW;
    avs_test_buffer[0].command_type = 1; // rail ID
    avs_test_buffer[0].command_control = AVS_CMD_WRITE_COMMIT;
    avs_test_buffer[0].command_group = AVS_CGROUP;
    avs_test_buffer[1].command_data_type = AVS_VOLTAGE_RW;
    avs_test_buffer[1].command_type = 1;
    avs_test_buffer[1].command_group = AVS_CGROUP;
    avs_test_buffer[1].command_control = AVS_CMD_READ;

    expect_memory(__wrap_avs_send_cmd_frame, cmd_mem, avs_test_buffer, sizeof(avs_test_buffer));

    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

TEST_FUNCTION(scp_avs_dispatch_test_read_all, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS0;
    test_avs_Request.avs_params.avs_cmd_array[0].rail_id = 0;
    test_avs_Request.Header.RequestType = AVS_REQUEST_READ_ALL_VCT;
    test_avs_Request.avs_params.cmd_count = 3;

    expect_value(__wrap_avs_send_cmd_frame, avs_id, AVS_BUS0);
    expect_value(__wrap_avs_send_cmd_frame, cmd_num, 3); // 3, since 3 commands to be processed.

    avs_master_command_t avs_test_buffer[3] = {};

    avs_test_buffer[0].command_data_type = AVS_VOLTAGE_RW;
    avs_test_buffer[0].command_type = 0; // rail ID
    avs_test_buffer[0].command_control = AVS_CMD_READ;
    avs_test_buffer[0].command_group = AVS_CGROUP;
    avs_test_buffer[1].command_data_type = AVS_CURRENT_READ;
    avs_test_buffer[1].command_type = 0;
    avs_test_buffer[1].command_group = AVS_CGROUP;
    avs_test_buffer[1].command_control = AVS_CMD_READ;
    avs_test_buffer[2].command_data_type = AVS_TEMPERATURE_READ;
    avs_test_buffer[2].command_type = 0;
    avs_test_buffer[2].command_group = AVS_CGROUP;
    avs_test_buffer[2].command_control = AVS_CMD_READ;

    expect_memory(__wrap_avs_send_cmd_frame, cmd_mem, avs_test_buffer, sizeof(avs_test_buffer));

    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

TEST_FUNCTION(scp_avs_dispatch_test_read_multi, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS0;
    test_avs_Request.avs_params.avs_cmd_array[0].rail_id = 0;
    test_avs_Request.avs_params.avs_cmd_array[0].cmd_type = AVS_VOLTAGE_RW;
    test_avs_Request.avs_params.avs_cmd_array[1].rail_id = 1;
    test_avs_Request.avs_params.avs_cmd_array[1].cmd_type = AVS_CURRENT_READ;
    test_avs_Request.avs_params.cmd_count = 2;
    test_avs_Request.Header.RequestType = AVS_REQUEST_READ_MULTI;

    expect_value(__wrap_avs_send_cmd_frame, avs_id, AVS_BUS0);
    expect_value(__wrap_avs_send_cmd_frame, cmd_num, 2); // sending 2 commands for test

    avs_master_command_t avs_test_buffer[2] = {};

    avs_test_buffer[0].command_data_type = AVS_VOLTAGE_RW;
    avs_test_buffer[0].command_type = 0; // rail ID
    avs_test_buffer[0].command_control = AVS_CMD_READ;
    avs_test_buffer[0].command_group = AVS_CGROUP;
    avs_test_buffer[1].command_data_type = AVS_CURRENT_READ;
    avs_test_buffer[1].command_type = 1;
    avs_test_buffer[1].command_group = AVS_CGROUP;
    avs_test_buffer[1].command_control = AVS_CMD_READ;

    expect_memory(__wrap_avs_send_cmd_frame, cmd_mem, avs_test_buffer, sizeof(avs_test_buffer));

    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

TEST_FUNCTION(scp_avs_dispatch_test_write_multi, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS0;
    test_avs_Request.avs_params.avs_cmd_array[0].rail_id = 0;
    test_avs_Request.avs_resp_multi.avs_response_multi[0].data = 900;
    test_avs_Request.avs_params.avs_cmd_array[1].rail_id = 1;
    test_avs_Request.avs_resp_multi.avs_response_multi[1].data = 902;
    test_avs_Request.avs_params.avs_cmd_array[1].cmd_type = AVS_CURRENT_READ;
    test_avs_Request.Header.RequestType = AVS_REQUEST_WRITE_MULTI;

    expect_value(__wrap_avs_send_cmd_frame, avs_id, AVS_BUS0);
    expect_value(__wrap_avs_send_cmd_frame, cmd_num, MAX_AVS_RAILS); // sending 2 commands, one for each rail read

    avs_master_command_t avs_test_buffer[2] = {};

    avs_test_buffer[0].command_data_type = AVS_VOLTAGE_RW;
    avs_test_buffer[0].command_type = 0; // rail ID
    avs_test_buffer[0].command_control = AVS_CMD_WRITE_COMMIT;
    avs_test_buffer[0].command_group = AVS_CGROUP;
    avs_test_buffer[0].command_data = 900;
    avs_test_buffer[1].command_data_type = AVS_VOLTAGE_RW;
    avs_test_buffer[1].command_type = 1; // rail ID
    avs_test_buffer[1].command_group = AVS_CGROUP;
    avs_test_buffer[1].command_data = 902;
    avs_test_buffer[1].command_control = AVS_CMD_WRITE_COMMIT;

    expect_memory(__wrap_avs_send_cmd_frame, cmd_mem, avs_test_buffer, sizeof(avs_test_buffer));
    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

TEST_FUNCTION(scp_avs_dispatch_sync, test_setup, test_cleanup)
{
    test_avs_get_Request.Header.RequestType = AVS_GET_ERROR_COUNTS;

    scp_avs_dispatch_sync(&test_avs_get_Request.Header);
    assert_int_equal(test_avs_get_Request.Header.RequestType, AVS_GET_ERROR_COUNTS);
}

TEST_FUNCTION(scp_avs_isr_test, test_setup, test_cleanup)
{
    scp_avs_device_t test_avs_isr_device = {
        .config = {.avs_irq = HW_INT_AVS_CTRL_1_INT},
        .avs_bus_num = AVS_BUS1,
    };

    expect_value(__wrap_avs_get_interrupt_status, avs_base_or_id, test_avs_isr_device.avs_bus_num);

    expect_value(__wrap_avs_clear_interrupt_status, avs_base_or_id, test_avs_isr_device.avs_bus_num);
    expect_value(__wrap_avs_clear_interrupt_status, intr, AVS_IRQ_CMD_DONE);

    expect_value(__wrap_DfwkQueueEnqueueRequest, Queue, &test_avs_isr_device.avs_isr_resp_queue);
    expect_value(__wrap_DfwkQueueEnqueueRequest, Request, &test_avs_isr_device.isr_request);

    scp_avs_isr(&test_avs_isr_device);
    assert_int_equal(test_avs_isr_device.isr_request.outstanding_client_request, test_avs_isr_device.outstanding_request);
}

TEST_FUNCTION(scp_avs_get_error_counts_test, test_setup, test_cleanup)
{
    scp_avs_error_count_t error_count_test = {0};
    avs_error_count = {0};

    avs_get_error_counts(&error_count_test);

    assert_int_equal(error_count_test.ack_no_action_busy_error_count, 0);
    assert_int_equal(error_count_test.ack_bad_crc_no_action_error_count, 0);
    assert_int_equal(error_count_test.ack_invalid_no_action_error_count, 0);
    assert_int_equal(error_count_test.status_alert_error_count, 0);
}

TEST_FUNCTION(scp_avs_status_error_test, test_setup, test_cleanup)
{
    scp_avs_error_count_t error_status_count = {0};
    avs_error_count = {0};

    uint32_t resp_error = AVS_ERROR_NONE;
    avs_error_t result_error = {.as_uint8 = AVS_ERROR_NONE};

    result_error = scp_avs_status_error(resp_error);
    assert_int_equal(result_error.as_uint8, AVS_NO_CONTROL);

    resp_error = (AVS_STATUS_MASK << AVS_VDONE_SHIFT);
    result_error = scp_avs_status_error(resp_error);
    assert_int_equal(result_error.as_uint8, (AVS_VDONE | AVS_NO_CONTROL));

    resp_error = (AVS_STATUS_MASK << AVS_ERR_STAT_ALERT_SHIFT);
    result_error = scp_avs_status_error(resp_error);
    assert_int_equal(result_error.as_uint8, (AVS_ERROR_STATUS_ALERT | AVS_NO_CONTROL));
    avs_get_error_counts(&error_status_count);
    assert_int_equal(error_status_count.status_alert_error_count, 1);

    resp_error = (0x1 << AVS_ERR_ACK_SHIFT);
    result_error = scp_avs_status_error(resp_error);
    assert_int_equal(result_error.as_uint8, (AVS_ERROR_ACK_NO_ACTION_BUSY | AVS_NO_CONTROL));
    avs_get_error_counts(&error_status_count);
    assert_int_equal(error_status_count.ack_no_action_busy_error_count, 1);
    result_error = scp_avs_status_error(resp_error); // inject the error again, to confirm count = 2
    assert_int_equal(result_error.as_uint8, (AVS_ERROR_ACK_NO_ACTION_BUSY | AVS_NO_CONTROL));
    avs_get_error_counts(&error_status_count);
    assert_int_equal(error_status_count.ack_no_action_busy_error_count, 2);

    resp_error = (0x2 << (AVS_ERR_ACK_SHIFT));
    result_error = scp_avs_status_error(resp_error);
    assert_int_equal(result_error.as_uint8, (AVS_ERROR_ACK_BAD_CRC_NO_ACTION | AVS_NO_CONTROL));
    avs_get_error_counts(&error_status_count);
    assert_int_equal(error_status_count.ack_bad_crc_no_action_error_count, 1);

    resp_error = (0x3 << (AVS_ERR_ACK_SHIFT));
    result_error = scp_avs_status_error(resp_error);
    assert_int_equal(result_error.as_uint8, (AVS_ERROR_ACK_INVALID_NO_ACTION | AVS_NO_CONTROL));
    avs_get_error_counts(&error_status_count);
    assert_int_equal(error_status_count.ack_invalid_no_action_error_count, 1);
}
