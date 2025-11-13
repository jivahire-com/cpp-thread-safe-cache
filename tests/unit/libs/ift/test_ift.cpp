//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ift.cpp
 * Implements test cases for IFT APIs
 */

/*---------------- Includes -------------------*/
#include <CMockaWrapper.h>

extern "C" {

#include <DfwkClient.h> // for DfwkClientInterfaceOpen
#include <DfwkCommon.h> // for DfwkAsyncRequestSetCompletionRoutine
#include <DfwkDriver.h> // for DfwkQueueInitialize, DfwkInterfaceInitialize
#include <DfwkHost.h>   // for DfwkDeviceInitialize
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <hsp_firmware_headers.h>
#include <icc_platform_defines.h> // for
#include <idsw_kng.h>
#include <ift_fw.h>
#include <ift_i.h>
#include <kingsgate_fuse_defines.h> // for CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_31_0_BIT_OFFSET
#include <mpu.h>                    // for MPU_STATUS_SUCCESS
#include <nvic.h>                   // for NVIC_STATUS_SUCCESS
#include <setjmp.h>
#include <startup_shutdown.h>
#include <startup_shutdown_init.h>
#include <startup_shutdown_ssi.h>
#include <stdbool.h>
#include <stdint.h>

/*---- Symbolic Constant Macros (defines) -----*/
#define BUG_CHECK_RETURN_VALUE 0x1
#define BUGCHECK_MOCK_RETURN   (setjmp(ift_mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

#define IFT_TEST_STATUS_SUCCESS 0x00 /**< IFT test success status */
#define IFT_TEST_STATUS_FAILURE 0x01 /**< IFT test failure status */

/*--------------- Typedefs --------------------*/

/*----------- Function Prototypes -------------*/

void reset_core_test_fw_idx(void);

/*---- Declarations (Statics and globals) -----*/

jmp_buf ift_mock_jump_buf;
icc_base_recv_complete_notify g_icc_recv_cb = NULL;
void* g_icc_recv_ctx = NULL;
void* g_icc_recv_msg_buf = NULL;
uint32_t g_ift_intent_type = IFT_MGR_IFT_INTENT_TYPE_MAX;

DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE g_ift_async_request_completion_routine = NULL;
void* g_ift_async_request_completion_ctx = NULL;

DFWK_ASYNC_REQUEST_DISPATCH g_ift_async_request_dispatch = NULL;
void* g_ift_async_request_dispatch_ctx = NULL;
PDFWK_ASYNC_REQUEST_HEADER g_ift_async_request = NULL;

DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE g_ift_sos_completion_cb = NULL;
void* g_ift_sos_completion_ctx = NULL;
PDFWK_ASYNC_REQUEST_HEADER g_ift_sos_request = NULL;

icc_base_recv_complete_notify g_icc_send_recv_async_cb = NULL;
void* g_icc_send_recv_async_ctx = NULL;

/*--------------- Functions ------------------*/

static void set_mem_test_ctx()
{
    g_ift_intent_type = IFT_MGR_IFT_INTENT_MEM_TEST;
}

static void set_core_test_ctx()
{
    g_ift_intent_type = IFT_MGR_IFT_INTENT_CORE_TEST;
}

static void reset_test_ctx()
{
    g_ift_intent_type = IFT_MGR_IFT_INTENT_TYPE_MAX;
}

static int setup_mem_test(void** state)
{
    FPFW_UNUSED(state);

    static ift_device_t ift_device;
    static ift_interface_t ift_interface;

    set_mem_test_ctx();

    will_return(__wrap_mpu_init, MPU_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_IFT_INTENT_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, sizeof(struct kng_hsp_mailbox_msg_header));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    ift_init((fpfw_icc_base_ctx_t*)0xDEADBEEF);

    ift_dfwk_device_initialize(&ift_device, (PDFWK_SCHEDULE)0xDEAD1EAF);
    ift_dfwk_interface_initialize(&ift_interface, &ift_device);
    ift_dfwk_set_interface(&ift_interface);
    ift_set_current_fw_size(512 * SL_1KB);

    return 0;
}

static int setup_core_test(void** state)
{
    FPFW_UNUSED(state);

    static ift_device_t ift_device;
    static ift_interface_t ift_interface;

    set_core_test_ctx();

    will_return(__wrap_mpu_init, MPU_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_IFT_INTENT_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, sizeof(struct kng_hsp_mailbox_msg_header));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    ift_init((fpfw_icc_base_ctx_t*)0xDEADBEEF);

    ift_dfwk_device_initialize(&ift_device, (PDFWK_SCHEDULE)0xDEAD1EAF);
    ift_dfwk_interface_initialize(&ift_interface, &ift_device);
    ift_dfwk_set_interface(&ift_interface);
    ift_set_current_fw_size(512 * SL_1KB);

    return 0;
}

static int cleanup(void** state)
{
    FPFW_UNUSED(state);
    reset_test_ctx();
    reset_core_test_fw_idx();

    return 0;
}

/*------------- Mock Functions ----------------*/

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    assert_non_null(addr);

    check_expected_ptr(addr);
    check_expected(data);
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    assert_non_null(addr);

    return mock_type(uint32_t);
}

void __wrap_sleep_us(uint64_t useconds)
{
    FPFW_UNUSED(useconds);
}

_Noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Handle noreturn, allowing control to return to test
    longjmp(ift_mock_jump_buf, BUG_CHECK_RETURN_VALUE);
}

fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    assert_non_null(icc_ctx);
    assert_non_null(payload_buffer);
    assert_true(buffer_size > 0);

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    assert_non_null(icc_ctx);
    assert_non_null(params);
    assert_true(params->buffer_size > 0);

    g_icc_recv_cb = params->cb;
    g_icc_recv_ctx = params->cb_ctx;
    g_icc_recv_msg_buf = params->payload_buffer;

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    assert_non_null(icc_ctx);
    assert_non_null(payload_buffer);
    assert_true(buffer_size > 0);
    assert_non_null(output_recv_bytes);

    kng_hsp_mailbox_msg* output_message = (kng_hsp_mailbox_msg*)payload_buffer;
    g_icc_recv_msg_buf = payload_buffer;

    if (output_message->header.cmd == HSP_MAILBOX_CMD_IFT_INTENT_REQ)
    {
        struct kng_hsp_mailbox_cmd_ift_intent_rsp* rsp = (struct kng_hsp_mailbox_cmd_ift_intent_rsp*)payload_buffer;
        rsp->ift_enabled = IFT_STATUS_ENABLED;
        rsp->ift_intent_type = g_ift_intent_type;
    }

    output_message->header.cmd = mock_type(uint32_t);
    *output_recv_bytes = mock_type(size_t);

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_recv_req_t* params)
{
    assert_non_null(icc_ctx);
    assert_non_null(params);

    g_icc_recv_msg_buf = params->payload_buffer;
    g_icc_send_recv_async_cb = params->cb;
    g_icc_send_recv_async_ctx = params->cb_ctx;

    return mock_type(fpfw_status_t);
}

void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    assert_non_null(Request);
    assert_true(RequestSize > 0);
}

void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    assert_non_null(Request);
    assert_non_null(CompletionRoutine);

    g_ift_async_request_completion_routine = CompletionRoutine;
    g_ift_async_request_completion_ctx = CompletionContext;
}

void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Interface);
    assert_non_null(Request);

    g_ift_async_request = Request;
}

void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    assert_non_null(Device);
    assert_non_null(Schedule);
}

void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    assert_non_null(Queue);
    assert_non_null(Device);
    assert_non_null(DispatchRoutine);
    assert_non_null(DispatchContext);
    assert_true(QueueType == DfwkQueueType_SerializedDispatch);

    g_ift_async_request_dispatch = DispatchRoutine;
    g_ift_async_request_dispatch_ctx = DispatchContext;
}

void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{
    FPFW_UNUSED(DispatchSync);

    assert_non_null(Interface);
    assert_non_null(Device);
    assert_non_null(DispatchQueue);
}

int32_t __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER Interface)
{
    assert_non_null(Interface);

    return 0;
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Request);
}

void __wrap_sos_start_phase(PDFWK_INTERFACE_HEADER p_interface,
                            pstartup_start_phase_request_t p_request,
                            ssi_startup_type_t boot_type,
                            ssi_startup_stage_t startup_stage,
                            DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                            void* p_completion_context)
{
    assert_non_null(p_interface);
    assert_non_null(p_request);
    assert_true(boot_type == IFT_BOOT);
    assert_true(startup_stage == STARTUP_PHASE_IFT_MEM_FW_LOAD || startup_stage == STARTUP_PHASE_IFT_CORE_FW_LOAD);
    assert_non_null(completion_routine);
    assert_null(p_completion_context);

    g_ift_sos_completion_cb = completion_routine;
    g_ift_sos_completion_ctx = p_completion_context;
    g_ift_sos_request = (PDFWK_ASYNC_REQUEST_HEADER)p_request;
}

int __wrap_spi_controller_bulk_write(uintptr_t spi_master_reg, uintptr_t ahbAddr32, const uint32_t* data, size_t numDataWords)
{
    assert_true(spi_master_reg == SPI_CTRL_MASTER_REG);
    assert_true((uint32_t)ahbAddr32 == SCP_EXP_IFT_BIN_DATA_BASE);
    assert_true((uint32_t)data == SCP_EXP_IFT_BIN_DATA_BASE);
    assert_true(numDataWords > 0);

    return mock_type(int);
}

int __wrap_spi_controller_write_direct_instruction(uintptr_t spi_master_reg, uintptr_t ahbAddr32, uint32_t actualWaitCycles, uint32_t ahbData32)
{
    assert_true(spi_master_reg == SPI_CTRL_MASTER_REG);
    assert_true((uint32_t)ahbAddr32 == SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR);
    assert_true(actualWaitCycles == 9);
    assert_true(ahbData32 == SCP_EXP_IFT_SYNC_MAGIC_NUM);

    return mock_type(int);
}

uint64_t __wrap_fuse_read_data(const uintptr_t base_addr, const unsigned fuse_bit_offset, const unsigned fuse_bit_size)
{
    FPFW_UNUSED(base_addr);
    FPFW_UNUSED(fuse_bit_offset);
    FPFW_UNUSED(fuse_bit_size);

    return mock_type(uint64_t);
}

silibs_status_t __wrap_ift_execute_ist(uintptr_t ist_resolved_addr,
                                       uint32_t* data,
                                       uint32_t pattern_version,
                                       uint32_t fail_limit,
                                       uint32_t* fail_buffer,
                                       uint32_t* fail_index)
{
    assert_true(ist_resolved_addr == SCP_TOP_DBGR_IFT_ADDRESS);
    assert_non_null(data);
    assert_true(pattern_version == IFT_PATTERN_VERSION);
    assert_true(fail_limit > 0);
    assert_non_null(fail_buffer);
    assert_non_null(fail_index);

    *fail_index = mock_type(uint32_t);

    return mock_type(silibs_status_t);
}

mpu_status_t __wrap_mpu_init(const ARM_MPU_Region_t* regions, const size_t count)
{
    assert_non_null(regions);
    assert_true(count > 0);

    return mock_type(mpu_status_t);
}

void __wrap_SCB_InvalidateICache()
{
    // Mock implementation
}
void __wrap_SCB_EnableICache()
{
    // Mock implementation
}
void __wrap_SCB_InvalidateDCache()
{
    // Mock implementation
}
void __wrap_SCB_EnableDCache()
{
    // Mock implementation
}

nvic_status_t __wrap_nvic_irq_disable(uint32_t irq_num)
{
    FPFW_UNUSED(irq_num);
    return NVIC_STATUS_SUCCESS;
}

/*------------- Test Cases ----------------*/

TEST_FUNCTION(test_ift_init, nullptr, nullptr)
{
    assert_false(ift_is_enabled());

    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_IFT_INTENT_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, sizeof(struct kng_hsp_mailbox_msg_header));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_mpu_init, MPU_STATUS_SUCCESS);

    ift_init((fpfw_icc_base_ctx_t*)0xDEADBEEF);

    assert_true(ift_is_enabled());
}

TEST_FUNCTION(test_ift_init_fail, nullptr, nullptr)
{
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        //! Null param
        ift_init((fpfw_icc_base_ctx_t*)nullptr);
    }

    if (!bugcheck_mock_return())
    {
        //! fpfw_icc_base_send_sync fails
        will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_IFT_INTENT_RSP);
        will_return(__wrap_fpfw_icc_base_send_recv_sync, sizeof(struct kng_hsp_mailbox_msg_header));
        will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_INVALID_ARG_ERR);
        ift_init((fpfw_icc_base_ctx_t*)0xDEADDEED);
    }

    if (!bugcheck_mock_return())
    {
        //! Invalid response
        will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_IFT_INTENT_REQ);
        will_return(__wrap_fpfw_icc_base_send_recv_sync, sizeof(struct kng_hsp_mailbox_msg_header));
        will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
        ift_init((fpfw_icc_base_ctx_t*)0xDEADDEED);
    }

    if (!bugcheck_mock_return())
    {
        //! Invalid received message size
        will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_IFT_INTENT_RSP);
        will_return(__wrap_fpfw_icc_base_send_recv_sync, 0);
        will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
        ift_init((fpfw_icc_base_ctx_t*)0xDEADDEED);
    }
}

TEST_FUNCTION(test_ift_mem_test_die0, setup_mem_test, cleanup)
{
    idsw_die_id_t die_id = DIE_0;
    uint32_t result_index = 2;

    /* Schedule the mem IFT tests */
    ift_start_tests();

    /**
     * First (n - 1) iterations of core will be the same i.e. to
     * recursively call core test. For the last iteration we stop
     * the recursion and send the IFT core test status to HSP.
     */
    for (uint32_t i = 0; i < NUM_IFT_MEM_TEST_FW_COUNT - 1; i++)
    {
        /* Set expectations for ift_dfwk_dispatch()->ift_load_fw_sos() */
        will_return(__wrap_idsw_get_die_id, die_id);
        will_return(__wrap_fpfw_init_get_handle, (void*)0xDEADCAFE);
        g_ift_async_request_dispatch(g_ift_async_request, g_ift_async_request_dispatch_ctx);
        /* Set expectations for ift_load_fw_sos() */
        /* Set expectations for ift_d0_prepare_scp1() */
        expect_value(__wrap_mmio_write32, addr, SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR);
        expect_value(__wrap_mmio_write32, data, 0);
        will_return(__wrap_spi_controller_bulk_write, SILIBS_SUCCESS);
        will_return(__wrap_fpfw_init_get_handle, (void*)0xDEADCAFE);
        will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
        /* Set expectations for ift_fw_load_done_sos_cb() */
        g_ift_sos_completion_cb(g_ift_sos_request, g_ift_sos_completion_ctx);
        /* Set expectations for ift_execute_test() */
        will_return(__wrap_idsw_get_die_id, die_id);
        will_return(__wrap_ift_execute_ist, result_index);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        /** Set expectations for ift_wait_for_scp_die1() **/
        will_return(__wrap_mmio_read32, 0);
        will_return(__wrap_mmio_read32, SCP_EXP_IFT_SYNC_MAGIC_NUM);
        /* Set expectations for ift_execute_test_dfwk() */
        g_ift_async_request_dispatch(g_ift_async_request, g_ift_async_request_dispatch_ctx);
    }

    /* Set expectations for ift_dfwk_dispatch()->ift_load_fw_sos() */
    will_return(__wrap_idsw_get_die_id, die_id);
    will_return(__wrap_fpfw_init_get_handle, (void*)0xDEADCAFE);
    g_ift_async_request_dispatch(g_ift_async_request, g_ift_async_request_dispatch_ctx);
    /* Set expectations for ift_load_fw_sos() */
    /* Set expectations for ift_d0_prepare_scp1() */
    expect_value(__wrap_mmio_write32, addr, SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR);
    expect_value(__wrap_mmio_write32, data, 0);
    will_return(__wrap_spi_controller_bulk_write, SILIBS_SUCCESS);
    will_return(__wrap_fpfw_init_get_handle, (void*)0xDEADCAFE);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    /* Set expectations for ift_fw_load_done_sos_cb() */
    g_ift_sos_completion_cb(g_ift_sos_request, g_ift_sos_completion_ctx);
    /* Set expectations for ift_execute_test() */
    will_return(__wrap_idsw_get_die_id, die_id);
    will_return(__wrap_ift_execute_ist, result_index);
    will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
    /** Set expectations for ift_wait_for_scp_die1() **/
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_mmio_read32, SCP_EXP_IFT_SYNC_MAGIC_NUM);
    will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
    will_return(__wrap_mmio_read32, 0x0);
    will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    /* Set expectations for ift_execute_test_dfwk() */
    g_ift_async_request_dispatch(g_ift_async_request, g_ift_async_request_dispatch_ctx);

    /** Set expectations for ift_icc_send_test_results() **/
    for (uint32_t i = 0; i < result_index - 1; i++)
    {
        will_return(__wrap_mmio_read32, 0x0);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);
    }

    /* Set expectations for ift_icc_send_test_status() */
    will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);
    /* Set expectations for ift_icc_recv_status_cb() */
    will_return(__wrap_idsw_get_die_id, die_id);
    g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);

    /* Set expectations for ift_dfwk_test_complete_cb() */
    g_ift_async_request_completion_routine(g_ift_async_request, g_ift_async_request_completion_ctx);
}

TEST_FUNCTION(test_ift_mem_test_die1, setup_mem_test, cleanup)
{
    idsw_die_id_t die_id = DIE_1;
    uint32_t result_index = 2;

    /* Schedule the mem IFT tests */
    ift_start_tests();

    /**
     * First (n - 1) iterations of core will be the same i.e. to
     * recursively call core test. For the last iteration we stop
     * the recursion and send the IFT core test status to HSP.
     */
    for (uint32_t i = 0; i < NUM_IFT_MEM_TEST_FW_COUNT; i++)
    {
        /* Set expectations for ift_dfwk_dispatch()->ift_load_fw_sos() */
        will_return(__wrap_idsw_get_die_id, die_id);
        will_return(__wrap_fpfw_init_get_handle, (void*)0xDEADCAFE);
        will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_ift_async_request_dispatch(g_ift_async_request, g_ift_async_request_dispatch_ctx);
        /* Set expectations for ift_recv_d2d_fw_done_cb() */
        g_icc_recv_cb(g_icc_recv_ctx, sizeof(rmss_d2d_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);
        /* Set expectations for ift_execute_test() */
        will_return(__wrap_idsw_get_die_id, die_id);
        will_return(__wrap_ift_execute_ist, result_index);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        if (i < NUM_IFT_MEM_TEST_FW_COUNT - 1)
        {
            will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
        }
        else
        {
            will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
            will_return(__wrap_mmio_read32, 0x0);
            will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        }
        /* Set expectations for ift_execute_test_dfwk() */
        g_ift_async_request_dispatch(g_ift_async_request, g_ift_async_request_dispatch_ctx);
    }

    /** Set expectations for ift_icc_send_test_results() **/
    for (uint32_t i = 0; i < result_index - 1; i++)
    {
        will_return(__wrap_mmio_read32, 0x0);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);
    }

    /* Set expectations for ift_icc_send_test_status() */
    will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);
    /* Set expectations for ift_icc_recv_status_cb() */
    will_return(__wrap_idsw_get_die_id, die_id);
    will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
    g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);

    /* Set expectations for ift_dfwk_test_complete_cb() */
    g_ift_async_request_completion_routine(g_ift_async_request, g_ift_async_request_completion_ctx);
}

TEST_FUNCTION(test_ift_core_test_die1, setup_core_test, cleanup)
{
    idsw_die_id_t die_id = DIE_1;
    uint32_t result_index = 2;

    /* Schedule the mem IFT tests */
    ift_start_tests();

    /**
     * First (n - 1) iterations of core will be the same i.e. to
     * recursively call core test. For the last iteration we stop
     * the recursion and send the IFT core test status to HSP.
     */
    for (uint32_t i = 0; i < NUM_IFT_CORE_TEST_FW_COUNT; i++)
    {
        /* Set expectations for ift_dfwk_dispatch()->ift_load_fw_sos() */
        will_return(__wrap_idsw_get_die_id, die_id);
        will_return(__wrap_fpfw_init_get_handle, (void*)0xDEADCAFE);
        will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_ift_async_request_dispatch(g_ift_async_request, g_ift_async_request_dispatch_ctx);
        /* Set expectations for ift_recv_d2d_fw_done_cb() */
        g_icc_recv_cb(g_icc_recv_ctx, sizeof(rmss_d2d_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);
        /* Set expectations for ift_execute_test() */
        will_return(__wrap_idsw_get_die_id, die_id);
        will_return(__wrap_ift_execute_ist, result_index);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        if (i < NUM_IFT_CORE_TEST_FW_COUNT - 1)
        {
            will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
        }
        else
        {
            will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
            will_return(__wrap_mmio_read32, 0x0);
            will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        }
        /* Set expectations for ift_execute_test_dfwk() */
        g_ift_async_request_dispatch(g_ift_async_request, g_ift_async_request_dispatch_ctx);
    }

    /** Set expectations for ift_icc_send_test_results() **/
    for (uint32_t i = 0; i < result_index - 1; i++)
    {
        will_return(__wrap_mmio_read32, 0x0);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);
    }

    /* Set expectations for ift_icc_send_test_status() */
    will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);
    /* Set expectations for ift_icc_recv_status_cb() */
    will_return(__wrap_idsw_get_die_id, die_id);
    will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
    g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_SUCCESS);

    /* Set expectations for ift_dfwk_test_complete_cb() */
    g_ift_async_request_completion_routine(g_ift_async_request, g_ift_async_request_completion_ctx);
}

TEST_FUNCTION(test_ift_mem_test_fail, setup_mem_test, cleanup)
{
    uint32_t result_index = 1;
    DFWK_ASYNC_REQUEST_HEADER ift_async_request;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    // spi_controller_bulk_write() failed
    if (!bugcheck_mock_return())
    {
        expect_value(__wrap_mmio_write32, addr, SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR);
        expect_value(__wrap_mmio_write32, data, 0);
        will_return(__wrap_spi_controller_bulk_write, SILIBS_E_PARAM);
        g_ift_sos_completion_cb(g_ift_sos_request, g_ift_sos_completion_ctx);
    }

    // fpfw_icc_base_send_sync() failed to send fw transfer done to SCP1
    if (!bugcheck_mock_return())
    {
        expect_value(__wrap_mmio_write32, addr, SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR);
        expect_value(__wrap_mmio_write32, data, 0);
        will_return(__wrap_spi_controller_bulk_write, SILIBS_SUCCESS);
        will_return(__wrap_fpfw_init_get_handle, (void*)0xDEADCAFE);
        will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_INVALID_SIZE_ARG_ERR);
        g_ift_sos_completion_cb(g_ift_sos_request, g_ift_sos_completion_ctx);
    }

    // Invalid Request type to DFWK
    {
        ift_async_request.RequestType = IFT_DFWK_REQUEST_TYPE_MAX;

        g_ift_async_request_dispatch(&ift_async_request, NULL);
    }

    // spi_controller_write_direct_instruction() failed
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, result_index);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_E_PARAM);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);
    }

    // fpfw_icc_base_recv() failed to register fw transfer done callback
    if (!bugcheck_mock_return())
    {
        ift_async_request.RequestType = IFT_CORE_TESTS_FW_LOAD_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_fpfw_init_get_handle, (void*)0xDEADCAFE);
        will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_INVALID_ARG_ERR);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);
    }

    // Invalid response received from HSP for IFT IST run status
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 1);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
        will_return(__wrap_mmio_read32, 0x0);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_DISPATCHER_INIT_ERR);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);
    }

    // Invalid response received from HSP for IFT run status
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_INVALID_ARG_ERR);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);
    }

    // No data received from HSP for IFT result status
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 1);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
        will_return(__wrap_mmio_read32, 0x0);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, 0, FPFW_ICC_BASE_STATUS_SUCCESS);
    }

    // Receive CD failed for HSP CB IFT result status
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 2);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
        will_return(__wrap_mmio_read32, 0x0);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_INVALID_ARG_ERR);
    }

    // No data received from HSP for IFT test status
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, 0, FPFW_ICC_BASE_STATUS_SUCCESS);
    }

    // Receive CD failed for HSP CB IFT test status
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        g_icc_send_recv_async_cb(g_icc_send_recv_async_ctx, sizeof(struct kng_hsp_mailbox_msg_header), FPFW_ICC_BASE_STATUS_INVALID_ARG_ERR);
    }

    // Increment the FW index variable (s_ift_fw_idx) beyond max limit
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        will_return(__wrap_idsw_get_die_id, DIE_1);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);
    }

    // FW transfer done receive failed to register callback
    if (!bugcheck_mock_return())
    {
        g_icc_recv_cb(g_icc_recv_ctx, sizeof(rmss_d2d_mailbox_msg_header), FPFW_ICC_TRANSPORT_STATUS_MBX_FLUSH_IS_REQUESTED);
    }

    // FW transfer done receive failed to register callback
    if (!bugcheck_mock_return())
    {
        g_icc_recv_cb(g_icc_recv_ctx, 0, FPFW_ICC_BASE_STATUS_SUCCESS);
    }

    // Invalid FW download size
    if (!bugcheck_mock_return())
    {
        ift_set_current_fw_size(512 * SL_1KB + 1);
    }

    // NULL interface
    if (!bugcheck_mock_return())
    {
        ift_dfwk_set_interface(NULL);
        ift_start_tests();
    }

    // Undo previous tests settings
    static ift_interface_t ift_interface;
    ift_dfwk_set_interface(&ift_interface);
}

TEST_FUNCTION(test_ift_core_test_fail, setup_core_test, cleanup)
{
    DFWK_ASYNC_REQUEST_HEADER ift_async_request;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    // Increment the FW index variable (s_ift_fw_idx) beyond max limit
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        for (uint32_t i = 0; i < NUM_IFT_CORE_TEST_FW_COUNT - 1; i++)
        {
            will_return(__wrap_idsw_get_die_id, DIE_1);
            will_return(__wrap_ift_execute_ist, 0);
            will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
            will_return(__wrap_spi_controller_write_direct_instruction, SILIBS_SUCCESS);
            g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);
        }

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, 0);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        will_return_count(__wrap_fuse_read_data, 0x0, CORE_DEFECT_MFG_MASK_ARRAY_SIZE);
        will_return(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);

        will_return(__wrap_idsw_get_die_id, DIE_1);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);
    }

    // Too many results generated
    if (!bugcheck_mock_return())
    {
        reset_core_test_fw_idx();
        ift_async_request.RequestType = IFT_EXECUTE_FW_ASYNC;

        will_return(__wrap_idsw_get_die_id, DIE_1);
        will_return(__wrap_ift_execute_ist, (SCP_EXP_IFT_RESULT_MAX << 1) + 1);
        will_return(__wrap_ift_execute_ist, SILIBS_SUCCESS);
        g_ift_async_request_dispatch(&ift_async_request, g_ift_async_request_dispatch_ctx);
    }
}

TEST_FUNCTION(test_ift_sleep_ns, NULL, NULL)
{
    sleep_ns(1);
    sleep_ns(1000);
}

} // extern "C"
