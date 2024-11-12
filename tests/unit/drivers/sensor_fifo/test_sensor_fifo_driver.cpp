//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_sensor_fifo_driver.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:sensor_fifo

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {

#include "device_fifo_id.h" // for DEVICE_FIFO_MAX_ID
#include "hw_fifo.h"        // for hw_fifo_enable, hw_fifo_...
#include "sensor_fifo_mock.h"

#include <DfwkClient.h>                     // for PDFWK_SYNC_REQUEST_HEADER
#include <FpFwUtils.h>                      // for FPFW_UNUSED
#include <fpfw_status.h>                    // for fpfw_status_t, FPFW_STAT...
#include <scf_mhu_device.h>                 // for DEVICE_FIFO_MAX_ID, scf...
#include <sensor_fifo_driver_interface.h>   // for sensor_fifo_driver_inf_r...
#include <sensor_fifo_driver_interface_i.h> // for scf_mhu_request_dispatch...
#include <sensor_ram_bridge.h>
#include <stddef.h>                  // for NULL
#include <stdint.h>                  // for uint32_t
#include <string.h>                  // for memset
#include <telemetry_config_struct.h> // for scf_base_config
#include <vcruntime_string.h>        // for memcmp
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
#define SCF_EXP_CSR_BASE_ADDR (0x3000)
#define SCF_MHU_BASE_ADDR     (0x4000)

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

scf_mhu_device_t scf_mhu_device;
DFWK_SCHEDULE schedule;
scf_mhu_device_config_t config;

extern "C" {
extern pscf_mhu_device_config_t sp_scf_mhu_device_cfg;
extern pscf_mhu_device_t sp_scf_mhu_device;
}

/*------------- Functions ----------------*/
extern "C" {
fpfw_status_t test_dispatch_sync_function(PDFWK_SYNC_REQUEST_HEADER request);
fpfw_status_t test_dispatch_sync_function(PDFWK_SYNC_REQUEST_HEADER request)
{
    FPFW_UNUSED(request);
    return FPFW_STATUS_SUCCESS;
}
}

static uint8_t mhu_placeholder[1000];
static uint8_t csr_placeholder[1000];

static int fw_fifo_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    snsr_fifo_mock_use_real_mmio = true;

    initialize_mock_fifos();

    static scf_base_config scf_config;
    scf_config.scf_mhu_base_address = (uintptr_t)mhu_placeholder;
    scf_config.scf_ram_base_address = (uintptr_t)fifo_mem.pstate_fifo;
    scf_config.scf_ram_buffer_size = sizeof(fifo_mem);
    scf_set_working_config((uintptr_t)&scf_config);

    static scf_mhu_device_config_t scf_mhu_device_cfg = {.scf_mhu_base_address = (uintptr_t)mhu_placeholder,
                                                         .scf_exp_csr_base_address = (uintptr_t)csr_placeholder,
                                                         .scf_ram_base_address = (uintptr_t)fifo_mem.pstate_fifo,
                                                         .scf_ram_buffer_size = sizeof(fifo_mem),
                                                         .is_scp = true};

    sp_scf_mhu_device_cfg = &scf_mhu_device_cfg;

    return 0;
}

static int fw_fifo_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    snsr_fifo_mock_use_real_mmio = false;

    return 0;
}

//
// Tests
//

TEST_FUNCTION(test_scf_device_init, nullptr, nullptr)
{
    snsr_fifo_mock_check_mmio_inputs = false;
    snsr_fifo_mock_use_real_mmio = false;

    config.is_scp = false;
    config.scf_exp_csr_base_address = SCF_EXP_CSR_BASE_ADDR;
    config.scf_mhu_base_address = SCF_MHU_BASE_ADDR;

    will_return_count(__wrap_mmio_read32, 0xFFFFFFFF, -1); // value and number is not pertinent for this
    expect_value(__wrap_FpFwAssert, expression, true);
    expect_value(__wrap_DfwkDeviceInitialize, Device, &scf_mhu_device);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &schedule);

    scf_mhu_device_initialize(&scf_mhu_device, &schedule, test_fifo_properties, DEVICE_FIFO_MAX_ID, &config);

    assert_true(scf_mhu_device.sensor_fifo_device.initialized == true);
    assert_true(scf_mhu_device.sensor_fifo_device.fifo_property_table == test_fifo_properties);

    config.is_scp = true;
    expect_value(__wrap_FpFwAssert, expression, true);
    expect_value(__wrap_DfwkDeviceInitialize, Device, &scf_mhu_device);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &schedule);
    expect_value(__wrap_scf_trigger_stop, scp_exp_csr_base_address, SCF_EXP_CSR_BASE_ADDR);
    expect_function_call(__wrap_init_scf_mhu);

    scf_mhu_device_initialize(&scf_mhu_device, &schedule, test_fifo_properties, DEVICE_FIFO_MAX_ID, &config);

    snsr_fifo_mock_check_mmio_inputs = true;
    snsr_fifo_mock_use_real_mmio = false;
}

TEST_FUNCTION(test_request_dispatch_read_reg, nullptr, nullptr)
{
    snsr_fifo_mock_check_mmio_inputs = true;
    snsr_fifo_mock_use_real_mmio = false;

    sensor_fifo_drv_inf_read_reg_request read_request;
    read_request.input.address = 0xAABBCCDD;
    read_request.header.RequestType = SENSOR_FIFO_SYNC_REQUEST_READ_REG;

    expect_value(__wrap_mmio_read32, addr, 0xAABBCCDD);
    will_return(__wrap_mmio_read32, 0x712534);

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&read_request);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(read_request.output.value, 0x712534);
}

TEST_FUNCTION(test_request_dispatch_write_reg, nullptr, nullptr)
{
    snsr_fifo_mock_check_mmio_inputs = true;
    snsr_fifo_mock_use_real_mmio = false;

    sensor_fifo_drv_inf_write_reg_request write_request;
    write_request.input.address = 0x1234;
    write_request.input.value = 0x72;
    write_request.header.RequestType = SENSOR_FIFO_SYNC_REQUEST_WRITE_REG;

    expect_value(__wrap_mmio_write32, addr, 0x1234);
    expect_value(__wrap_mmio_write32, data, 0x72);

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&write_request);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_request_dispatch_global_hw_enable, fw_fifo_setup, fw_fifo_teardown)
{
    snsr_fifo_mock_check_mmio_inputs = false;
    snsr_fifo_mock_use_real_mmio = false;

    sensor_fifo_drv_inf_global_enable global_enable_req;

    global_enable_req.header.RequestType = SENSOR_FIFO_SYNC_SET_GLOBAL_HW_ENABLE;
    global_enable_req.input.enable = true;

    expect_value(__wrap_scf_trigger_start, scp_exp_csr_base_address, csr_placeholder);
    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&global_enable_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    global_enable_req.input.enable = false;

    expect_value(__wrap_scf_trigger_stop, scp_exp_csr_base_address, csr_placeholder);
    status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&global_enable_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_request_dispatch_write_read_entry, fw_fifo_setup, fw_fifo_teardown)
{
#define ENTRY_VALUE (0x48)
    uint8_t src_data[PVT_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};

    memset(src_data, ENTRY_VALUE, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
    memset(fifo_mem.pvt_temp_fifo, 0x00, sizeof(fifo_mem.pvt_temp_fifo));

    pvt_temp_fifo_read_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
    pvt_temp_fifo_write_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);

    assert_true(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    sensor_fifo_drv_inf_write_entry write_entry_req;
    write_entry_req.input.fifo_id = DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD;
    write_entry_req.input.src_data = src_data;
    write_entry_req.input.entry_size = PVT_TEMP_FIFO_ENTRY_SIZE_BYTES;
    write_entry_req.input.num_entries = 1;
    write_entry_req.input.stride_index = 0;
    write_entry_req.header.RequestType = SENSOR_FIFO_SYNC_WRITE_ENTRY;

    hw_fifo_enable(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD);
    expect_value_count(__wrap_FpFwAssert, expression, true, 5);

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&write_entry_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    assert_false(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    // now read it back

    uint8_t dest_data[PVT_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};
    memset(dest_data, 0, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
    uint16_t num_entries_read;
    uint16_t num_entries_remaining;
    uint16_t stride_index;

    sensor_fifo_drv_inf_read_entry read_entry_req;
    read_entry_req.input.fifo_id = DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD;
    read_entry_req.input.entry_size = PVT_TEMP_FIFO_ENTRY_SIZE_BYTES;

    read_entry_req.output.dest_data = dest_data;
    read_entry_req.output.num_entries_read = &num_entries_read;
    read_entry_req.output.num_entries_remaining = &num_entries_remaining;
    read_entry_req.output.stride_index = &stride_index;

    read_entry_req.header.RequestType = SENSOR_FIFO_SYNC_READ_ENTRY;

    expect_value_count(__wrap_FpFwAssert, expression, true, 4);

    status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&read_entry_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    assert_true(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    assert_int_equal(memcmp(dest_data, src_data, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES), 0);
}

TEST_FUNCTION(test_request_dispatch_set_fifo_enable, fw_fifo_setup, fw_fifo_teardown)
{
    sensor_fifo_drv_inf_fifo_enable fifo_enable_req;

    fifo_enable_req.header.RequestType = SENSOR_FIFO_SYNC_SET_FIFO_ENABLE;
    fifo_enable_req.input.fifo_id = DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD;
    fifo_enable_req.input.enable = true;

    assert_false(hw_fifo_is_enabled(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    expect_value_count(__wrap_FpFwAssert, expression, true, 1);

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&fifo_enable_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    assert_true(hw_fifo_is_enabled(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));
    fifo_enable_req.input.enable = false;

    expect_value_count(__wrap_FpFwAssert, expression, true, 1);

    status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&fifo_enable_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_false(hw_fifo_is_enabled(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));
}

TEST_FUNCTION(test_request_dispatch_update_write_ptr, fw_fifo_setup, fw_fifo_teardown)
{
    sensor_fifo_drv_inf_update_write_stride update_stride_req;

    update_stride_req.header.RequestType = SENSOR_FIFO_SYNC_UPDATE_WRITE_PTR;
    update_stride_req.input.fifo_id = DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD;

    pvt_temp_fifo_write_reg = (uint32_t)(fifo_mem.pvt_temp_fifo);

    expect_value_count(__wrap_FpFwAssert, expression, true, 1);

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&update_stride_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    assert_int_equal(pvt_temp_fifo_write_reg, (uint32_t)(fifo_mem.pvt_temp_fifo + PVT_TEMP_FIFO_STRIDE_SIZE_BYTES));
}

TEST_FUNCTION(test_request_dispatch_query_enabled, fw_fifo_setup, fw_fifo_teardown)
{
    bool is_enabled[DEVICE_FIFO_MAX_ID] = {0};

    for (uint32_t id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; id < DEVICE_FIFO_MAX_ID; id++)
    {
        hw_fifo_disable((DEVICE_FIFO_ID)id);
    }

    sensor_fifo_drv_inf_fifo_is_enabled is_enabled_req;
    is_enabled_req.header.RequestType = SENSOR_FIFO_SYNC_QUERY_IS_ENABLED;
    is_enabled_req.output.is_enabled = &is_enabled;

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&is_enabled_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    for (uint32_t id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; id < DEVICE_FIFO_MAX_ID; id++)
    {
        assert_false(is_enabled[id]);
    }

    for (uint32_t id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; id < DEVICE_FIFO_MAX_ID; id++)
    {
        hw_fifo_enable((DEVICE_FIFO_ID)id);
    }

    status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&is_enabled_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    for (uint32_t id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; id < DEVICE_FIFO_MAX_ID; id++)
    {
        assert_true(is_enabled[id]);
    }
}

TEST_FUNCTION(test_request_dispatch_query_empty, fw_fifo_setup, fw_fifo_teardown)
{
    bool is_empty[DEVICE_FIFO_MAX_ID] = {0};

    for (uint32_t id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; id < DEVICE_FIFO_MAX_ID; id++)
    {
        hw_fifo_disable((DEVICE_FIFO_ID)id);
    }

    sensor_fifo_drv_inf_fifo_is_empty is_empty_req;
    is_empty_req.header.RequestType = SENSOR_FIFO_SYNC_QUERY_IS_EMPTY;
    is_empty_req.output.is_empty = &is_empty;

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&is_empty_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    for (uint32_t id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; id < DEVICE_FIFO_MAX_ID; id++)
    {
        assert_true(is_empty[id]);
        hw_fifo_enable((DEVICE_FIFO_ID)id); // enable for next test case below
    }

    // pointers not being equal will indicate not empty
    pstate_fifo_write_reg = pstate_fifo_read_reg + 8;
    msg_fifo_write_reg = msg_fifo_read_reg + 8;
    tile_temp_fifo_write_reg = tile_temp_fifo_read_reg + 8;
    tile_volt_fifo_write_reg = tile_volt_fifo_read_reg + 8;
    core_current_fifo_write_reg = core_current_fifo_read_reg + 8;
    pvt_temp_fifo_write_reg = pvt_temp_fifo_read_reg + 8;
    pvt_volt_fifo_write_reg = pvt_volt_fifo_read_reg + 8;
    dimm_temp_fifo_write_reg = dimm_temp_fifo_read_reg + 8;
    vr_temp_fifo_write_reg = vr_temp_fifo_read_reg + 8;
    vr_curr_fifo_write_reg = vr_curr_fifo_read_reg + 8;

    is_empty_req.output.is_empty = &is_empty;
    status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&is_empty_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    for (uint32_t id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; id < DEVICE_FIFO_MAX_ID; id++)
    {
        assert_false(is_empty[id]);
    }
}

TEST_FUNCTION(test_request_dispatch_reset, fw_fifo_setup, fw_fifo_teardown)
{
    snsr_fifo_mock_check_mmio_inputs = false;
    snsr_fifo_mock_use_real_mmio = false;

    DFWK_SYNC_REQUEST_HEADER reset_req;
    reset_req.RequestType = SENSOR_FIFO_SYNC_RESET;

    will_return_count(__wrap_mmio_read32, 0x0, -1); // value and number is not pertinent for this
    expect_value(__wrap_scf_trigger_stop, scp_exp_csr_base_address, csr_placeholder);
    expect_function_call(__wrap_init_scf_mhu);

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&reset_req);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_request_dispatch_sync_bad, nullptr, nullptr)
{
    sensor_fifo_drv_inf_write_reg_request write_request;
    write_request.input.address = 0x1234;
    write_request.input.value = 0x72;
    write_request.header.RequestType = 200;

    expect_value(__wrap_FpFwAssert, expression, false);
    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&write_request);

    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_init, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    sensor_fifo_device_t device;

    device.dispatch_sync = test_dispatch_sync_function;
    device.initialized = true;

    expect_value(__wrap_FpFwAssert, expression, true);
    expect_value(__wrap_FpFwAssert, expression, true);
    expect_value(__wrap_FpFwAssert, expression, true);

    expect_value(__wrap_DfwkInterfaceInitialize, Interface, &driver_inf);
    expect_value(__wrap_DfwkInterfaceInitialize, Device, &device);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchQueue, NULL);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchSync, test_dispatch_sync_function);

    sensor_fifo_driver_inf_init(&driver_inf, &device);
    assert_true(driver_inf.device = &device);
}

TEST_FUNCTION(test_driver_inf_read_reg, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    uint32_t value = 0;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status = sensor_fifo_driver_inf_read_reg(&driver_inf, 0x5566, &value);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_driver_inf_read_reg_fail, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    uint32_t value = 0;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    fpfw_status_t status = sensor_fifo_driver_inf_read_reg(&driver_inf, 0x3232, &value);
    assert_int_equal(status, FPFW_STATUS_FAIL);

    status = sensor_fifo_driver_inf_read_reg(NULL, 0x3232, &value);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);

    status = sensor_fifo_driver_inf_read_reg(&driver_inf, 0x3232, NULL);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_write_reg, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    uint32_t value = 0;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status = sensor_fifo_driver_inf_write_reg(&driver_inf, 0x5566, value);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_driver_inf_write_reg_fail, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    uint32_t value = 0;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    fpfw_status_t status = sensor_fifo_driver_inf_write_reg(&driver_inf, 0x3232, value);
    assert_int_equal(status, FPFW_STATUS_FAIL);

    status = sensor_fifo_driver_inf_write_reg(NULL, 0x3232, value);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_global_hw_enable, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status = sensor_fifo_driver_inf_set_global_hw_enable(&driver_inf, false);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_driver_inf_global_hw_enable_fail, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    fpfw_status_t status = sensor_fifo_driver_inf_set_global_hw_enable(&driver_inf, true);
    assert_int_equal(status, FPFW_STATUS_FAIL);

    status = sensor_fifo_driver_inf_set_global_hw_enable(NULL, true);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_set_fifo_enable, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status =
        sensor_fifo_driver_inf_set_fifo_enable(&driver_inf, DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD, false);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_driver_inf_set_fifo_enable_fail, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    fpfw_status_t status =
        sensor_fifo_driver_inf_set_fifo_enable(&driver_inf, DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD, false);
    assert_int_equal(status, FPFW_STATUS_FAIL);

    status = sensor_fifo_driver_inf_set_fifo_enable(NULL, DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD, true);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_update_write_stride, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status = sensor_fifo_driver_inf_update_write_stride(&driver_inf, DEVICE_FIFO_PSTATE_TLM_HW_PROD);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_driver_inf_update_write_stride_fail, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    fpfw_status_t status = sensor_fifo_driver_inf_update_write_stride(&driver_inf, DEVICE_FIFO_PSTATE_TLM_HW_PROD);
    assert_int_equal(status, FPFW_STATUS_FAIL);

    status = sensor_fifo_driver_inf_update_write_stride(NULL, DEVICE_FIFO_PSTATE_TLM_HW_PROD);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_is_enabled, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    bool is_enabled[DEVICE_FIFO_MAX_ID];

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status = sensor_fifo_driver_inf_is_enabled(&driver_inf, &is_enabled);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_driver_inf_is_enabled_fail, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    bool is_enabled[DEVICE_FIFO_MAX_ID];

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    fpfw_status_t status = sensor_fifo_driver_inf_is_enabled(&driver_inf, &is_enabled);
    assert_int_equal(status, FPFW_STATUS_FAIL);

    status = sensor_fifo_driver_inf_is_enabled(NULL, &is_enabled);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_is_empty, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    bool is_empty[DEVICE_FIFO_MAX_ID];

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status = sensor_fifo_driver_inf_is_empty(&driver_inf, &is_empty);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_driver_inf_is_empty_fail, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    bool is_empty[DEVICE_FIFO_MAX_ID];

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    fpfw_status_t status = sensor_fifo_driver_inf_is_empty(&driver_inf, &is_empty);
    assert_int_equal(status, FPFW_STATUS_FAIL);

    status = sensor_fifo_driver_inf_is_empty(NULL, &is_empty);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_write_entries, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;

    uint8_t src_data[100];

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status =
        sensor_fifo_driver_write_entries(&driver_inf, DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD, src_data, 10, 1, 0);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_driver_inf_write_entries_fail, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    uint8_t src_data[100];

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    fpfw_status_t status =
        sensor_fifo_driver_write_entries(&driver_inf, DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD, src_data, 10, 1, 0);

    assert_int_equal(status, FPFW_STATUS_FAIL);

    status = sensor_fifo_driver_write_entries(NULL, DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD, src_data, 10, 1, 0);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_read_entries, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;

    uint8_t dest_data[100];
    uint16_t num_entries_read;
    uint16_t num_entries_remaining;
    uint16_t stride_index;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status = sensor_fifo_driver_read_entry(&driver_inf,
                                                         DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD,
                                                         10,
                                                         dest_data,
                                                         &num_entries_read,
                                                         &num_entries_remaining,
                                                         &stride_index);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_driver_inf_read_entries_fail, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;
    uint8_t dest_data[100];
    uint16_t num_entries_read;
    uint16_t num_entries_remaining;
    uint16_t stride_index;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    fpfw_status_t status = sensor_fifo_driver_read_entry(&driver_inf,
                                                         DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD,
                                                         10,
                                                         dest_data,
                                                         &num_entries_read,
                                                         &num_entries_remaining,
                                                         &stride_index);

    assert_int_equal(status, FPFW_STATUS_FAIL);

    status =
        sensor_fifo_driver_read_entry(NULL, DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD, 10, dest_data, &num_entries_read, &num_entries_remaining, &stride_index);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

TEST_FUNCTION(test_driver_inf_reset, nullptr, nullptr)
{
    sensor_fifo_driver_interface_t driver_inf;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &driver_inf);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    fpfw_status_t status = sensor_fifo_driver_reset(&driver_inf);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}
