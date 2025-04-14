// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_dma_mscp.cpp
 * DMA controller tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <DfwkCommon.h>
#include <FPFwInterrupts.h>
#include <FpFwUtils.h>
#include <dma_dfwk.h>
#include <dma_private.h>
#include <dmac.h>
#include <error_handler.h> // for set_error_handler_return
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <nvic.h>
#include <silibs_ap_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DMA_TEST_BASE_ADDR  0x12345678
#define FPFW_LOCK_SIGNATURE 0xfeedf00d

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
// Declared in dma_mocks.c
extern uint32_t g_intu_sts;
extern bool g_mmio_read32_mocktype;
extern bool g_mmio_read64_mocktype;
extern bool g_FpFwLockAcquire_check_signature;
extern fpfw_init_component_t _fpfw_component_dma;

static dma_device_t device;
static dma_config_t config = {0};
static DFWK_SCHEDULE schedule = {0};

/*------------- Functions ----------------*/
static int setup(void** state)
{
    FPFW_UNUSED(state);
    g_intu_sts = 0;
    g_mmio_read32_mocktype = false;
    g_mmio_read64_mocktype = false;
    g_FpFwLockAcquire_check_signature = false;

    memset(&device, 0, sizeof(device));
    config = {0};
    schedule = {0};
    device.config = &config;

    device.Channel[CH_0].current_request = NULL;
    device.Channel[CH_1].current_request = NULL;

    return 0;
}

static int teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

//
// Tests
//

TEST_FUNCTION(test_scp_dma_device_init_polling_pass, setup, teardown)
{
    device.config->base_address = DMA_TEST_BASE_ADDR;
    device.config->config_type = DMA_CONFIG_TYPE_POLLING;

    // Arrange
    config.base_address = (uintptr_t)DMA_TEST_BASE_ADDR;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_count(__wrap__txe_timer_create, TX_SUCCESS, 2);

    // DfwkDeviceInitialize
    expect_value(__wrap_DfwkDeviceInitialize, Device, &device);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &schedule);

    // Act
    dma_device_init(&device, &config, &schedule);

    // Assert
    assert_ptr_not_equal(device.config, NULL);
    assert_int_not_equal(device.config->base_address, DMA_TEST_BASE_ADDR); // Bogus value
    assert_int_equal(device.config->base_address, DMAC_SCP_BASE_ADDR);     // Defined by SiLibs
}

TEST_FUNCTION(test_scp_dma_device_init_interrupt_pass, setup, teardown)
{
    device.config->base_address = DMA_TEST_BASE_ADDR;
    device.config->config_type = DMA_CONFIG_TYPE_INTERRUPT;

    // Arrange
    config.base_address = (uintptr_t)DMA_TEST_BASE_ADDR;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    // dma_enable_nvic_interrupts
    device_identifier_pair_t device_interrupt_pair[NUM_DMA_INTERRUPTS] = {};
    for (int this_irq_num = HW_INT_SCP_DMAC_CH0_INT; this_irq_num <= HW_INT_SCP_DMAC_ATU_PARITY_INT; this_irq_num++)
    {
        device_interrupt_pair[this_irq_num - HW_INT_SCP_DMAC_CH0_INT].device = &device;
        device_interrupt_pair[this_irq_num - HW_INT_SCP_DMAC_CH0_INT].interrupt_id = this_irq_num;

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, dma_isr);

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }

    // DfwkDeviceInitialize
    expect_value(__wrap_DfwkDeviceInitialize, Device, &device);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &schedule);

    // Act
    dma_device_init(&device, &config, &schedule);

    // Assert
    assert_ptr_not_equal(device.config, NULL);
    assert_int_not_equal(device.config->base_address, DMA_TEST_BASE_ADDR); // Bogus value
    assert_int_equal(device.config->base_address, DMAC_SCP_BASE_ADDR);     // Defined by SiLibs
}

TEST_FUNCTION(test_dma_interface_init, setup, teardown)
{
    // Arrange
    dma_interface_t Interface = {};
    device.config = &config;
    device.config->base_address = DMA_TEST_BASE_ADDR;
    device.config->config_type = DMA_CONFIG_TYPE_INTERRUPT;

    // Act
    dma_interface_init(&device, &Interface);

    // Assert
    assert_ptr_equal(Interface.Header.OwningDevice, &device);
    assert_ptr_equal(Interface.Header.DispatchQueue, &device.Request_Queue);
    assert_null(Interface.Header.ClientDevice);
    assert_ptr_equal(Interface.Device, &device);
}

TEST_FUNCTION(test_dma_lib_init, setup, teardown)
{
    // Arrange
    device.config->base_address = (uintptr_t)DMA_TEST_BASE_ADDR;

    // Act
    dma_lib_init(&device);

    // Assert nothing here - all functions within are mocked as they write to hardware
}

/* This tests move_request_to_channel_queue will update the channel's total outstanding byte count */
TEST_FUNCTION(test_dma_move_request_to_channel_queue, setup, teardown)
{
    // Arrange
    device.Channel[CH_0].current_request = NULL;
    device.Channel[CH_1].current_request = NULL;
    device.Channel[CH_1].remaining_bytes_all_requests = 20;

    dma_async_request_t TestRequest = {};
    TestRequest.input.dest_addr_32b_aligned = 0x12345600;
    TestRequest.input.src_addr_32b_aligned = 0x87654300;
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.dma_private.assigned_channel = CH_1;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;

    // Act
    expect_value(__wrap_DfwkQueueEnqueueRequest, Queue, &device.Channel[CH_1].queue);
    expect_value(__wrap_DfwkQueueEnqueueRequest, Request, &TestRequest);
    move_request_to_channel_queue(&device, &TestRequest);

    // Assert
    assert_int_equal(device.Channel[CH_1].remaining_bytes_all_requests, 30);
}

TEST_FUNCTION(test_check_parameters_pass, setup, teardown)
{
    // Arrange
    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = 0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600;
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.dma_private.assigned_channel = CH_1;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;

    // Act
    bool result = check_parameters(&device, &TestRequest);

    // Assert
    assert_true(result);
}

TEST_FUNCTION(test_check_parameters_fail_src_addr_zero, setup, teardown)
{
    // Arrange
    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = 0;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600;
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.dma_private.assigned_channel = CH_1;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;

    // Act
    bool result = check_parameters(&device, &TestRequest);

    // Assert
    assert_false(result);
}

TEST_FUNCTION(test_check_parameters_fail_dest_addr_zero, setup, teardown)
{
    // Arrange
    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = 0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0;
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.dma_private.assigned_channel = CH_1;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;

    // Act
    bool result = check_parameters(&device, &TestRequest);

    // Assert
    assert_false(result);
}

TEST_FUNCTION(test_check_parameters_fail_byte_count_zero, setup, teardown)
{
    // Arrange
    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = 0x87654321;
    TestRequest.input.dest_addr_32b_aligned = 0x12345678;
    TestRequest.input.byte_count = 0;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.dma_private.assigned_channel = CH_1;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;

    // Act
    bool result = check_parameters(&device, &TestRequest);

    // Assert
    assert_false(result);
}

TEST_FUNCTION(test_check_parameters_fail_null_request, setup, teardown)
{
    // Arrange
    pdma_async_request_t pTestRequest = NULL;

    // Act
    bool result = check_parameters(&device, pTestRequest);

    // Assert
    assert_false(result);
}

TEST_FUNCTION(test_dma_main_queue_dispatch_bad_parameters, setup, teardown)
{
    // Arrange
    device.config->cpu_type = CPU_SCP;

    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = 0x87654321;
    TestRequest.input.dest_addr_32b_aligned = 0; // Invalid parameter
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.dma_private.assigned_channel = CH_1;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;

    // Test: dest_addr_32b_aligned is 0
    TestRequest.input.dest_addr_32b_aligned = 0;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &TestRequest.header);

    // Act
    dma_main_queue_dispatch(&TestRequest.header, &device);

    // Assert
    assert_int_equal(TestRequest.status, FPFW_DMA_STATUS_FAIL);
}

TEST_FUNCTION(test_dma_main_queue_dispatch_unmapped_atu_dest_addr, setup, teardown)
{
    // Arrange
    device.config->cpu_type = CPU_SCP;

    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = 0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0; // Invalid parameter
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.dma_private.assigned_channel = CH_1;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;

    TestRequest.input.dest_addr_32b_aligned = 0x20000000000;

    // atu_translate_address mock values
    will_return(__wrap_atu_translate_address, 0xDEADBEEF);
    will_return(__wrap_atu_translate_address, SILIBS_E_PARAM);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &TestRequest.header);

    // Act
    dma_main_queue_dispatch(&TestRequest.header, &device);

    // Assert
    assert_int_equal(TestRequest.status, FPFW_DMA_STATUS_FAIL);
}

TEST_FUNCTION(test_dma_main_queue_dispatch_good_atu_dest_addr, setup, teardown)
{
    // Arrange
    device.config->cpu_type = CPU_SCP;
    device.Channel[CH_0].remaining_bytes_all_requests = 5; // CH_0 has less bytes queued than CH_1
    device.Channel[CH_1].remaining_bytes_all_requests = 20;

    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = 0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600;
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;

    TestRequest.input.dest_addr_32b_aligned = 0x30000000000;

    // atu_translate_address mock values
    will_return(__wrap_atu_translate_address, 0xDEADBEEF);
    will_return(__wrap_atu_translate_address, SILIBS_SUCCESS);

    expect_value(__wrap_DfwkQueueEnqueueRequest, Queue, &device.Channel[CH_0].queue);
    expect_value(__wrap_DfwkQueueEnqueueRequest, Request, &TestRequest);

    // Act
    dma_main_queue_dispatch(&TestRequest.header, &device);

    // Assert
    assert_int_equal(TestRequest.dma_private.translated_mscp_atu_address, 0xDEADBEEF);
    assert_int_equal(TestRequest.status, FPFW_DMA_STATUS_QUEUED);
    assert_int_equal(device.Channel[CH_0].remaining_bytes_all_requests, 15);
}

TEST_FUNCTION(test_dma_channel_0_queue_dispatch_pass_starts_polling_timer, setup, teardown)
{
    // Arrange
    device.config->config_type = DMA_CONFIG_TYPE_POLLING;
    device.config->cpu_type = CPU_SCP;
    device.config->base_address = DMAC_SCP_BASE_ADDR;

    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = (uint64_t)0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600; // not used in this test
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;
    TestRequest.dma_private.translated_mscp_atu_address = (uint64_t)0xC0FFEE;

    dmac_transfer_cfg_t dma_tr_cfg = {};
    dmac_cfg_t dma_cfg = {};
    dma_tr_cfg.channel_id = DMAC_CHANNEL0;
    dma_tr_cfg.dmac_src_addr = TestRequest.input.src_addr_32b_aligned;
    dma_tr_cfg.dmac_dest_addr = TestRequest.dma_private.translated_mscp_atu_address;
    dma_tr_cfg.transfer_size = TestRequest.input.byte_count;
    dma_cfg.dmac_transfer_cfg = &dma_tr_cfg;
    dma_cfg.max_block_ts_bytes = DMAC_MAX_BLOCK_TS_BYTES;
    dma_cfg.dmac_src_tr_width = DMAC_TRANSFER_WIDTH_32_BITS;
    dma_cfg.dmac_dest_tr_width = DMAC_TRANSFER_WIDTH_32_BITS;

    // Polling timer
    will_return(__wrap__txe_timer_activate, TX_SUCCESS);

    expect_value(__wrap_dmac_start_single_block_transfer, dmac_base_addr, DMAC_SCP_BASE_ADDR);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_dest_tr_width, dma_cfg.dmac_dest_tr_width);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_src_tr_width, dma_cfg.dmac_src_tr_width);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->max_block_ts_bytes, dma_cfg.max_block_ts_bytes);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_transfer_cfg->channel_id, dma_tr_cfg.channel_id);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_transfer_cfg->dmac_dest_addr, dma_tr_cfg.dmac_dest_addr);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_transfer_cfg->dmac_src_addr, dma_tr_cfg.dmac_src_addr);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_transfer_cfg->transfer_size, dma_tr_cfg.transfer_size);

    // Act
    dma_channel_0_queue_dispatch(&TestRequest.header, &device);

    // Assert
    assert_ptr_equal(device.Channel[CH_0].current_request, &TestRequest);
    assert_int_equal(TestRequest.status, FPFW_DMA_STATUS_IN_PROGRESS);
}

TEST_FUNCTION(test_dma_isr_ch0, setup, teardown)
{
    // Arrange
    device.config->config_type = DMA_CONFIG_TYPE_POLLING;
    device.config->cpu_type = CPU_SCP;
    device.config->base_address = DMAC_SCP_BASE_ADDR;

    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = (uint64_t)0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600; // not used in this test
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;
    TestRequest.dma_private.translated_mscp_atu_address = (uint64_t)0xC0FFEE;

    device.Channel[CH_0].current_request = &TestRequest;
    device.Channel[CH_0].current_request->status = FPFW_DMA_STATUS_IN_PROGRESS;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &TestRequest.header);

    // Set up device_interrupt_pair with and interrupt ID = DMAC_CH0_INT
    device_identifier_pair_t device_interrupt_pair;
    device_interrupt_pair.device = &device;
    device_interrupt_pair.interrupt_id = DMAC_CH0_INT;
    will_return(__wrap_dmac_get_ch_interrupt_status, 0x1); // DMAC_CH0_INT is pending
    dma_isr(static_cast<void*>(&device_interrupt_pair));

    // Assert
    assert_ptr_equal(device.Channel[CH_0].current_request, NULL);
}

TEST_FUNCTION(test_dma_isr_ch1, setup, teardown)
{
    // Arrange
    device.config->config_type = DMA_CONFIG_TYPE_POLLING;
    device.config->cpu_type = CPU_SCP;
    device.config->base_address = DMAC_SCP_BASE_ADDR;

    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = (uint64_t)0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600; // not used in this test
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;
    TestRequest.dma_private.translated_mscp_atu_address = (uint64_t)0xC0FFEE;

    device.Channel[CH_1].current_request = &TestRequest;
    device.Channel[CH_1].current_request->status = FPFW_DMA_STATUS_IN_PROGRESS;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &TestRequest.header);

    // Set up device_interrupt_pair with and interrupt ID = DMAC_CH1_INT
    device_identifier_pair_t device_interrupt_pair;
    device_interrupt_pair.device = &device;
    device_interrupt_pair.interrupt_id = DMAC_CH1_INT;
    will_return(__wrap_dmac_get_ch_interrupt_status, 0x1); // DMAC_CH1_INT is pending
    dma_isr(static_cast<void*>(&device_interrupt_pair));

    // Assert
    assert_ptr_equal(device.Channel[CH_1].current_request, NULL);
}

TEST_FUNCTION(test_dma_isr_reg, setup, teardown)
{
    // Arrange
    device.config->config_type = DMA_CONFIG_TYPE_POLLING;
    device.config->cpu_type = CPU_SCP;
    device.config->base_address = DMAC_SCP_BASE_ADDR;

    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = (uint64_t)0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600; // not used in this test
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;
    TestRequest.dma_private.translated_mscp_atu_address = (uint64_t)0xC0FFEE;

    device.Channel[CH_1].current_request = &TestRequest;
    device.Channel[CH_1].current_request->status = FPFW_DMA_STATUS_IN_PROGRESS;

    // Set up device_interrupt_pair with and interrupt ID = DMAC_REG_INT
    device_identifier_pair_t device_interrupt_pair;
    device_interrupt_pair.device = &device;
    device_interrupt_pair.interrupt_id = DMAC_REG_INT;
    dma_isr(static_cast<void*>(&device_interrupt_pair));

    // Assert
    assert_ptr_equal(device.Channel[CH_0].current_request, NULL);
    assert_ptr_equal(device.Channel[CH_1].current_request, &TestRequest);
}

TEST_FUNCTION(test_dma_update_channel_struct_subtract_remaining_bytes_pass, setup, teardown)
{
    // Arrange
    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = (uint64_t)0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600; // not used in this test
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;
    TestRequest.dma_private.translated_mscp_atu_address = (uint64_t)0xC0FFEE;
    TestRequest.dma_private.assigned_channel = CH_1;

    device.Channel[CH_0].lock = (FPFW_LOCK)0;
    device.Channel[CH_1].lock = (FPFW_LOCK)FPFW_LOCK_SIGNATURE;
    device.Channel[CH_1].remaining_bytes_all_requests = 50;

    device.Channel[CH_1].current_request = &TestRequest;
    device.Channel[CH_1].current_request->status = FPFW_DMA_STATUS_IN_PROGRESS;

    g_FpFwLockAcquire_check_signature = true;
    expect_value(__wrap_FpFwLockAcquire, Lock->Signature, FPFW_LOCK_SIGNATURE);

    // Act
    update_channel_struct_subtract_remaining_bytes(&device, &TestRequest);

    // Assert
    assert_int_equal(device.Channel[CH_1].remaining_bytes_all_requests, 40); // 50 - 10 = 40
}

TEST_FUNCTION(test_dma_update_channel_struct_subtract_remaining_bytes_fail, setup, teardown)
{
    // Arrange
    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = (uint64_t)0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600; // not used in this test
    TestRequest.input.byte_count = 10;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;
    TestRequest.dma_private.translated_mscp_atu_address = (uint64_t)0xC0FFEE;
    TestRequest.dma_private.assigned_channel = CH_1;

    device.Channel[CH_0].lock = (FPFW_LOCK)0;
    device.Channel[CH_1].lock = (FPFW_LOCK)FPFW_LOCK_SIGNATURE;
    device.Channel[CH_1].remaining_bytes_all_requests = 10;

    device.Channel[CH_1].current_request = &TestRequest;
    device.Channel[CH_1].current_request->status = FPFW_DMA_STATUS_IN_PROGRESS;

    // Act
    update_channel_struct_subtract_remaining_bytes(&device, &TestRequest);

    // Assert
    assert_int_equal(device.Channel[CH_1].remaining_bytes_all_requests, 0); // 10 - 10 = 0

    // Act again - first_time is now false
    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ERROR_DMA_REMAINING_BYTES_INVALID);
    if (!set_error_handler_return())
    {
        update_channel_struct_subtract_remaining_bytes(&device, &TestRequest);
    }
}

TEST_FUNCTION(test_dma_channel_interrupts, setup, teardown)
{
    uint8_t chan = 0;
    uint8_t req_byte_sz = 10;

    // Arrange
    device.config->config_type = DMA_CONFIG_TYPE_INTERRUPT;
    device.config->cpu_type = CPU_SCP;
    device.config->base_address = DMAC_SCP_BASE_ADDR;

    dma_async_request_t TestRequest = {};
    TestRequest.input.src_addr_32b_aligned = (uint64_t)0x87654300;
    TestRequest.input.dest_addr_32b_aligned = 0x12345600; // not used in this test
    TestRequest.input.byte_count = req_byte_sz;
    TestRequest.header.OwningQueue = NULL;
    TestRequest.header.RequestType = DMA_REQUEST_SINGLE_ASYNC;
    TestRequest.status = FPFW_DMA_STATUS_NOT_STARTED;
    TestRequest.dma_private.translated_mscp_atu_address = (uint64_t)0xC0FFEE;

    for (uint64_t i = (1ULL << 63); i > 0U; i >>= 1)
    {
        if ((i & DMAC_ENABLED_CH_INTS_MASK) != 0)
        {
            chan = (chan + 1) % DMAC_MAX_CHANNELS;

            TestRequest.dma_private.assigned_channel = chan;
            device.Channel[chan].current_request = &TestRequest;
            device.Channel[chan].current_request->status = FPFW_DMA_STATUS_IN_PROGRESS;
            device.Channel[chan].remaining_bytes_all_requests = req_byte_sz;
            expect_value(__wrap_DfwkAsyncRequestComplete, Request, &TestRequest.header);

            // Set up device_interrupt_pair with and interrupt ID = DMAC_CH0_INT
            device_identifier_pair_t device_interrupt_pair;
            device_interrupt_pair.device = &device;
            device_interrupt_pair.interrupt_id = DMAC_CH0_INT + chan;
            will_return(__wrap_dmac_get_ch_interrupt_status, i); // DMAC_CH0_INT is pending

            dma_isr(static_cast<void*>(&device_interrupt_pair));

            // Assert
            assert_ptr_equal(device.Channel[chan].current_request, NULL);
        }
    }
}

} // extern "C"