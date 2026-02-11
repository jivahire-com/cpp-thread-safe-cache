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
#include <stdio.h>
#include <string.h> // IWYU pragma: keep

extern "C" {
#include "ddr_mocks.h"
#include "memory_map/ddrss_reserved_regions.h"

#include <atu_lib.h>     // for atu_map
#include <bdat_schema.h> // for bdat structure
#include <boot_status.h>
#include <cli_ddr.h>
#include <crash_dump.h> // for crash_dump_init
#include <ddr_i3c.h>
#include <ddr_manager.h> // for ddr_manager_init, ddr_service_context_t
#include <ddr_manager_bwl.h>
#include <ddr_manager_dfwk.h>
#include <ddr_manager_i.h> // for ddr_poll_dimms, ddr_worker_thread_func
#include <ddr_ppr.h>
#include <ddr_rhtlm_service.h>
#include <ddrss_lib.h>
#include <ddrss_sdl.h>
#include <error_handler.h> // for set_error_handler_return
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <kingsgate_hsp_mailbox_commands.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <sensor_fifo_service.h>
#include <smbios_structs.h>
#include <startup_shutdown.h>
#include <thread_x_mocks.h>
#include <tx_api.h> // for TX_SUCCESS, ULONG, TX_NOT_DONE, TX_NO_MEMORY

bool g_check_boot_status_params = false;

void __wrap_boot_status_notify_extd(boot_status_req_t* req, uint32_t status_code, uint32_t ext_code)
{
    FPFW_UNUSED(req);
    FPFW_UNUSED(ext_code);
    if (g_check_boot_status_params)
    {
        check_expected(status_code);
        function_called();
    }
}

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
#define UNUSED(x)                       (void)(x)
#define BDAT_RESERVATION_SIZE           (BDAT_DATA_END - BDAT_DATA_START)
#define SMBIOS_HANDOFF_RESERVATION_SIZE (SMBIOS_HANDOFF_RESERVATION_END - SMBIOS_HANDOFF_RESERVATION_BASE)

// #define DEBUG_PRINT_EXPECT_CALLS_ENABLED (true)
#ifdef DEBUG_PRINT_EXPECT_CALLS_ENABLED
    #define PRINT_EXPECT_CALL(fn, param, ret) printf("Expecting %s(%s) -> %d\n", #fn, #param, ret)
#else
    #define PRINT_EXPECT_CALL(fn, param, ret) (void)0
#endif

#define DDRSS_PHY_TRAINING_MARGIN_SIZE_BYTES (7680)
#define NUM_TEMP_SENSORS_PER_DIMM            (2)
#define DDR_TEST_STACK_SIZE                  (1024)
#define DDR_TEST_THREAD_PRIORITY             (10)
#define DDR_TEST_TIMER_INITIAL_TICKS         (10)
#define DDR_TEST_TIMER_RESCHEDULE_TICKS      (15)
#define TEST_DDRSS_DDR5_RDIMM_VPP_MV         (1800)
#define TEST_DDRSS_DDR5_RDIMM_VDD_MV         (1100)
#define TEST_JEDEC_MICROSOFT_ID              (0xD5)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* icc_ctx;
mscp_exp_spi_sync_point_t d2d_sync_point_test = {};
uint8_t test_dq_training_margin[DDRSS_PHY_TRAINING_MARGIN_SIZE_BYTES] = {0};

uint8_t ddr_test_stack[DDR_TEST_STACK_SIZE];
static uint32_t ddr_queue_pool[10];
MEMORY_DEFECT_LIST_HEADER empty_header = {0};
ddr_service_config_t config = {.thread_config =
                                   {
                                       .p_stack = ddr_test_stack,
                                       .stack_size = sizeof(ddr_test_stack),
                                       .priority = DDR_TEST_THREAD_PRIORITY,
                                       .time_slice_option = TX_NO_TIME_SLICE,
                                   },
                               .queue_config = {
                                   .p_queue = ddr_queue_pool,
                                   .msg_size = sizeof(ddr_queue_pool[0]) / sizeof(uint32_t),
                                   .queue_num_words = sizeof(ddr_queue_pool) / sizeof(uint32_t),
                               }};

uint8_t sdl_test_buffer[SCP_EXP_SDL_LOAD_SIZE] = {0};

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
bool in_memory(uintptr_t start_addr, uintptr_t end_addr)
{
    assert_true(start_addr <= end_addr);

    return mock_type(bool);
}

UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit, UINT mutex_control_block_size)
{
    assert_non_null(mutex_ptr); // Ensure the mutex pointer is not NULL

    assert_non_null(name_ptr);
    UNUSED(inherit);
    UNUSED(mutex_control_block_size);

    function_called();

    return 0;
}

int __wrap_atu_translate_address(atu_id_t atu_id, uint64_t ap_addr, uint32_t* mscp_addr)
{
    check_expected(atu_id);
    check_expected(ap_addr);
    assert_non_null(mscp_addr);
    // return a value
    *mscp_addr = mock_type(uint32_t);
    return mock_type(int);
}

int __wrap_mscp_exp_spi_synchronize_dies(mscp_exp_spi_sync_point_t sync_point, int die_id)
{
    d2d_sync_point_test = sync_point;
    check_expected(die_id);

    return mock_type(int);
}

int __wrap_mscp_exp_spi_synchronize_dies_timeout(mscp_exp_spi_sync_point_t sync_point, int die_id, uint32_t timeout_ms)
{
    FPFW_UNUSED(timeout_ms);

    d2d_sync_point_test = sync_point;
    check_expected(die_id);

    return mock_type(int);
}

int32_t __wrap_ddr_i3c_interface_read_spd_nvm_data(i3c_cmd_t* s_i3c_cmd,
                                                   uint8_t ddrss_index,
                                                   uint8_t* data,
                                                   uint16_t* data_len,
                                                   uint16_t* ptr_start,
                                                   uint16_t size)
{
    UNUSED(s_i3c_cmd);
    UNUSED(ddrss_index);
    UNUSED(data_len);
    UNUSED(ptr_start);
    UNUSED(size);
    UNUSED(data);

    function_called();
    return mock_type(int32_t);
}

//! Mocks for mailbox primitives called inside hsp_send_ddr_init_notify()
fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    UNUSED(icc_ctx);
    UNUSED(buffer_size);
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

bool __wrap_system_info_is_warm_start()
{
    return mock_type(bool);
}

int __wrap_ddrss_phy_get_training_margin(uint32_t mc, ddrss_phy_training_dq_margin_t* ddrss_phy_training_margin)
{
    UNUSED(ddrss_phy_training_margin);
    UNUSED(mc);

    function_called();
    return mock_type(int);
}

const char* __wrap_dimm_vendor_from_id(uint8_t id_byte)
{
    UNUSED(id_byte);
    return mock_type(const char*);
}

size_t __wrap_get_dimm_serial_number_string(const uint8_t* spd_buff, char* buffer, size_t bufferSize)
{
    UNUSED(spd_buff);
    UNUSED(bufferSize);
    return snprintf(buffer, bufferSize, "%s", mock_type(const char*));
}

size_t __wrap_get_dimm_part_number_string(const uint8_t* spd_buff, char* buffer, size_t bufferSize)
{
    UNUSED(spd_buff);
    UNUSED(bufferSize);
    return snprintf(buffer, bufferSize, "%s", mock_type(const char*));
}

uint32_t __wrap_get_i3c_dimm_detected(void)
{
    return mock_type(uint32_t);
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

void __wrap_sensor_fifo_svc_add_dimm_info(sensor_ram_dimm_info_t* dimm_info)
{
    check_expected_ptr(dimm_info);
}

int32_t __wrap_ddr_i3c_interface_read_pmic_power(i3c_instance_t* instance,
                                                 i3c_cmd_t* s_i3c_cmd,
                                                 uint8_t dev_id,
                                                 uint8_t mr_reg,
                                                 uint8_t* data8,
                                                 uint8_t* data_len)
{
    UNUSED(instance);
    UNUSED(s_i3c_cmd);
    UNUSED(dev_id);
    UNUSED(mr_reg);
    UNUSED(data_len);
    *data8 = mock_type(uint8_t);

    return SILIBS_SUCCESS;
}

bool __wrap_ift_is_enabled(void)
{
    return mock_type(bool);
}

int32_t __wrap_sos_register_ssi(PDFWK_INTERFACE_HEADER p_interface,
                                pstartup_ssi_registration_t p_registration,
                                PDFWK_INTERFACE_HEADER p_ssi_interface)
{
    check_expected_ptr(p_interface);
    check_expected_ptr(p_registration);
    check_expected_ptr(p_ssi_interface);
    return mock_type(int32_t);
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
    function_called();
}

} // extern "C"

//
// Tests
//
TEST_FUNCTION(ddr_manager_init_fail, NULL, NULL)
{
    ddr_service_context_t ddr_service_context = {};
    ddr_service_config_t ddr_service_config = {};

    will_return_always(__wrap_idsw_get_die_id, 0);

    // tx queue fails
    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_NO_MEMORY);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NO_MEMORY);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    // tx queue create passes but tx sem create fails
    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_any(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_create, name_ptr);
    expect_any(__wrap__txe_semaphore_create, initial_count);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_NO_MEMORY);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NO_MEMORY);

    will_return(__wrap_system_info_is_warm_start, false);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    // tx queue & sem create passes but tx queue send fails
    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    expect_any(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_create, name_ptr);
    expect_any(__wrap__txe_semaphore_create, initial_count);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);
    will_return(__wrap_system_info_is_warm_start, false);

    // tx queue send DDR CREATE BDAT EVENT fails
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_ERROR);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    // tx queue send DDR BDAT CREATE EVENT passes
    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    expect_any(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_create, name_ptr);
    expect_any(__wrap__txe_semaphore_create, initial_count);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);
    will_return(__wrap_system_info_is_warm_start, false);
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // tx queue send DDR BDAT EVENT fails
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_ERROR);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    expect_any(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_create, name_ptr);
    expect_any(__wrap__txe_semaphore_create, initial_count);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);
    will_return(__wrap_system_info_is_warm_start, false);
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_CREATE_BDAT_EVENT

    // tx queue send DDR SMBIOS EVENT fails
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_ERROR);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    expect_any(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_create, name_ptr);
    expect_any(__wrap__txe_semaphore_create, initial_count);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);
    will_return(__wrap_system_info_is_warm_start, false);
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_CREATE_BDAT_EVENT
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_CREATE_SMBIOS_TABLES_EVENT
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_COPY_PRM_ADDR_TRANS_CONFIG_EVENT

    // tx queue send DDR_START_POLLING_TIMER_EVENT fails
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR); // DDR_START_POLLING_TIMER_EVENT
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_ERROR);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    expect_any(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_create, name_ptr);
    expect_any(__wrap__txe_semaphore_create, initial_count);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);
    will_return(__wrap_system_info_is_warm_start, false);
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_CREATE_BDAT_EVENT
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_CREATE_SMBIOS_TABLES_EVENT
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_COPY_PRM_ADDR_TRANS_CONFIG_EVENT
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_START_POLLING_TIMER_EVENT

    // tx thread create fails
    expect_any(__wrap__txe_thread_create, thread_ptr);
    expect_any(__wrap__txe_thread_create, name_ptr);
    expect_any(__wrap__txe_thread_create, entry_function);
    expect_any(__wrap__txe_thread_create, entry_input);
    expect_any(__wrap__txe_thread_create, stack_start);
    expect_any(__wrap__txe_thread_create, stack_size);
    expect_any(__wrap__txe_thread_create, priority);
    expect_any(__wrap__txe_thread_create, preempt_threshold);
    expect_any(__wrap__txe_thread_create, time_slice);
    expect_any(__wrap__txe_thread_create, auto_start);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_NOT_DONE);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NOT_DONE);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }

    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    expect_any(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_create, name_ptr);
    expect_any(__wrap__txe_semaphore_create, initial_count);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);
    will_return(__wrap_system_info_is_warm_start, false);
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_CREATE_BDAT_EVENT
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_CREATE_SMBIOS_TABLES_EVENT
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_COPY_PRM_ADDR_TRANS_CONFIG_EVENT
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS); // DDR_START_POLLING_TIMER_EVENT
    expect_any(__wrap__txe_thread_create, thread_ptr);
    expect_any(__wrap__txe_thread_create, name_ptr);
    expect_any(__wrap__txe_thread_create, entry_function);
    expect_any(__wrap__txe_thread_create, entry_input);
    expect_any(__wrap__txe_thread_create, stack_start);
    expect_any(__wrap__txe_thread_create, stack_size);
    expect_any(__wrap__txe_thread_create, priority);
    expect_any(__wrap__txe_thread_create, preempt_threshold);
    expect_any(__wrap__txe_thread_create, time_slice);
    expect_any(__wrap__txe_thread_create, auto_start);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    // Inside ddr_manager_i3c_init()
    will_return(__wrap_idhw_get_die_id, 0);

    will_return(__wrap_gtimer_prodfw_get_counter, 1000000);     // start timestamp
    will_return(__wrap_gtimer_prodfw_get_counter, 4000000);     // ddr training timestamp
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000); // 125 MHz

    // SDL pointer variable write
    will_return(__wrap_config_get_ddrmanager_sdl_en, false);

    g_should_wrap_ddr_create_memory_map = true;
    expect_function_call(__wrap_ddr_create_memory_map);

    // hsp_send_ddr_init_notify()
    // will_return(__wrap_system_info_is_warm_start, false);
    kng_hsp_mailbox_msg expected_msg = {};
    expected_msg.header.cmd = HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY;
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, payload_buffer, &expected_msg, sizeof(expected_msg));
    expect_any(__wrap_fpfw_icc_base_send_recv_sync, output_recv_bytes);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, SILIBS_SUCCESS);

    // Telemetry init
    expect_function_calls(__wrap__txe_mutex_create, 2);

    // Crash dump pre-dump callback init
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    crash_dump_init(&context);

    // Mock gtimer for duration calculation
    will_return(__wrap_gtimer_prodfw_get_counter, 5000000);     // ddr init timestamp
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000); // 125 MHz

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config, icc_ctx);
    }
    g_should_wrap_ddr_create_memory_map = false;
}

TEST_FUNCTION(cli_ddr_dev_mode_disabled, NULL, NULL)
{
    assert_false(g_cli_ddr_dev_mode);
}

TEST_FUNCTION(ddr_manager_init_check_params, NULL, NULL)
{
    static ddr_service_context_t ddr_service_ctx = {};

    will_return_always(__wrap_config_get_ddrmanager_sdl_en, true);

    will_return_always(__wrap_variable_service_sync_get_variable, SILIBS_SUCCESS);
    will_return_always(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);
    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    expect_value(__wrap__txe_queue_create, queue_ptr, &ddr_service_ctx.work_queue);
    expect_value(__wrap__txe_queue_create, name_ptr, DDR_WORK_QUEUE_NAME);
    expect_value(__wrap__txe_queue_create, message_size, config.queue_config.msg_size);
    expect_value(__wrap__txe_queue_create, queue_start, config.queue_config.p_queue);
    expect_value(__wrap__txe_queue_create, queue_size, config.queue_config.queue_num_words * sizeof(uint32_t));
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_value(__wrap__txe_semaphore_create, semaphore_ptr, &ddr_service_ctx.quiesce_sem);
    expect_string(__wrap__txe_semaphore_create, name_ptr, DDR_QUIESCE_SEM_NAME);
    expect_value(__wrap__txe_semaphore_create, initial_count, 0);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);

    will_return(__wrap_system_info_is_warm_start, false);

    // DDR_CREATE_BDAT_EVENT
    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // DDR_CREATE_SMBIOS_TABLES_EVENT
    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // DDR_COPY_PRM_ADDR_TRANS_CONFIG_EVENT
    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // DDR_START_POLLING_TIMER_EVENT
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
    expect_value(__wrap__txe_thread_create, auto_start, TX_AUTO_START);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    // Inside ddr_manager_i3c_init()
    will_return(__wrap_idhw_get_die_id, 0);

    // SDL load
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)sdl_test_buffer);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    // Don't skip publishing SDL address variable
    will_return(__wrap_config_get_skip_sdl_address_publishing, false);

    // SDL pointer variable write
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)sdl_test_buffer);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    // Invalidate Cache for SDL region
    expect_any(SCB_CleanInvalidateDCache_by_Addr, addr);
    expect_any(SCB_CleanInvalidateDCache_by_Addr, dsize);
    expect_function_call(SCB_CleanInvalidateDCache_by_Addr);

    g_should_wrap_ddr_create_memory_map = true;
    expect_function_call(__wrap_ddr_create_memory_map);

    // hsp_send_ddr_init_notify()
    will_return(__wrap_system_info_is_warm_start, true); // Skip fpfw_icc_base_send_recv_sync for unit test

    // Telemetry init
    expect_function_calls(__wrap__txe_mutex_create, 2);

    // Crash dump pre-dump callback init
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    crash_dump_init(&context);

    // Mock gtimer for duration calculation
    will_return(__wrap_gtimer_prodfw_get_counter, 1000000);     // start timestamp
    will_return(__wrap_gtimer_prodfw_get_counter, 4000000);     // ddr training timestamp
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000); // 125 MHz
    will_return(__wrap_gtimer_prodfw_get_counter, 5000000);     // ddr init timestamp
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000); // 125 MHz

    ddr_manager_init(&ddr_service_ctx, &config, icc_ctx);
    g_should_wrap_ddr_create_memory_map = false;
}

void tx_queue_copy_parameter(void* p)
{
    // Copy the parameter to the destination
    *(uint32_t*)p = mock_type(int);
}

TEST_FUNCTION(ddr_manager_init_boot_status_cold_start, NULL, NULL)
{
    static ddr_service_context_t ddr_service_ctx = {};

    g_check_boot_status_params = true;

    will_return_always(__wrap_config_get_ddrmanager_sdl_en, true);
    will_return_always(__wrap_variable_service_sync_get_variable, SILIBS_SUCCESS);
    will_return_always(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);
    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_warm_start, false);

    // Queue create
    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    // Semaphore create
    expect_any(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_create, name_ptr);
    expect_any(__wrap__txe_semaphore_create, initial_count);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);

    // Queue sends: BDAT, SMBIOS, PRM, POLLING_TIMER
    expect_any_count(__wrap__txe_queue_send, queue_ptr, 4);
    expect_any_count(__wrap__txe_queue_send, source_ptr, 4);
    expect_any_count(__wrap__txe_queue_send, wait_option, 4);
    will_return_count(__wrap__txe_queue_send, TX_SUCCESS, 4);

    // Thread create
    expect_any(__wrap__txe_thread_create, thread_ptr);
    expect_any(__wrap__txe_thread_create, name_ptr);
    expect_any(__wrap__txe_thread_create, entry_function);
    expect_any(__wrap__txe_thread_create, entry_input);
    expect_any(__wrap__txe_thread_create, stack_start);
    expect_any(__wrap__txe_thread_create, stack_size);
    expect_any(__wrap__txe_thread_create, priority);
    expect_any(__wrap__txe_thread_create, preempt_threshold);
    expect_any(__wrap__txe_thread_create, time_slice);
    expect_any(__wrap__txe_thread_create, auto_start);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    // ddr_manager_i3c_init
    will_return(__wrap_idhw_get_die_id, 0);

    // gtimer mocks
    will_return(__wrap_gtimer_prodfw_get_counter, 1000000); // start timestamp
    will_return(__wrap_gtimer_prodfw_get_counter, 4000000); // ddr training end
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000);

    // --- Boot status expectations ---
    // 1. DDR training start
    expect_value(__wrap_boot_status_notify_extd, status_code, MSCP_BOOT_STATUS_CODE_SCP_DDR_TRAINING_START);
    expect_function_call(__wrap_boot_status_notify_extd);

    // 2. DDR training end
    expect_value(__wrap_boot_status_notify_extd, status_code, MSCP_BOOT_STATUS_CODE_SCP_DDR_TRAINING_END);
    expect_function_call(__wrap_boot_status_notify_extd);

    // SDL load
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)sdl_test_buffer);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    // Don't skip publishing SDL address variable
    will_return(__wrap_config_get_skip_sdl_address_publishing, false);

    // SDL pointer variable write
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)sdl_test_buffer);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    // Invalidate Cache for SDL region
    expect_any(SCB_CleanInvalidateDCache_by_Addr, addr);
    expect_any(SCB_CleanInvalidateDCache_by_Addr, dsize);
    expect_function_call(SCB_CleanInvalidateDCache_by_Addr);

    // 3. SDL load end
    expect_value(__wrap_boot_status_notify_extd, status_code, MSCP_BOOT_STATUS_CODE_SCP_DDR_SDL_LOAD_END);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Memory map
    g_should_wrap_ddr_create_memory_map = true;
    expect_function_call(__wrap_ddr_create_memory_map);

    // 4. Memory map end
    expect_value(__wrap_boot_status_notify_extd, status_code, MSCP_BOOT_STATUS_CODE_SCP_DDR_MEMORY_MAP_END);
    expect_function_call(__wrap_boot_status_notify_extd);

    // hsp_send_ddr_init_notify
    kng_hsp_mailbox_msg expected_msg = {};
    expected_msg.header.cmd = HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY;
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, payload_buffer, &expected_msg, sizeof(expected_msg));
    expect_any(__wrap_fpfw_icc_base_send_recv_sync, output_recv_bytes);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, SILIBS_SUCCESS);

    // 5. HSP notify end
    expect_value(__wrap_boot_status_notify_extd, status_code, MSCP_BOOT_STATUS_CODE_SCP_DDR_HSP_NOTIFY_END);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Telemetry init
    expect_function_calls(__wrap__txe_mutex_create, 2);

    // Crash dump
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    crash_dump_init(&context);

    // Duration calculation
    will_return(__wrap_gtimer_prodfw_get_counter, 5000000);
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000);

    ddr_manager_init(&ddr_service_ctx, &config, icc_ctx);
    g_should_wrap_ddr_create_memory_map = false;
    g_check_boot_status_params = false;
}

TEST_FUNCTION(ddr_manager_init_warm_start, NULL, NULL)
{
    static ddr_service_context_t ddr_service_ctx = {};

    g_check_boot_status_params = false;

    will_return_always(__wrap_config_get_ddrmanager_sdl_en, true);

    will_return_always(__wrap_variable_service_sync_get_variable, SILIBS_SUCCESS);
    will_return_always(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);
    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    expect_value(__wrap__txe_queue_create, queue_ptr, &ddr_service_ctx.work_queue);
    expect_value(__wrap__txe_queue_create, name_ptr, DDR_WORK_QUEUE_NAME);
    expect_value(__wrap__txe_queue_create, message_size, config.queue_config.msg_size);
    expect_value(__wrap__txe_queue_create, queue_start, config.queue_config.p_queue);
    expect_value(__wrap__txe_queue_create, queue_size, config.queue_config.queue_num_words * sizeof(uint32_t));
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_value(__wrap__txe_semaphore_create, semaphore_ptr, &ddr_service_ctx.quiesce_sem);
    expect_string(__wrap__txe_semaphore_create, name_ptr, DDR_QUIESCE_SEM_NAME);
    expect_value(__wrap__txe_semaphore_create, initial_count, 0);
    expect_any(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return(__wrap__txe_semaphore_create, TX_SUCCESS);

    will_return(__wrap_system_info_is_warm_start, true);

    // Do not expect DDR_CREATE_BDAT_EVENT

    // Do not expect DDR_CREATE_SMBIOS_TABLES_EVENT

    // Do not expect DDR_COPY_PRM_ADDR_TRANS_CONFIG_EVENT

    // DDR_START_POLLING_TIMER_EVENT
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
    expect_value(__wrap__txe_thread_create, auto_start, TX_AUTO_START);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    // will_return(__wrap__txe_timer_create, TX_SUCCESS);
    will_return(__wrap_idhw_get_die_id, DIE_0);

    // SDL load
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)sdl_test_buffer);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    // Dont't skip publishing SDL address variable
    will_return(__wrap_config_get_skip_sdl_address_publishing, false);

    // SDL pointer variable write
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)sdl_test_buffer);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    // Invalidate Cache for SDL region
    expect_any(SCB_CleanInvalidateDCache_by_Addr, addr);
    expect_any(SCB_CleanInvalidateDCache_by_Addr, dsize);
    expect_function_call(SCB_CleanInvalidateDCache_by_Addr);

    g_should_wrap_ddr_create_memory_map = true;
    expect_function_call(__wrap_ddr_create_memory_map);

    // hsp_send_ddr_init_notify()
    will_return(__wrap_system_info_is_warm_start, true); // Skip fpfw_icc_base_send_recv_sync

    // Telemetry init
    expect_function_calls(__wrap__txe_mutex_create, 2);

    // Crash dump pre-dump callback init
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    crash_dump_init(&context);

    // Mock gtimer for duration calculation
    will_return(__wrap_gtimer_prodfw_get_counter, 1000000);     // start timestamp
    will_return(__wrap_gtimer_prodfw_get_counter, 4000000);     // ddr training timestamp
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000); // 125 MHz
    will_return(__wrap_gtimer_prodfw_get_counter, 5000000);     // ddr init timestamp
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000); // 125 MHz

    ddr_manager_init(&ddr_service_ctx, &config, icc_ctx);
    g_should_wrap_ddr_create_memory_map = false;
}

TEST_FUNCTION(ecc_ce_timer_cb_success, NULL, NULL)
{
    ddr_service_context_t ddr_service_ctx = {};

    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    ecc_ce_timer_cb((ULONG)&ddr_service_ctx);
}

TEST_FUNCTION(rh_tlm_timer_cb_success, NULL, NULL)
{
    ddr_service_context_t ddr_service_ctx = {};

    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    rh_tlm_timer_cb((ULONG)&ddr_service_ctx);
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

    will_return(__wrap_ift_is_enabled, false);
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

TEST_FUNCTION(ddr_worker_thread_func_ift_enabled, NULL, NULL)
{
    ddr_service_context_t ddr_service_ctx = {};
    ddr_service_ctx.work_queue.tx_queue_start = (ULONG*)0x1234;

    will_return(__wrap_ift_is_enabled, true);

    if (!set_error_handler_return())
    {
        ddr_worker_thread_func((ULONG)&ddr_service_ctx);
    }
}

TEST_FUNCTION(ddr_create_bdat_test_single_die, NULL, NULL)
{
    uint8_t Bios_data_sign[] = {'B', 'D', 'A', 'T', 'H', 'E', 'A', 'D'};

    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    will_return(__wrap_ddrss_get_training_margin_base, &test_dq_training_margin);
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);

    // Clearing memory
    expect_function_calls(__wrap_mmio_write32, (BDAT_RESERVATION_SIZE) / 4);

    // BDAT header
    expect_function_calls(__wrap_mmio_write8, (sizeof(Bios_data_sign)));

    // BiosDataStructSize
    expect_function_call(__wrap_mmio_write32);

    // Reserved, PrimaryVersion, SecondaryVersion
    expect_function_calls(__wrap_mmio_write16, 3);

    // OemOffset, Reserved1, Reserved2
    expect_function_calls(__wrap_mmio_write32, 3);

    // Create BDAT schema list
    // SchemaListLength, Reserved, Year
    expect_function_calls(__wrap_mmio_write16, 3);

    // Month, Day, Hour, Minute, Second, Reserved1
    expect_function_calls(__wrap_mmio_write8, 6);

    // Schemas[0]
    expect_function_call(__wrap_mmio_write32);

    // refCodeRevision
    expect_function_calls(__wrap_mmio_write8, 4);

    // maxNode, maxCh, maxDimm, maxRankDimm, maxSubchannelRank
    expect_function_calls(__wrap_mmio_write8, 5);

    for (int socket_idx = 0; socket_idx < MAX_SOCKET; socket_idx++)
    {
        // PiStepUnit, RxVrefStepUnit, TxVrefStepUnit, CSVrefStepUnit, CAVrefStepUnit, DeviceVrefStepUnit, ddrVoltage, ddrFreq
        expect_function_calls(__wrap_mmio_write8, 16);

        will_return_maybe(__wrap_get_i3c_dimm_detected, 0x0FFF);
        for (int dimm_local_idx = 0; dimm_local_idx < DDRSS_SS_NUM_PER_DIE; dimm_local_idx++)
        {
            // chEnabled
            expect_function_calls(__wrap_mmio_write8, 1);

            // bdat_spd_valid_bytes
            expect_function_calls(__wrap_mmio_write8, (MAX_SPD_BYTE_1024 / 8));

            // spdData
            expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
            will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

            expect_function_calls(__wrap_mmio_write8, MAX_SPD_BYTE_1024);

            expect_function_call(__wrap_ddrss_phy_get_training_margin);
            will_return(__wrap_ddrss_phy_get_training_margin, SILIBS_SUCCESS);

            // RANK_STRUCTURE
            for (int rank = 0; rank < 2; rank++)
            {
                // rankEnabled
                expect_function_calls(__wrap_mmio_write8, 1);

                // subchannelList
                for (int subchannel = 0; subchannel < MAX_SUBCHANNEL; subchannel++)
                {
                    expect_function_calls(__wrap_mmio_write8, 10);
                }

                // New DQ margin data
                for (size_t dq_lane_margin_idx = 0; dq_lane_margin_idx < MAX_DDRSS_DQ_LANE_MARGIN_DBYTE_NUM;
                     dq_lane_margin_idx++)
                {
                    for (size_t tx_rx_lane_idx = 0; tx_rx_lane_idx < MAX_DDRSS_DQ_LANE_MARGIN_TX_RX_LANE_NUM; tx_rx_lane_idx++)
                    {
                        expect_function_calls(__wrap_mmio_write8, 8);
                    }
                }
            }
        }
    }

    // GUID
    expect_function_calls(__wrap_mmio_write8, 16);

    // schemaHeader.DataSize
    expect_function_calls(__wrap_mmio_write8, 4);

    // schemaHeader.Crc16 (as 0)
    expect_function_calls(__wrap_mmio_write8, 2);

    // BdatHeader.Crc16 (as 0)
    expect_function_calls(__wrap_mmio_write8, 2);

    // CalculateRemoteCheckSum16
    for (uint32_t i = 0; i < sizeof(BDAT_DATA_STRUCTURE_MSFT_5) / 2; i++)
    {
        will_return(__wrap_mmio_read16, 0xFFFF);
    }

    // CRC16 values
    expect_function_calls(__wrap_mmio_write16, 2);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    ddr_create_bdat();
}

TEST_FUNCTION(ddr_create_bdat_test_die_0, NULL, NULL)
{
    uint8_t Bios_data_sign[] = {'B', 'D', 'A', 'T', 'H', 'E', 'A', 'D'};

    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    will_return(__wrap_ddrss_get_training_margin_base, &test_dq_training_margin);

    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    // Clearing memory
    expect_function_calls(__wrap_mmio_write32, (BDAT_RESERVATION_SIZE) / 4);

    // BDAT header
    expect_function_calls(__wrap_mmio_write8, sizeof(Bios_data_sign));

    // BiosDataStructSize
    expect_function_call(__wrap_mmio_write32);

    // Reserved, PrimaryVersion, SecondaryVersion
    expect_function_calls(__wrap_mmio_write16, 3);

    // OemOffset, Reserved1, Reserved2
    expect_function_calls(__wrap_mmio_write32, 3);

    // Create BDAT schema list
    // SchemaListLength, Reserved, Year
    expect_function_calls(__wrap_mmio_write16, 3);

    // Month, Day, Hour, Minute, Second, Reserved1
    expect_function_calls(__wrap_mmio_write8, 6);

    // Schemas[0]
    expect_function_call(__wrap_mmio_write32);

    // refCodeRevision
    expect_function_calls(__wrap_mmio_write8, 4);

    // maxNode, maxCh, maxDimm, maxRankDimm, maxSubchannelRank
    expect_function_calls(__wrap_mmio_write8, 5);

    for (int socket_idx = 0; socket_idx < MAX_SOCKET; socket_idx++)
    {
        // PiStepUnit, RxVrefStepUnit, TxVrefStepUnit, CSVrefStepUnit, CAVrefStepUnit, DeviceVrefStepUnit, ddrVoltage, ddrFreq
        expect_function_calls(__wrap_mmio_write8, 16);

        will_return_maybe(__wrap_get_i3c_dimm_detected, 0x0FFF);
        for (int dimm_local_idx = 0; dimm_local_idx < DDRSS_SS_NUM_PER_DIE; dimm_local_idx++)
        {
            // chEnabled
            expect_function_calls(__wrap_mmio_write8, 1);

            // bdat_spd_valid_bytes
            expect_function_calls(__wrap_mmio_write8, (MAX_SPD_BYTE_1024 / 8));

            // spdData
            expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
            will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

            expect_function_calls(__wrap_mmio_write8, MAX_SPD_BYTE_1024);

            expect_function_call(__wrap_ddrss_phy_get_training_margin);
            will_return(__wrap_ddrss_phy_get_training_margin, SILIBS_SUCCESS);

            // RANK_STRUCTURE
            for (int rank = 0; rank < 2; rank++)
            {
                // rankEnabled
                expect_function_calls(__wrap_mmio_write8, 1);

                // subchannelList
                for (int subchannel = 0; subchannel < MAX_SUBCHANNEL; subchannel++)
                {
                    expect_function_calls(__wrap_mmio_write8, 10);
                }

                // New DQ margin data
                for (size_t dq_lane_margin_idx = 0; dq_lane_margin_idx < MAX_DDRSS_DQ_LANE_MARGIN_DBYTE_NUM;
                     dq_lane_margin_idx++)
                {
                    for (size_t tx_rx_lane_idx = 0; tx_rx_lane_idx < MAX_DDRSS_DQ_LANE_MARGIN_TX_RX_LANE_NUM; tx_rx_lane_idx++)
                    {
                        expect_function_calls(__wrap_mmio_write8, 8);
                    }
                }
            }
        }
    }

    // GUID
    expect_function_calls(__wrap_mmio_write8, 16);

    // schemaHeader.DataSize
    expect_function_calls(__wrap_mmio_write8, 4);

    // schemaHeader.Crc16 (as 0)
    expect_function_calls(__wrap_mmio_write8, 2);

    // BdatHeader.Crc16 (as 0)
    expect_function_calls(__wrap_mmio_write8, 2);

    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    // CalculateRemoteCheckSum16
    for (uint32_t i = 0; i < sizeof(BDAT_DATA_STRUCTURE_MSFT_5) / 2; i++)
    {
        will_return(__wrap_mmio_read16, 0xFFFF);
    }

    // CRC16 values
    expect_function_calls(__wrap_mmio_write16, 2);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    ddr_create_bdat();
}

TEST_FUNCTION(ddr_create_bdat_test_die_1, NULL, NULL)
{
    // cmocka_set_message_output(CM_OUTPUT_STDOUT);
    PRINT_EXPECT_CALL(__wrap_idsw_get_die_id, DIE_1, DIE_1);
    will_return(__wrap_idsw_get_die_id, DIE_1);

    PRINT_EXPECT_CALL(__wrap_atu_map, ATU_ID_MSCP, SILIBS_SUCCESS);
    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    will_return(__wrap_ddrss_get_training_margin_base, &test_dq_training_margin);
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);

    PRINT_EXPECT_CALL(__wrap_mscp_exp_spi_synchronize_dies, DIE_1, SILIBS_SUCCESS);
    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    will_return_maybe(__wrap_get_i3c_dimm_detected, 0x0FFF);
    for (int socket_idx = 0; socket_idx < MAX_SOCKET; socket_idx++)
    {
        for (int dimm_local_idx = 0; dimm_local_idx < DDRSS_SS_NUM_PER_DIE; dimm_local_idx++)
        {
            // chEnabled
            PRINT_EXPECT_CALL(__wrap_mmio_write8, addr, 1);
            expect_function_calls(__wrap_mmio_write8, 1);

            // bdat_spd_valid_bytes
            PRINT_EXPECT_CALL(__wrap_mmio_write8, addr, (MAX_SPD_BYTE_1024 / 8));
            expect_function_calls(__wrap_mmio_write8, (MAX_SPD_BYTE_1024 / 8));

            // spdData
            PRINT_EXPECT_CALL(__wrap_ddr_i3c_interface_read_spd_nvm_data, s_i3c_cmd, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
            will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

            PRINT_EXPECT_CALL(__wrap_mmio_write8, addr, MAX_SPD_BYTE_1024);
            expect_function_calls(__wrap_mmio_write8, MAX_SPD_BYTE_1024);

            PRINT_EXPECT_CALL(__wrap_ddrss_phy_get_training_margin, mc, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddrss_phy_get_training_margin);
            will_return(__wrap_ddrss_phy_get_training_margin, SILIBS_SUCCESS);

            // RANK_STRUCTURE
            for (int rank = 0; rank < 2; rank++)
            {
                // rankEnabled
                PRINT_EXPECT_CALL(__wrap_mmio_write8, addr, 1);
                expect_function_calls(__wrap_mmio_write8, 1);

                // subchannelList
                for (int subchannel = 0; subchannel < MAX_SUBCHANNEL; subchannel++)
                {
                    PRINT_EXPECT_CALL(__wrap_mmio_write8, addr, 10);
                    expect_function_calls(__wrap_mmio_write8, 10);
                }

                // New DQ margin data
                for (size_t dq_lane_margin_idx = 0; dq_lane_margin_idx < MAX_DDRSS_DQ_LANE_MARGIN_DBYTE_NUM;
                     dq_lane_margin_idx++)
                {
                    for (size_t tx_rx_lane_idx = 0; tx_rx_lane_idx < MAX_DDRSS_DQ_LANE_MARGIN_TX_RX_LANE_NUM; tx_rx_lane_idx++)
                    {
                        expect_function_calls(__wrap_mmio_write8, 8);
                    }
                }
            }
        }
    }

    PRINT_EXPECT_CALL(__wrap_mscp_exp_spi_synchronize_dies, DIE_1, SILIBS_SUCCESS);
    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    PRINT_EXPECT_CALL(__wrap_atu_unmap, ATU_ID_MSCP, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    ddr_create_bdat();
}

TEST_FUNCTION(ddr_create_smbios_tables_test_die_0, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    // Clearing memory
    expect_function_calls(__wrap_mmio_write32, (SMBIOS_HANDOFF_RESERVATION_SIZE) / 4);

    // Write Type16 table to memory
    expect_function_calls(__wrap_mmio_write8, sizeof(SMBIOS_PHYS_MEM_ARRAY_16));

    // Write double-null terminator
    expect_function_calls(__wrap_mmio_write8, 2);

    will_return_always(__wrap_idhw_is_single_die_boot_en, false);

    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
        will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

        will_return(__wrap_ddrss_is_valid_local_mc, true);

        // Check expected Type17 parameters
        g_check_smb17_params = true;
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->Type, 17);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->Length, UEFI_SPEC33_SMBIOS17_LENGTH);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->Handle, 0);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->PhysMemoryArrayHandle, 0);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->MemoryErrorInfoHandle, 0);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->TotalWidth, 80);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->DataWidth, 64);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->FormFactor, 0x09);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->DeviceSet, 0xFF);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->MemoryType, 0x22);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->TypeDetail, 0x2000);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->Attributes, 2); // Rank hardcoded for EF
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->ExtendedSize, 0);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->MinVoltage, TEST_DDRSS_DDR5_RDIMM_VDD_MV);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->MaxVoltage, TEST_DDRSS_DDR5_RDIMM_VPP_MV);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->ConfiguredVoltage, TEST_DDRSS_DDR5_RDIMM_VDD_MV);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->MemoryTechnology, 0x03);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->MemoryOperatingModeCapability, 8);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->MemorySubsystemControllerManufacturerID, TEST_JEDEC_MICROSOFT_ID);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->MemorySubsystemControllerProductID, TEST_JEDEC_MICROSOFT_ID);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->NonvolatileSize, 0);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->CacheSize, 0);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->LogicalSize, 0);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->ExtendedSpeed, 0);
        expect_value(__wrap_copy_single_smbios_type_17, smb_table17->ExtendedConfiguredMemorySpeed, 0);

        // Write Type17 table to memory
        expect_function_calls(__wrap_mmio_write8, sizeof(SMBIOS_MEM_DEVICE_17));

        // Write strings... [DeviceLocator]
        const char* device_locator = "SLOT A";
        expect_function_calls(__wrap_mmio_write8, strlen(device_locator) + 1);

        // Write strings... [BankLocator]
        const char* bank_locator = "BANK A";
        expect_function_calls(__wrap_mmio_write8, strlen(bank_locator) + 1);

        expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
        will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

        // Write strings... [Manufacturer]
        const char* manufacturer = "MICRON";
        will_return(__wrap_dimm_vendor_from_id, manufacturer);
        expect_function_calls(__wrap_mmio_write8, strlen(manufacturer) + 1);

        // Write strings... [SerialNumber]
        const char* serial_number = "12345678";
        will_return(__wrap_get_dimm_serial_number_string, serial_number);
        expect_function_calls(__wrap_mmio_write8, strlen(serial_number) + 1);

        // Write strings... [AssetTag]
        const char* asset_tag = "N/A";
        expect_function_calls(__wrap_mmio_write8, strlen(asset_tag) + 1);

        // Write strings... [PartNumber]
        const char* part_number = "MT8JTF25664HZ-1G4F1";
        will_return(__wrap_get_dimm_part_number_string, part_number);
        expect_function_calls(__wrap_mmio_write8, strlen(part_number) + 1);

        // Write strings.. [PHY FW Version]
        expect_function_calls(__wrap_mmio_write8, ((sizeof(unsigned long)) * 2) + strlen("0x") + 1);

        // Additional NULL byte between type17 tables
        expect_function_calls(__wrap_mmio_write8, 1);
    }

    // Write SMBIOS length
    expect_function_calls(__wrap_mmio_write32, 1);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    ddr_create_smbios_tables();
    g_check_smb17_params = false;
}

TEST_FUNCTION(ddr_create_smbios_tables_test_die_1, NULL, NULL)
{
    ddr_service_context_t ddr_service_ctx = {};
    ddr_service_ctx.work_queue.tx_queue_start = (ULONG*)0x1234;

    will_return(__wrap_ift_is_enabled, false);

    expect_value(__wrap__txe_queue_receive, queue_ptr, &(ddr_service_ctx.work_queue));
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, (ULONG)TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    int callback_param = DDR_CREATE_SMBIOS_TABLES_EVENT;
    will_return(tx_queue_copy_parameter, callback_param);
    set_txe_queue_receive_callback_func(tx_queue_copy_parameter);

    // ddr_create_smbios_tables()
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    will_return_always(__wrap_idhw_is_single_die_boot_en, false);

    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    // Read SMBIOS length
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
        will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

        will_return(__wrap_ddrss_is_valid_local_mc, true);

        // Write Type17 table to memory
        expect_function_calls(__wrap_mmio_write8, sizeof(SMBIOS_MEM_DEVICE_17));

        // Write strings... [DeviceLocator]
        const char* device_locator = "SLOT A";
        expect_function_calls(__wrap_mmio_write8, strlen(device_locator) + 1);

        // Write strings... [BankLocator]
        const char* bank_locator = "BANK A";
        expect_function_calls(__wrap_mmio_write8, strlen(bank_locator) + 1);

        expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
        will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

        // Write strings... [Manufacturer]
        const char* manufacturer = "MICRON";
        will_return(__wrap_dimm_vendor_from_id, manufacturer);
        expect_function_calls(__wrap_mmio_write8, strlen(manufacturer) + 1);

        // Write strings... [SerialNumber]
        const char* serial_number = "12345678";
        will_return(__wrap_get_dimm_serial_number_string, serial_number);
        expect_function_calls(__wrap_mmio_write8, strlen(serial_number) + 1);

        // Write strings... [AssetTag]
        const char* asset_tag = "N/A";
        expect_function_calls(__wrap_mmio_write8, strlen(asset_tag) + 1);

        // Write strings... [PartNumber]
        const char* part_number = "MT8JTF25664HZ-1G4F1";
        will_return(__wrap_get_dimm_part_number_string, part_number);
        expect_function_calls(__wrap_mmio_write8, strlen(part_number) + 1);

        // Write strings.. [PHY FW Version]
        expect_function_calls(__wrap_mmio_write8, ((sizeof(unsigned long)) * 2) + strlen("0x") + 1);

        // Additional NULL byte between type17 tables
        expect_function_calls(__wrap_mmio_write8, 1);
    }

    // Additional 8 byte for the end
    expect_function_calls(__wrap_mmio_write8, 8);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    // end of ddr_create_smbios_tables()

    // Exit the while (1) loop
    callback_param = 0xFF;
    will_return(tx_queue_copy_parameter, callback_param);
    expect_value(__wrap__txe_queue_receive, queue_ptr, &(ddr_service_ctx.work_queue));
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, (ULONG)TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    // Expect the error
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_EMPTY);

    if (!set_error_handler_return())
    {
        ddr_worker_thread_func((ULONG)&ddr_service_ctx);
    }
}

TEST_FUNCTION(ddr_start_i3c_and_ecc_ce_timer, NULL, NULL)
{
    int callback_param = DDR_START_POLLING_TIMER_EVENT;
    will_return(tx_queue_copy_parameter, callback_param);
    set_txe_queue_receive_callback_func(tx_queue_copy_parameter);

    ddr_service_context_t ddr_service_ctx = {};
    ddr_service_ctx.work_queue.tx_queue_start = (ULONG*)0x1234;

    will_return(__wrap_ift_is_enabled, false);
    expect_value(__wrap__txe_queue_receive, queue_ptr, &(ddr_service_ctx.work_queue));
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, (ULONG)TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    // Check that I3C polling timer is created/started
    will_return(__wrap_config_get_ddrmanager_bwl_polling_en, true);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    // Check that ECC CE HW workaround timer is created/started
    will_return(__wrap_config_get_ras_init_en, true);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    // Check that row hammer polling timer is created/started
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 1000);
    will_return(__wrap_config_get_erhm_en, true);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    // Exit the while (1) loop
    callback_param = 0xFF;
    will_return(tx_queue_copy_parameter, callback_param);
    expect_value(__wrap__txe_queue_receive, queue_ptr, &(ddr_service_ctx.work_queue));
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, (ULONG)TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    // Expect the error
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_QUEUE_EMPTY);

    if (!set_error_handler_return())
    {
        ddr_worker_thread_func((ULONG)&ddr_service_ctx);
    }
}

TEST_FUNCTION(ddr_worker_quiesce_exits_thread, NULL, NULL)
{
    ddr_service_context_t ddr_service_ctx = {};
    ddr_service_ctx.work_queue.tx_queue_start = (ULONG*)0x1234;

    will_return(__wrap_ift_is_enabled, false);

    int callback_param = DDR_QUIESCE_EVENT;
    will_return(tx_queue_copy_parameter, callback_param);
    set_txe_queue_receive_callback_func(tx_queue_copy_parameter);

    callback_param = 0xFF;
    will_return(tx_queue_copy_parameter, callback_param);
    expect_value(__wrap__txe_queue_receive, queue_ptr, &(ddr_service_ctx.work_queue));
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, (ULONG)TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    // Drain loop uses TX_NO_WAIT; return empty once
    expect_value(__wrap__txe_queue_receive, queue_ptr, &(ddr_service_ctx.work_queue));
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, (ULONG)TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    // After handling the quiesce event the worker should exit the loop and return.
    ddr_worker_thread_func((ULONG)&ddr_service_ctx);
}

TEST_FUNCTION(test_rhtlm_cfg_scan_no_telemetry, NULL, NULL)
{

    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_get_telemetry_record, SILIBS_E_DATA, RHTLM_MC_DIE0_COUNT_NR);

    expect_value(__wrap_hm_submit_cper, error_domain_idx, ACPI_ERROR_DOMAIN_RHTLM);
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_function_call(__wrap_hm_submit_cper);

    rhtlm_cfg_scan();
}

TEST_FUNCTION(test_rhtlm_cfg_scan_no_telemetry_die1, NULL, NULL)
{

    will_return(__wrap_idsw_get_die_id, DIE_1);
    will_return_count(__wrap_ddrss_get_telemetry_record, SILIBS_E_DATA, (RHTLM_MC_DIE1_COUNT_NR - RHTLM_MC_DIE0_COUNT_NR));

    expect_value(__wrap_hm_submit_cper, error_domain_idx, ACPI_ERROR_DOMAIN_RHTLM);
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_function_call(__wrap_hm_submit_cper);

    rhtlm_cfg_scan();
}

TEST_FUNCTION(test_rhtlm_cfg_scan_telemetry, NULL, NULL)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);

    for (int i = 0; i < RHTLM_MC_DIE0_COUNT_NR; i++)
    {
        will_return(__wrap_ddrss_get_telemetry_record, SILIBS_SUCCESS);
        expect_function_call(__wrap_prod_ddrss_convert_rh_cfg_rec_to_rh_cper);
        expect_value(__wrap_hm_submit_cper, error_domain_idx, ACPI_ERROR_DOMAIN_RHTLM);
        expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
        expect_function_call(__wrap_hm_submit_cper);
    }
    rhtlm_cfg_scan();
}

TEST_FUNCTION(ddr_telemetry_report_verify_temps, NULL, NULL)
{
    ddr_manager_i3c_temperature_t dimm_temp = {.is_positive = true};
    sensor_ram_dimm_info_t test_dimm_info = {0};
    test_dimm_info.dimm_memory_frequency_id = (uint8_t)config_get_ddr_speed_grade();
    const int POWER_SCALING_FACTOR = 125;
    const int THROTTLE_COUNT_TO_TEST = 0;

    ddr_telemetry_reset_throttle_count();

    will_return(__wrap_gtimer_prodfw_get_counter, 0);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        will_return(__wrap_ddr_i3c_interface_read_pmic_power, (dimm_idx));
        test_dimm_info.dimm_id = dimm_idx;
        test_dimm_info.dimm_power_mW = (dimm_idx * POWER_SCALING_FACTOR);
        test_dimm_info.dimm_throttle_count = THROTTLE_COUNT_TO_TEST;

        for (int ts_idx = 0; ts_idx < NUM_TEMP_SENSORS_PER_DIMM; ts_idx++)
        {
            dimm_temp.temp_int = (10 * dimm_idx) + ts_idx;
            dimm_temp.temp_frac = 50;
            ddr_telemetry_update_dimm_temp(dimm_idx, ts_idx, dimm_temp);

            if (ts_idx == 0)
            {
                test_dimm_info.dimm_temp_s0_dC = (10 * dimm_temp.temp_int) + ((dimm_temp.temp_frac + 5) / 10);
            }
            else
            {
                test_dimm_info.dimm_temp_s1_dC = (10 * dimm_temp.temp_int) + ((dimm_temp.temp_frac + 5) / 10);
            }
        }

        expect_memory(__wrap_sensor_fifo_svc_add_dimm_info, dimm_info, &test_dimm_info, sizeof(test_dimm_info));
    }

    ddr_telemetry_report();
}

TEST_FUNCTION(ddr_telemetry_check_throttling_count, NULL, NULL)
{
    ddr_manager_i3c_temperature_t dimm_temp = {.is_positive = true};
    sensor_ram_dimm_info_t test_dimm_info = {0};
    test_dimm_info.dimm_memory_frequency_id = (uint8_t)config_get_ddr_speed_grade();
    const int POWER_SCALING_FACTOR = 125;
    const int THROTTLE_COUNT_TO_TEST = 2;

    for (int throttle_count = 0; throttle_count < THROTTLE_COUNT_TO_TEST; throttle_count++)
    {
        // Arrange - Enable BWL MR4
        expect_function_call(__wrap_mmio_read32);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_write32);
        will_return(__wrap_idsw_get_die_id, DIE_0);
        will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);
        will_return(__wrap_gtimer_prodfw_get_counter, 100);

        // Act
        ddr_manager_enable_bwl_mr4();

        // Arrange
        expect_function_call(__wrap_mmio_read32);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_write32);
        will_return(__wrap_idsw_get_die_id, DIE_0);
        will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);
        will_return(__wrap_gtimer_prodfw_get_counter, 100);

        // Act
        ddr_manager_disable_bwl_mr4();
    }

    will_return(__wrap_gtimer_prodfw_get_counter, 0);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        will_return(__wrap_ddr_i3c_interface_read_pmic_power, (dimm_idx));
        test_dimm_info.dimm_id = dimm_idx;
        test_dimm_info.dimm_power_mW = (dimm_idx * POWER_SCALING_FACTOR);
        test_dimm_info.dimm_throttle_count = THROTTLE_COUNT_TO_TEST;

        for (int ts_idx = 0; ts_idx < NUM_TEMP_SENSORS_PER_DIMM; ts_idx++)
        {
            dimm_temp.temp_int = (11 * dimm_idx) + ts_idx;
            dimm_temp.temp_frac = 50;
            ddr_telemetry_update_dimm_temp(dimm_idx, ts_idx, dimm_temp);

            if (ts_idx == 0)
            {
                test_dimm_info.dimm_temp_s0_dC = (10 * dimm_temp.temp_int) + ((dimm_temp.temp_frac + 5) / 10);
            }
            else
            {
                test_dimm_info.dimm_temp_s1_dC = (10 * dimm_temp.temp_int) + ((dimm_temp.temp_frac + 5) / 10);
            }
        }

        expect_memory(__wrap_sensor_fifo_svc_add_dimm_info, dimm_info, &test_dimm_info, sizeof(test_dimm_info));
    }

    ddr_telemetry_report();
}

// Tests aimed at increasing coverage for ddr_manager_platform.c

TEST_FUNCTION(ddr_copy_empty_sdl_to_ddr, NULL, NULL)
{
    // Create a local copy of what the header should look like for checksum calculation
    MEMORY_DEFECT_LIST_HEADER expected_header_for_checksum = {0};
    expected_header_for_checksum.Signature = (uint32_t)PSHED_PI_DEFECT_LIST_SIGNATURE;
    expected_header_for_checksum.Version = MEMORY_DEFECT_VERSION_20;
    expected_header_for_checksum.Length = sizeof(MEMORY_DEFECT_LIST_HEADER);
    expected_header_for_checksum.DefectCount = 0;
    expected_header_for_checksum.Changed = 0;
    expected_header_for_checksum.Checksum = 0; // This is 0 during checksum calculation
    expected_header_for_checksum.Reserved1 = 0;
    expected_header_for_checksum.Reserved2 = 0;

    will_return_always(__wrap_mmio_read16, 0x0);

    copy_empty_sdl_header_to_reserved_ddr((uintptr_t)&empty_header);

    // Verify that the SDL header was copied correctly
    assert_int_equal(empty_header.Signature, (uint32_t)PSHED_PI_DEFECT_LIST_SIGNATURE);
    assert_int_equal(empty_header.Version, MEMORY_DEFECT_VERSION_20);
    assert_int_equal(empty_header.Length, sizeof(expected_header_for_checksum));
    assert_int_equal(empty_header.DefectCount, 0);
    assert_int_equal(empty_header.Changed, 0);

    // Calculate the expected checksum by using the same algorithm/endianess
    uint16_t expected_checksum =
        CalculateRemoteCheckSum16((uint32_t)&expected_header_for_checksum, sizeof(MEMORY_DEFECT_LIST_HEADER));

    assert_int_equal(empty_header.Checksum, expected_checksum);
}

TEST_FUNCTION(ddr_sdl_load_success, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    // SDL load
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)sdl_test_buffer);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);
    will_return(__wrap_variable_service_sync_get_variable, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    load_shared_defect_list_to_DDR();
}

TEST_FUNCTION(ddr_sdl_load_not_found, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_atu_map, &empty_header);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    // SDL load
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)sdl_test_buffer);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    will_return(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);

    // Expect to create empty SDL and copy to DDR
    will_return_always(__wrap_mmio_read16, 0x0); // For CalculateRemoteCheckSum16

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    load_shared_defect_list_to_DDR();
}

TEST_FUNCTION(ddr_sdl_load_alternate_path_1, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_1);

    // Nothing happens for DIE_1 (noop)
    load_shared_defect_list_to_DDR();
}

TEST_FUNCTION(ddr_ddr_manager_dfwk_init_test, NULL, NULL)
{
    DFWK_INTERFACE_HEADER ssi_interface = {};
    DFWK_THREADX_HOST mock_dfwk_host;
    memset(&mock_dfwk_host, 0, sizeof(mock_dfwk_host));

    will_return(__wrap_fpfw_init_get_handle, &mock_dfwk_host);
    will_return(__wrap_fpfw_init_get_handle, &ssi_interface);

    expect_value(__wrap_sos_register_ssi, p_interface, &ssi_interface);
    expect_any(__wrap_sos_register_ssi, p_registration);
    expect_any(__wrap_sos_register_ssi, p_ssi_interface);
    will_return(__wrap_sos_register_ssi, FPFW_INIT_STATUS_SUCCESS);

    ddr_manager_dfwk_init();
}

TEST_FUNCTION(ddr_manager_dfwk_dispatch_test_die_0, NULL, NULL)
{
    uint8_t sdl_payload[2048] = {0};
    ddr_service_context_t* ddr_service_ctx = ddr_get_service_context();

    ssi_shutdown_notification_request_t mock_request = {};
    mock_request.header.RequestType = SSI_QUIESCE_ASYNC;
    mock_request.shutdown_type = SHUTDOWN_SCP_INITIATED;

    // Mock gtimer for quiesce timing measurement
    will_return(__wrap_gtimer_prodfw_get_counter, 1000000); // start timestamp

    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx->work_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, (ULONG)TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // Mock semaphore wait for quiesce
    expect_any(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_get, wait_option);
    will_return(__wrap__txe_semaphore_get, TX_SUCCESS);

    // should_store_sdl = true
    will_return(__wrap_config_get_ddrmanager_sdl_en, true);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    // store_sdl_var_async(): first mock payload_base, then max_payload_size
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)&sdl_payload[0]);
    will_return(__wrap_sdl_get_mem_ctx, sizeof(sdl_payload));
    will_return(__wrap_variable_service_initialize_ctx, 0);

    will_return(__wrap_atu_map, 0x012345678);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_function_call(__wrap_variable_service_async_set_variable);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    will_return(__wrap_gtimer_prodfw_get_counter, 2000000);     // end timestamp
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000); // 125 MHz

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &mock_request);
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    ddr_manager_dfwk_dispatch(&mock_request.header, NULL);
}

TEST_FUNCTION(ddr_manager_dfwk_dispatch_test_die_1, NULL, NULL)
{
    ddr_service_context_t* ddr_service_ctx = ddr_get_service_context();

    ssi_shutdown_notification_request_t mock_request = {};
    mock_request.header.RequestType = SSI_QUIESCE_ASYNC;
    mock_request.shutdown_type = SHUTDOWN_SCP_INITIATED;

    // Mock gtimer for quiesce timing measurement
    will_return(__wrap_gtimer_prodfw_get_counter, 1000000);     // start timestamp
    will_return(__wrap_gtimer_prodfw_get_counter, 2000000);     // end timestamp
    will_return(__wrap_gtimer_prodfw_get_frequency, 125000000); // 125 MHz

    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx->work_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, (ULONG)TX_NO_WAIT);
    will_return_always(__wrap__txe_queue_send, TX_SUCCESS);

    // Mock semaphore wait for quiesce
    expect_any(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_any(__wrap__txe_semaphore_get, wait_option);
    will_return(__wrap__txe_semaphore_get, TX_SUCCESS);

    // should_store_sdl = false
    will_return(__wrap_idsw_get_die_id, DIE_1);

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &mock_request);
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    ddr_manager_dfwk_dispatch(&mock_request.header, NULL);
}
