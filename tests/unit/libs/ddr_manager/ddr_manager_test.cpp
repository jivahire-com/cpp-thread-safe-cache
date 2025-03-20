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
#include <ddr_i3c.h>
#include <ddr_manager.h>   // for ddr_manager_init, ddr_service_context_t
#include <ddr_manager_i.h> // for ddr_poll_dimms, ddr_worker_thread_func
#include <ddrss_lib.h>
#include <error_handler.h> // for set_error_handler_return
#include <fpfw_icc_base.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <kingsgate_hsp_mailbox_commands.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <smbios_structs.h>
#include <tx_api.h> // for TX_SUCCESS, ULONG, TX_NOT_DONE, TX_NO_MEMORY

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

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* icc_ctx;
static bool g_should_wrap_idsw_get_platform_sdv = false;
mscp_exp_spi_sync_point_t d2d_sync_point_test = {};

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit, UINT mutex_control_block_size)
{
    assert_non_null(mutex_ptr); // Ensure the mutex pointer is not NULL

    assert_non_null(name_ptr);
    UNUSED(inherit);
    UNUSED(mutex_control_block_size);

    function_called();

    return 0;
}

void __wrap_mmio_write8(volatile uint8_t* addr, uint8_t data)
{
    check_expected_ptr(addr);
    check_expected(data);
}
uint8_t __wrap_mmio_read8(volatile uint8_t* addr)
{
    check_expected_ptr(addr);
    return mock_type(uint8_t);
}
void __wrap_mmio_write16(volatile uint16_t* addr, uint16_t data)
{
    check_expected_ptr(addr);
    check_expected(data);
}
uint16_t __wrap_mmio_read16(volatile uint16_t* addr)
{

    check_expected_ptr(addr);
    return mock_type(uint16_t);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == nullptr)
    {
        return SILIBS_E_PARAM;
    }

    atu_map_entry->mscp_start_address = 0x60100000;

    return mock_type(int);
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

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == nullptr)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(int);
}

idsw_die_id_t __wrap_idsw_get_die_id()
{
    return mock_type(idsw_die_id_t);
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

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    if (g_should_wrap_idsw_get_platform_sdv)
    {
        return mock_type(KNG_PLAT_ID);
    }
    else
    {
        return PLATFORM_RVP_EVT_SILICON;
    }
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
    expect_value(__wrap__txe_thread_create, auto_start, TX_AUTO_START);
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

TEST_FUNCTION(ddr_create_bdat_test_single_die, NULL, NULL)
{
    uint8_t Bios_data_sign[] = {'B', 'D', 'A', 'T', 'H', 'E', 'A', 'D'};

    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    will_return_always(__wrap_idhw_is_single_die_boot_en, true);

    // Clearing memory
    expect_function_calls(__wrap_mmio_write32, (BDAT_RESERVATION_SIZE) / 4);

    // BDAT header
    expect_any_count(__wrap_mmio_write8, addr, sizeof(Bios_data_sign));
    expect_any_count(__wrap_mmio_write8, data, sizeof(Bios_data_sign));

    // BiosDataStructSize
    expect_function_call(__wrap_mmio_write32);

    // Reserved, PrimaryVersion, SecondaryVersion
    expect_any_count(__wrap_mmio_write16, addr, 3);
    expect_any_count(__wrap_mmio_write16, data, 3);

    // OemOffset, Reserved1, Reserved2
    expect_function_calls(__wrap_mmio_write32, 3);

    // Create BDAT schema list
    // SchemaListLength, Reserved, Year
    expect_any_count(__wrap_mmio_write16, addr, 3);
    expect_any_count(__wrap_mmio_write16, data, 3);

    // Month, Day, Hour, Minute, Second, Reserved1
    expect_any_count(__wrap_mmio_write8, addr, 6);
    expect_any_count(__wrap_mmio_write8, data, 6);

    // Schemas[0]
    expect_function_call(__wrap_mmio_write32);

    // refCodeRevision
    expect_any_count(__wrap_mmio_write8, addr, 4);
    expect_any_count(__wrap_mmio_write8, data, 4);

    // maxNode, maxCh, maxRankDimm, maxSubchannelRank
    expect_any_count(__wrap_mmio_write8, addr, 4);
    expect_any_count(__wrap_mmio_write8, data, 4);

    for (int socket_idx = 0; socket_idx < MAX_SOCKET; socket_idx++)
    {
        // PiStepUnit, RxVrefStepUnit, TxVrefStepUnit, CSVrefStepUnit, CAVrefStepUnit, DeviceVrefStepUnit, ddrVoltage, ddrFreq
        expect_any_count(__wrap_mmio_write16, addr, 8);
        expect_any_count(__wrap_mmio_write16, data, 8);

        will_return_maybe(__wrap_get_i3c_dimm_detected, 0x0FFF);
        for (int dimm_local_idx = 0; dimm_local_idx < DDRSS_SS_NUM_PER_DIE; dimm_local_idx++)
        {
            // chEnabled
            expect_any_count(__wrap_mmio_write8, addr, 1);
            expect_any_count(__wrap_mmio_write8, data, 1);

            // bdat_spd_valid_bytes
            expect_any_count(__wrap_mmio_write8, addr, (MAX_SPD_BYTE_1024 / 8));
            expect_any_count(__wrap_mmio_write8, data, (MAX_SPD_BYTE_1024 / 8));

            // spdData
            expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
            will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

            expect_any_count(__wrap_mmio_write8, addr, MAX_SPD_BYTE_1024);
            expect_any_count(__wrap_mmio_write8, data, MAX_SPD_BYTE_1024);

            expect_function_call(__wrap_ddrss_phy_get_training_margin);
            will_return(__wrap_ddrss_phy_get_training_margin, SILIBS_SUCCESS);

            // RANK_STRUCTURE
            for (int rank = 0; rank < 2; rank++)
            {
                // rankEnabled
                expect_any_count(__wrap_mmio_write8, addr, 1);
                expect_any_count(__wrap_mmio_write8, data, 1);

                // subchannelList
                for (int subchannel = 0; subchannel < MAX_SUBCHANNEL; subchannel++)
                {
                    expect_any_count(__wrap_mmio_write8, addr, 10);
                    expect_any_count(__wrap_mmio_write8, data, 10);
                }
            }
        }
    }

    // GUID
    expect_any_count(__wrap_mmio_write8, addr, 16);
    expect_any_count(__wrap_mmio_write8, data, 16);

    // schemaHeader.DataSize
    expect_function_call(__wrap_mmio_write32);

    // schemaHeader.Crc16 (as 0)
    expect_any_count(__wrap_mmio_write16, addr, 1);
    expect_any_count(__wrap_mmio_write16, data, 1);

    // BdatHeader.Crc16 (as 0)
    expect_any_count(__wrap_mmio_write16, addr, 1);
    expect_any_count(__wrap_mmio_write16, data, 1);

    // CalculateRemoteCheckSum16
    for (uint32_t i = 0; i < sizeof(BDAT_DATA_STRUCTURE_MSFT_4) / 2; i++)
    {
        expect_any_count(__wrap_mmio_read16, addr, 1);
        will_return(__wrap_mmio_read16, 0xFFFF);
    }

    // CRC16 values
    expect_any_count(__wrap_mmio_write16, addr, 2);
    expect_any_count(__wrap_mmio_write16, data, 2);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    ddr_create_bdat();
}

TEST_FUNCTION(ddr_create_bdat_test_die_0, NULL, NULL)
{
    uint8_t Bios_data_sign[] = {'B', 'D', 'A', 'T', 'H', 'E', 'A', 'D'};

    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    // Clearing memory
    expect_function_calls(__wrap_mmio_write32, (BDAT_RESERVATION_SIZE) / 4);

    // BDAT header
    expect_any_count(__wrap_mmio_write8, addr, sizeof(Bios_data_sign));
    expect_any_count(__wrap_mmio_write8, data, sizeof(Bios_data_sign));

    // BiosDataStructSize
    expect_function_call(__wrap_mmio_write32);

    // Reserved, PrimaryVersion, SecondaryVersion
    expect_any_count(__wrap_mmio_write16, addr, 3);
    expect_any_count(__wrap_mmio_write16, data, 3);

    // OemOffset, Reserved1, Reserved2
    expect_function_calls(__wrap_mmio_write32, 3);

    // Create BDAT schema list
    // SchemaListLength, Reserved, Year
    expect_any_count(__wrap_mmio_write16, addr, 3);
    expect_any_count(__wrap_mmio_write16, data, 3);

    // Month, Day, Hour, Minute, Second, Reserved1
    expect_any_count(__wrap_mmio_write8, addr, 6);
    expect_any_count(__wrap_mmio_write8, data, 6);

    // Schemas[0]
    expect_function_call(__wrap_mmio_write32);

    // refCodeRevision
    expect_any_count(__wrap_mmio_write8, addr, 4);
    expect_any_count(__wrap_mmio_write8, data, 4);

    // maxNode, maxCh, maxRankDimm, maxSubchannelRank
    expect_any_count(__wrap_mmio_write8, addr, 4);
    expect_any_count(__wrap_mmio_write8, data, 4);

    for (int socket_idx = 0; socket_idx < MAX_SOCKET; socket_idx++)
    {
        // PiStepUnit, RxVrefStepUnit, TxVrefStepUnit, CSVrefStepUnit, CAVrefStepUnit, DeviceVrefStepUnit, ddrVoltage, ddrFreq
        expect_any_count(__wrap_mmio_write16, addr, 8);
        expect_any_count(__wrap_mmio_write16, data, 8);

        will_return_maybe(__wrap_get_i3c_dimm_detected, 0x0FFF);
        for (int dimm_local_idx = 0; dimm_local_idx < DDRSS_SS_NUM_PER_DIE; dimm_local_idx++)
        {
            // chEnabled
            expect_any_count(__wrap_mmio_write8, addr, 1);
            expect_any_count(__wrap_mmio_write8, data, 1);

            // bdat_spd_valid_bytes
            expect_any_count(__wrap_mmio_write8, addr, (MAX_SPD_BYTE_1024 / 8));
            expect_any_count(__wrap_mmio_write8, data, (MAX_SPD_BYTE_1024 / 8));

            // spdData
            expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
            will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

            expect_any_count(__wrap_mmio_write8, addr, MAX_SPD_BYTE_1024);
            expect_any_count(__wrap_mmio_write8, data, MAX_SPD_BYTE_1024);

            expect_function_call(__wrap_ddrss_phy_get_training_margin);
            will_return(__wrap_ddrss_phy_get_training_margin, SILIBS_SUCCESS);

            // RANK_STRUCTURE
            for (int rank = 0; rank < 2; rank++)
            {
                // rankEnabled
                expect_any_count(__wrap_mmio_write8, addr, 1);
                expect_any_count(__wrap_mmio_write8, data, 1);

                // subchannelList
                for (int subchannel = 0; subchannel < MAX_SUBCHANNEL; subchannel++)
                {
                    expect_any_count(__wrap_mmio_write8, addr, 10);
                    expect_any_count(__wrap_mmio_write8, data, 10);
                }
            }
        }
    }

    // GUID
    expect_any_count(__wrap_mmio_write8, addr, 16);
    expect_any_count(__wrap_mmio_write8, data, 16);

    // schemaHeader.DataSize
    expect_function_call(__wrap_mmio_write32);

    // schemaHeader.Crc16 (as 0)
    expect_any_count(__wrap_mmio_write16, addr, 1);
    expect_any_count(__wrap_mmio_write16, data, 1);

    // BdatHeader.Crc16 (as 0)
    expect_any_count(__wrap_mmio_write16, addr, 1);
    expect_any_count(__wrap_mmio_write16, data, 1);

    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    // CalculateRemoteCheckSum16
    for (uint32_t i = 0; i < sizeof(BDAT_DATA_STRUCTURE_MSFT_4) / 2; i++)
    {
        expect_any_count(__wrap_mmio_read16, addr, 1);
        will_return(__wrap_mmio_read16, 0xFFFF);
    }

    // CRC16 values
    expect_any_count(__wrap_mmio_write16, addr, 2);
    expect_any_count(__wrap_mmio_write16, data, 2);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    ddr_create_bdat();
}

TEST_FUNCTION(ddr_create_bdat_test_die_1, NULL, NULL)
{
    // cmocka_set_message_output(CM_OUTPUT_STDOUT);
    PRINT_EXPECT_CALL(__wrap_idsw_get_die_id, DIE_1, DIE_1);
    will_return(__wrap_idsw_get_die_id, DIE_1);

    PRINT_EXPECT_CALL(__wrap_atu_map, ATU_ID_MSCP, SILIBS_SUCCESS);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

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
            expect_any_count(__wrap_mmio_write8, addr, 1);
            expect_any_count(__wrap_mmio_write8, data, 1);

            // bdat_spd_valid_bytes
            PRINT_EXPECT_CALL(__wrap_mmio_write8, addr, (MAX_SPD_BYTE_1024 / 8));
            expect_any_count(__wrap_mmio_write8, addr, (MAX_SPD_BYTE_1024 / 8));
            expect_any_count(__wrap_mmio_write8, data, (MAX_SPD_BYTE_1024 / 8));

            // spdData
            PRINT_EXPECT_CALL(__wrap_ddr_i3c_interface_read_spd_nvm_data, s_i3c_cmd, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
            will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

            PRINT_EXPECT_CALL(__wrap_mmio_write8, addr, MAX_SPD_BYTE_1024);
            expect_any_count(__wrap_mmio_write8, addr, MAX_SPD_BYTE_1024);
            expect_any_count(__wrap_mmio_write8, data, MAX_SPD_BYTE_1024);

            PRINT_EXPECT_CALL(__wrap_ddrss_phy_get_training_margin, mc, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddrss_phy_get_training_margin);
            will_return(__wrap_ddrss_phy_get_training_margin, SILIBS_SUCCESS);

            // RANK_STRUCTURE
            for (int rank = 0; rank < 2; rank++)
            {
                // rankEnabled
                PRINT_EXPECT_CALL(__wrap_mmio_write8, addr, 1);
                expect_any_count(__wrap_mmio_write8, addr, 1);
                expect_any_count(__wrap_mmio_write8, data, 1);

                // subchannelList
                for (int subchannel = 0; subchannel < MAX_SUBCHANNEL; subchannel++)
                {
                    PRINT_EXPECT_CALL(__wrap_mmio_write8, addr, 10);
                    expect_any_count(__wrap_mmio_write8, addr, 10);
                    expect_any_count(__wrap_mmio_write8, data, 10);
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

    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    // Clearing memory
    expect_function_calls(__wrap_mmio_write32, (SMBIOS_HANDOFF_RESERVATION_SIZE) / 4);

    // Write Type16 table to memory
    expect_any_count(__wrap_mmio_write8, addr, sizeof(SMBIOS_PHYS_MEM_ARRAY_16));
    expect_any_count(__wrap_mmio_write8, data, sizeof(SMBIOS_PHYS_MEM_ARRAY_16));

    // Write double-null terminator
    expect_any_count(__wrap_mmio_write8, addr, 2);
    expect_any_count(__wrap_mmio_write8, data, 2);

    will_return_always(__wrap_idhw_is_single_die_boot_en, false);

    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
        will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

        // Write Type17 table to memory
        expect_any_count(__wrap_mmio_write8, addr, sizeof(SMBIOS_MEM_DEVICE_17));
        expect_any_count(__wrap_mmio_write8, data, sizeof(SMBIOS_MEM_DEVICE_17));

        // Write strings... [DeviceLocator]
        const char* device_locator = "SLOT A";
        expect_any_count(__wrap_mmio_write8, addr, strlen(device_locator) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(device_locator) + 1);

        // Write strings... [BankLocator]
        const char* bank_locator = "BANK A";
        expect_any_count(__wrap_mmio_write8, addr, strlen(bank_locator) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(bank_locator) + 1);

        expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
        will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

        // Write strings... [Manufacturer]
        const char* manufacturer = "MICRON";
        will_return(__wrap_dimm_vendor_from_id, manufacturer);
        expect_any_count(__wrap_mmio_write8, addr, strlen(manufacturer) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(manufacturer) + 1);

        // Write strings... [SerialNumber]
        const char* serial_number = "12345678";
        will_return(__wrap_get_dimm_serial_number_string, serial_number);
        expect_any_count(__wrap_mmio_write8, addr, strlen(serial_number) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(serial_number) + 1);

        // Write strings... [AssetTag]
        const char* asset_tag = "N/A";
        expect_any_count(__wrap_mmio_write8, addr, strlen(asset_tag) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(asset_tag) + 1);

        // Write strings... [PartNumber]
        const char* part_number = "MT8JTF25664HZ-1G4F1";
        will_return(__wrap_get_dimm_part_number_string, part_number);
        expect_any_count(__wrap_mmio_write8, addr, strlen(part_number) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(part_number) + 1);

        // Write strings.. [PHY FW Version]
        expect_any_count(__wrap_mmio_write8, addr, ((sizeof(unsigned long)) * 2) + strlen("0x") + 1);
        expect_any_count(__wrap_mmio_write8, data, ((sizeof(unsigned long)) * 2) + strlen("0x") + 1);

        // Additional NULL byte between type17 tables
        expect_any_count(__wrap_mmio_write8, addr, 1);
        expect_any_count(__wrap_mmio_write8, data, 1);
    }

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    ddr_create_smbios_tables();
}

TEST_FUNCTION(ddr_create_smbios_tables_test_die_1, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_1);

    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    will_return_always(__wrap_idhw_is_single_die_boot_en, false);

    expect_value(__wrap_mscp_exp_spi_synchronize_dies, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    // Calculating offsets for DIE_1 to begin hits mocked functions that need values..
    will_return(__wrap_dimm_vendor_from_id, "MICRON");
    will_return(__wrap_get_dimm_serial_number_string, "12345678");
    will_return(__wrap_get_dimm_part_number_string, "MT8JTF25664HZ-1G4F1");

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
        will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

        // Write Type17 table to memory
        expect_any_count(__wrap_mmio_write8, addr, sizeof(SMBIOS_MEM_DEVICE_17));
        expect_any_count(__wrap_mmio_write8, data, sizeof(SMBIOS_MEM_DEVICE_17));

        // Write strings... [DeviceLocator]
        const char* device_locator = "SLOT A";
        expect_any_count(__wrap_mmio_write8, addr, strlen(device_locator) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(device_locator) + 1);

        // Write strings... [BankLocator]
        const char* bank_locator = "BANK A";
        expect_any_count(__wrap_mmio_write8, addr, strlen(bank_locator) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(bank_locator) + 1);

        expect_function_call(__wrap_ddr_i3c_interface_read_spd_nvm_data);
        will_return(__wrap_ddr_i3c_interface_read_spd_nvm_data, SILIBS_SUCCESS);

        // Write strings... [Manufacturer]
        const char* manufacturer = "MICRON";
        will_return(__wrap_dimm_vendor_from_id, manufacturer);
        expect_any_count(__wrap_mmio_write8, addr, strlen(manufacturer) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(manufacturer) + 1);

        // Write strings... [SerialNumber]
        const char* serial_number = "12345678";
        will_return(__wrap_get_dimm_serial_number_string, serial_number);
        expect_any_count(__wrap_mmio_write8, addr, strlen(serial_number) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(serial_number) + 1);

        // Write strings... [AssetTag]
        const char* asset_tag = "N/A";
        expect_any_count(__wrap_mmio_write8, addr, strlen(asset_tag) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(asset_tag) + 1);

        // Write strings... [PartNumber]
        const char* part_number = "MT8JTF25664HZ-1G4F1";
        will_return(__wrap_get_dimm_part_number_string, part_number);
        expect_any_count(__wrap_mmio_write8, addr, strlen(part_number) + 1);
        expect_any_count(__wrap_mmio_write8, data, strlen(part_number) + 1);

        // Write strings.. [PHY FW Version]
        expect_any_count(__wrap_mmio_write8, addr, ((sizeof(unsigned long)) * 2) + strlen("0x") + 1);
        expect_any_count(__wrap_mmio_write8, data, ((sizeof(unsigned long)) * 2) + strlen("0x") + 1);

        // Additional NULL byte between type17 tables
        expect_any_count(__wrap_mmio_write8, addr, 1);
        expect_any_count(__wrap_mmio_write8, data, 1);
    }

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    ddr_create_smbios_tables();
}
