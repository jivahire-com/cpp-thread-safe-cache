//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_mock.c
 * Provide mock functions for sensor fifo driver tests
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h> // for PDFWK_INTERFACE_HEADER
#include <FpFwCMocka.h> // for check_expected, check_expected_ptr, mock_type
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <device_fifo_id.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCEEDED, fpf...
#include <sensor_fifo_driver_interface.h>
#include <stddef.h> // for size_t
#include <stdint.h> // for int32_t
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

int32_t __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER Interface)
{
    check_expected_ptr(Interface);
    return mock_type(int32_t);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{

    check_expected_ptr(Interface);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchQueue);
    check_expected_ptr(DispatchSync);
}

// wrapper function that checks expected values for the two incoming parameters
void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    check_expected_ptr(Device);
    check_expected_ptr(Schedule);
}

int32_t __wrap_DfwkInterfaceSendSync(PDFWK_INTERFACE_HEADER Interface, PDFWK_SYNC_REQUEST_HEADER Request)
{
    assert_non_null(Interface);
    assert_non_null(Request);
    return mock_type(int32_t);
}

fpfw_status_t __wrap_sensor_fifo_driver_read_entry(sensor_fifo_driver_interface_t* driver_interface,
                                                   DEVICE_FIFO_ID fifo_id,
                                                   size_t entry_size,
                                                   uint8_t* dest_data,
                                                   uint16_t* num_entries_read,
                                                   uint16_t* num_entries_remaining,
                                                   uint16_t* stride_index)
{
    assert_non_null(driver_interface);
    assert_non_null(dest_data);
    assert_non_null(num_entries_read);
    assert_non_null(num_entries_remaining);
    assert_non_null(stride_index);
    FPFW_UNUSED(fifo_id);

    uint8_t* mocked_src = mock_type(uint8_t*);
    *num_entries_read = mock_type(uint16_t);
    if (*num_entries_read > 0)
    {
        memcpy_s(dest_data, entry_size, mocked_src, entry_size);
    }
    *num_entries_remaining = mock_type(uint16_t);
    *stride_index = mock_type(uint16_t);

    return mock_type(fpfw_status_t);
}
