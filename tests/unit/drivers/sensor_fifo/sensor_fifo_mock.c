//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_mock.c
 * Provide mock functions for sensor fifo driver tests
 */

/*------------- Includes -----------------*/

#include <DfwkClient.h> // for PDFWK_DEVICE_HEADER, PDFWK_INTERFACE_HEADER
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <stdint.h>     // for uint32_t, uint64_t, int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

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
    check_expected_ptr(Interface);
    check_expected_ptr(Request);
    return mock_type(int32_t);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected_ptr(addr);
    return mock_type(uint32_t);
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    check_expected_ptr(addr);
    check_expected(data);
}

void __wrap_mmio_update32(volatile uint32_t* addr, uint32_t data, uint32_t mask)
{
    check_expected_ptr(addr);
    check_expected(data);
    check_expected(mask);
}

uint64_t __wrap_mmio_read64(volatile uint64_t* addr)
{
    check_expected_ptr(addr);
    return mock_type(uint64_t);
}

void __wrap_mmio_write64(volatile uint64_t* addr, uint64_t data)
{
    check_expected_ptr(addr);
    check_expected(data);
}
