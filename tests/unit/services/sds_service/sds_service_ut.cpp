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
#include <fpfw_status.h>
#include <kng_error.h>
#include <sds_api.h>
#include <sds_init.h>
#include <stdlib.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SECURE_REGION_SIZE     0x100
#define NON_SECURE_REGION_SIZE 0x400

#define FIRST_TEST_MODULE_ID           1
#define FIRST_TEST_MODULE_POOL_SIZE    0x50
#define FIRST_TEST_MODULE_SDS_CONTENTS "FIRST"

#define SECOND_TEST_MODULE_ID           2
#define SECOND_TEST_MODULE_POOL_SIZE    0x60
#define SECOND_TEST_MODULE_SDS_CONTENTS "SECOND"

enum SDS_BLOCK_ID
{
    SECURE_REGION,
    NON_SECURE_REGION,
    END_OF_SDS_REGIONS
};

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern int32_t create_sds_block(sds_service_request_t* p_sds_request);
extern int32_t read_sds_block(sds_service_request_t* p_sds_request);
extern int32_t write_sds_block(sds_service_request_t* p_sds_request);

static unsigned char sds_region_data_non_secure[NON_SECURE_REGION_SIZE];
static unsigned char sds_region_data_secure[SECURE_REGION_SIZE];
static sds_region_desc_t regions[END_OF_SDS_REGIONS];
static sds_config_t sds_config;

/*------------- Functions ----------------*/
//
// Mocks
//
sds_config_t* __wrap_retrieve_sds_config_info()
{
    return mock_type(sds_config_t*);
}

int32_t __wrap_DfwkInterfaceSendSync(PDFWK_INTERFACE_HEADER Interface, PDFWK_SYNC_REQUEST_HEADER Request)
{
    FPFW_UNUSED(Interface);

    int32_t status = KNG_SUCCESS;
    psds_service_request_t p_sds_request = (psds_service_request_t)Request;

    switch (Request->RequestType)
    {
    case SDS_IO_REQUEST_CREATION_SYNC: {
        status = create_sds_block(p_sds_request);
        break;
    }
    case SDS_IO_REQUEST_READ_SYNC: {
        status = read_sds_block(p_sds_request);
        break;
    }
    case SDS_IO_REQUEST_WRITE_SYNC: {
        status = write_sds_block(p_sds_request);
        break;
    }
    }

    return status;
}

void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    FPFW_UNUSED(Device);
    FPFW_UNUSED(Schedule);
}

void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    FPFW_UNUSED(Queue);
    FPFW_UNUSED(Device);
    FPFW_UNUSED(DispatchRoutine);
    FPFW_UNUSED(DispatchContext);
    FPFW_UNUSED(QueueType);
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_FPFwSpinLockInitialize(size_t* pLock)
{
    FPFW_UNUSED(pLock);
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
    function_called();
}
}
//
// Tests
//

static int sds_region_setup(void** state)
{
    FPFW_UNUSED(state);

    memset(sds_region_data_secure, 0, sizeof(sds_region_data_secure));
    memset(sds_region_data_non_secure, 0, sizeof(sds_region_data_non_secure));

    regions[SECURE_REGION].base = &sds_region_data_secure;
    regions[SECURE_REGION].size = sizeof(sds_region_data_secure);
    regions[NON_SECURE_REGION].base = &sds_region_data_non_secure;
    regions[NON_SECURE_REGION].size = sizeof(sds_region_data_non_secure);

    sds_config.region_count = END_OF_SDS_REGIONS;
    sds_config.regions = regions;

    return 0;
}

static int sds_invalid_region_setup(void** state)
{
    FPFW_UNUSED(state);

    memset(sds_region_data_secure, 0, sizeof(sds_region_data_secure));
    memset(sds_region_data_non_secure, 0, sizeof(sds_region_data_non_secure));

    regions[SECURE_REGION].base = &sds_region_data_secure;
    regions[SECURE_REGION].size = 0;
    regions[NON_SECURE_REGION].base = &sds_region_data_non_secure;
    regions[NON_SECURE_REGION].size = 0;

    sds_config.region_count = END_OF_SDS_REGIONS;
    sds_config.regions = regions;

    return 0;
}

TEST_FUNCTION(test_query_sds_config_info, sds_region_setup, nullptr)
{
    // Expectation
    will_return(__wrap_retrieve_sds_config_info, &sds_config);

    // Do the test
    sds_config_t* returned = retrieve_sds_config_info();

    assert_true(returned->region_count == sds_config.region_count);
    assert_true(returned->regions[SECURE_REGION].base == sds_config.regions[SECURE_REGION].base);
    assert_true(returned->regions[SECURE_REGION].size == sds_config.regions[SECURE_REGION].size);
    assert_true(returned->regions[NON_SECURE_REGION].base == sds_config.regions[NON_SECURE_REGION].base);
    assert_true(returned->regions[NON_SECURE_REGION].size == sds_config.regions[NON_SECURE_REGION].size);
}

TEST_FUNCTION(test_sds_region_creation, sds_region_setup, nullptr)
{
    // Expectation
    sds_service_interface_t sdsInterface = {};
    will_return_count(__wrap_retrieve_sds_config_info, &sds_config, -2);
    will_return_count(__wrap_fpfw_init_get_handle, &sdsInterface, -2);

    // Do the test
    sds_init(nullptr, nullptr);

    // Check all memory region block created in SDS
    sds_region_header_t* sds_secure_region_start_addr = (sds_region_header_t*)&sds_region_data_secure;
    sds_region_header_t* sds_non_secure_region_start_addr = (sds_region_header_t*)&sds_region_data_non_secure;

    assert_true(sds_secure_region_start_addr->signature == REGION_SIGNATURE);
    assert_true(sds_secure_region_start_addr->region_size == SECURE_REGION_SIZE);
    assert_true(sds_secure_region_start_addr->block_count == 0);

    assert_true(sds_non_secure_region_start_addr->signature == REGION_SIGNATURE);
    assert_true(sds_non_secure_region_start_addr->region_size == NON_SECURE_REGION_SIZE);
    assert_true(sds_non_secure_region_start_addr->block_count == 0);
}

TEST_FUNCTION(test_sds_region_invalid_creation, sds_invalid_region_setup, nullptr)
{
    // Expectation
    sds_service_interface_t sdsInterface = {};
    will_return_count(__wrap_retrieve_sds_config_info, &sds_config, -2);
    will_return_count(__wrap_fpfw_init_get_handle, &sdsInterface, -2);
    expect_function_calls(__wrap_crash_dump_bug_check, END_OF_SDS_REGIONS);

    // Do the test
    sds_init(nullptr, nullptr);
}

TEST_FUNCTION(test_sds_new_block_creation_on_same_region, sds_region_setup, nullptr)
{
    // Expectation
    sds_service_interface_t sdsInterface = {};
    will_return_count(__wrap_retrieve_sds_config_info, &sds_config, -2);
    will_return_count(__wrap_fpfw_init_get_handle, &sdsInterface, -2);

    // Do the test
    sds_init(nullptr, nullptr);
    sds_interface_init(nullptr, &sdsInterface);

    sds_region_header_t* sds_secure_region_start_addr = (sds_region_header_t*)&sds_region_data_secure;
    uint16_t previous_on_secure = sds_secure_region_start_addr->block_count;

    // create two blocks on the same region
    sds_block_creation(FIRST_TEST_MODULE_ID, FIRST_TEST_MODULE_POOL_SIZE, SECURE_REGION);
    sds_block_creation(SECOND_TEST_MODULE_ID, SECOND_TEST_MODULE_POOL_SIZE, SECURE_REGION);

    // Check all memory region block created in SDS
    assert_true((sds_secure_region_start_addr->block_count == (previous_on_secure + 2)));
}

TEST_FUNCTION(test_sds_new_block_creation_on_different_region, sds_region_setup, nullptr)
{
    // Expectation
    sds_service_interface_t sdsInterface = {};
    will_return_count(__wrap_retrieve_sds_config_info, &sds_config, -2);
    will_return_count(__wrap_fpfw_init_get_handle, &sdsInterface, -2);

    // Do the test
    sds_init(nullptr, nullptr);
    sds_interface_init(nullptr, &sdsInterface);

    sds_region_header_t* sds_secure_region_start_addr = (sds_region_header_t*)&sds_region_data_secure;
    sds_region_header_t* sds_non_secure_region_start_addr = (sds_region_header_t*)&sds_region_data_non_secure;
    uint16_t previous_on_secure = sds_secure_region_start_addr->block_count;
    uint16_t previous_on_non_secure = sds_non_secure_region_start_addr->block_count;

    sds_block_creation(FIRST_TEST_MODULE_ID, FIRST_TEST_MODULE_POOL_SIZE, SECURE_REGION);
    sds_block_creation(SECOND_TEST_MODULE_ID, SECOND_TEST_MODULE_POOL_SIZE, NON_SECURE_REGION);

    // Check all memory region block created in SDS
    assert_true((sds_secure_region_start_addr->block_count == (previous_on_secure + 1)));
    assert_true((sds_non_secure_region_start_addr->block_count == (previous_on_non_secure + 1)));
}

TEST_FUNCTION(test_sds_duplicated_block_creation, sds_region_setup, nullptr)
{
    // Expectation
    sds_service_interface_t sdsInterface = {};
    will_return_count(__wrap_retrieve_sds_config_info, &sds_config, -2);
    will_return_count(__wrap_fpfw_init_get_handle, &sdsInterface, -2);

    // Do the test
    sds_init(nullptr, nullptr);
    sds_interface_init(nullptr, &sdsInterface);

    sds_block_creation(FIRST_TEST_MODULE_ID, FIRST_TEST_MODULE_POOL_SIZE, SECURE_REGION);
    sds_region_header_t* sds_secure_region_start_addr = (sds_region_header_t*)&sds_region_data_secure;
    uint16_t previous = sds_secure_region_start_addr->block_count;

    sds_block_creation(FIRST_TEST_MODULE_ID, FIRST_TEST_MODULE_POOL_SIZE, SECURE_REGION);

    // Check all memory region block created in SDS
    assert_true(sds_secure_region_start_addr->block_count == previous);
}

TEST_FUNCTION(test_sds_block_read_write, sds_region_setup, nullptr)
{
    // Expectation
    sds_service_interface_t sdsInterface = {};
    will_return_count(__wrap_retrieve_sds_config_info, &sds_config, -2);
    will_return_count(__wrap_fpfw_init_get_handle, &sdsInterface, -2);

    // Do the test
    sds_init(nullptr, nullptr);
    sds_interface_init(nullptr, &sdsInterface);

    // check with first sds region
    sds_block_creation(FIRST_TEST_MODULE_ID, FIRST_TEST_MODULE_POOL_SIZE, SECURE_REGION);

    char write_buffer[FIRST_TEST_MODULE_POOL_SIZE] = FIRST_TEST_MODULE_SDS_CONTENTS;
    assert_true(KNG_SUCCEEDED(sds_block_write(FIRST_TEST_MODULE_ID, write_buffer, sizeof(write_buffer))));

    char read_buffer[FIRST_TEST_MODULE_POOL_SIZE] = {0};
    assert_true(KNG_SUCCEEDED(sds_block_read(FIRST_TEST_MODULE_ID, read_buffer, sizeof(read_buffer))));

    assert_string_equal(write_buffer, read_buffer);

    // check with second sds region
    sds_block_creation(SECOND_TEST_MODULE_ID, SECOND_TEST_MODULE_POOL_SIZE, SECURE_REGION);

    strcpy_s(write_buffer, _countof(write_buffer), SECOND_TEST_MODULE_SDS_CONTENTS);
    strcpy_s(read_buffer, _countof(read_buffer), "");

    assert_true(KNG_SUCCEEDED(sds_block_write(SECOND_TEST_MODULE_ID, write_buffer, sizeof(write_buffer))));

    assert_true(KNG_SUCCEEDED(sds_block_read(SECOND_TEST_MODULE_ID, read_buffer, sizeof(read_buffer))));

    assert_string_equal(write_buffer, read_buffer);
}

TEST_FUNCTION(test_sds_block_write_overflow, sds_region_setup, nullptr)
{
    // Expectation
    sds_service_interface_t sdsInterface = {};
    will_return_count(__wrap_retrieve_sds_config_info, &sds_config, -2);
    will_return_count(__wrap_fpfw_init_get_handle, &sdsInterface, -2);

    // Do the test
    sds_init(nullptr, nullptr);
    sds_interface_init(nullptr, &sdsInterface);

    sds_block_creation(FIRST_TEST_MODULE_ID, FIRST_TEST_MODULE_POOL_SIZE, SECURE_REGION);

    char write_buffer[FIRST_TEST_MODULE_POOL_SIZE * 2] = "FAIL EXPECTED";
    assert_true(KNG_FAILED(sds_block_write(FIRST_TEST_MODULE_ID, write_buffer, sizeof(write_buffer))));
}

TEST_FUNCTION(test_sds_block_read_overflow, sds_region_setup, nullptr)
{
    // Expectation
    sds_service_interface_t sdsInterface = {};
    will_return_count(__wrap_retrieve_sds_config_info, &sds_config, -2);
    will_return_count(__wrap_fpfw_init_get_handle, &sdsInterface, -2);

    // Do the test
    sds_init(nullptr, nullptr);
    sds_interface_init(nullptr, &sdsInterface);

    sds_block_creation(FIRST_TEST_MODULE_ID, FIRST_TEST_MODULE_POOL_SIZE, SECURE_REGION);

    char write_buffer[FIRST_TEST_MODULE_POOL_SIZE] = "ACK";
    assert_true(KNG_SUCCEEDED(sds_block_write(FIRST_TEST_MODULE_ID, write_buffer, sizeof(write_buffer))));

    char read_buffer[FIRST_TEST_MODULE_POOL_SIZE * 2] = "FAIL EXPECTED";
    assert_true(KNG_FAILED(sds_block_read(FIRST_TEST_MODULE_ID, read_buffer, sizeof(read_buffer))));
}
