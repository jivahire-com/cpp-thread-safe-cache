//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_sensor_fifo_driver.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include "DfwkClient.h"  // for PDFWK_SYNC_REQUEST_HEADER
#include "fpfw_status.h" // for fpfw_status_t, FPFW_STAT...

#include <FpFwUtils.h>                      // for FPFW_UNUSED
#include <scf_mhu_device.h>                 // for SCF_MHU_FIFO_MAX_ID, scf...
#include <sensor_fifo_driver_interface.h>   // for sensor_fifo_driver_inf_r...
#include <sensor_fifo_driver_interface_i.h> // for scf_mhu_request_dispatch...
#include <stddef.h>                         // for NULL
#include <stdint.h>                         // for uint32_t
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
sensor_fifo_device_properties_t s_test_fifo_properties[SCF_MHU_FIFO_MAX_ID];
/*------------- Functions ----------------*/
extern "C" {
fpfw_status_t test_dispatch_sync_function(PDFWK_SYNC_REQUEST_HEADER request);
fpfw_status_t test_dispatch_sync_function(PDFWK_SYNC_REQUEST_HEADER request)
{
    FPFW_UNUSED(request);
    return FPFW_STATUS_SUCCESS;
}
}
//
// Tests
//
TEST_FUNCTION(test_mhu_device_init, nullptr, nullptr)
{
    scf_mhu_device_t scf_mhu_device;
    DFWK_SCHEDULE schedule;
    scf_mhu_device_config_t config;

    expect_value(__wrap_FpFwAssert, expression, true);
    expect_value(__wrap_DfwkDeviceInitialize, Device, &scf_mhu_device);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &schedule);

    scf_mhu_device_initialize(&scf_mhu_device, &schedule, s_test_fifo_properties, SCF_MHU_FIFO_MAX_ID, &config);

    assert_true(scf_mhu_device.sensor_fifo_device.initialized == true);
    assert_true(scf_mhu_device.sensor_fifo_device.fifo_property_table == s_test_fifo_properties);
}

TEST_FUNCTION(test_request_dispatch_sync_read, nullptr, nullptr)
{
    sensor_fifo_drv_inf_read_reg_request read_request;
    read_request.input.address = 0xAABBCCDD;
    read_request.header.RequestType = SENSOR_FIFO_SYNC_REQUEST_READ_REG;

    expect_value(__wrap_mmio_read32, addr, 0xAABBCCDD);
    will_return(__wrap_mmio_read32, 0x712534);

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&read_request);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(read_request.output.value, 0x712534);
}

TEST_FUNCTION(test_request_dispatch_sync_write, nullptr, nullptr)
{
    sensor_fifo_drv_inf_write_reg_request write_request;
    write_request.input.address = 0x1234;
    write_request.input.value = 0x72;
    write_request.header.RequestType = SENSOR_FIFO_SYNC_REQUEST_WRITE_REG;

    expect_value(__wrap_mmio_write32, addr, 0x1234);
    expect_value(__wrap_mmio_write32, data, 0x72);

    fpfw_status_t status = scf_mhu_request_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&write_request);

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

    assert_int_equal(status, FPFW_STATUS_FAIL);
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
