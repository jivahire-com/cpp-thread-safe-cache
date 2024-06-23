//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_sds_init.cpp
 * Tests the init of the sds service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <FpFwUtils.h>
#include <fpfw_init.h>
#include <sds_api.h>
#include <sds_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
// extern int32_t sds_read;
// extern int32_t sds_write;
static sds_api_type_t currentType = SDS_IO_REQUEST_READ_SYNC;

/*------------- Functions ----------------*/

//
// Mocks
//
int32_t __wrap_DfwkInterfaceSendSync(PDFWK_INTERFACE_HEADER Interface, PDFWK_SYNC_REQUEST_HEADER Request)
{
    FPFW_UNUSED(Interface);
    assert_true(Request->RequestType == currentType);
    return mock_type(int32_t);
}
}
//
// Tests
//

TEST_FUNCTION(test_sds_read, nullptr, nullptr)
{
    // Set up expectations
    sds_service_interface_t sds_test_interface = {};
    uint8_t buffer[10];
    size_t buffer_size = sizeof(buffer);
    currentType = SDS_IO_REQUEST_READ_SYNC;

    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    // Call the function under test
    int32_t result = sds_read(&sds_test_interface.header, buffer, buffer_size, nullptr);

    // Perform necessary assertions on result
    assert_true(result == DFWK_SUCCESS);
}

TEST_FUNCTION(test_sds_write, nullptr, nullptr)
{
    // Set up expectations
    sds_service_interface_t sds_test_interface = {};
    uint8_t buffer[10];
    size_t buffer_size = sizeof(buffer);
    currentType = SDS_IO_REQUEST_WRITE_SYNC;

    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    // Call the function under test
    int32_t result = sds_write(&sds_test_interface.header, buffer, buffer_size, nullptr);

    // Perform necessary assertions on result
    assert_true(result == DFWK_SUCCESS);
}
