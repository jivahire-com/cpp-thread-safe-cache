//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_variable_services.cpp
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, check_expected, CmockaWra...

extern "C" {
#include <FpFwUtils.h>            // for FPFW_UNUSED
#include <fpfw_icc_base.h>             // for fpfw_icc_base_ctx_t, fpfw_icc_ba...
#include <hsp_firmware_headers.h> // for kng_hsp_mailbox_msg, HSP_MAILBOX_RSP...
#include <stdint.h>               // for int32_t, uint32_t
#include <variable_services.h>    // for var_service_shared_mem_t, var_serv...
#include <variable_services_helper.h> // for variable_services_sync_get_vari...

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_GUID {0x3363AE8A, 0xDAB5, 0x4DCA, {0xBF, 0x32, 0xDD, 0x0E, 0x65, 0x89, 0x95, 0xC5}}

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t test_rsp_data[] = {0x11, 0x22, 0x33, 0x44, 0x55};
static uint8_t rmss_shared_ram_region[128] = {0};
static var_service_shared_mem_t shared_mem = {
    .payload_base = (uintptr_t)rmss_shared_ram_region,
    .max_payload_size = sizeof(rmss_shared_ram_region),
};
static uint32_t dummy_icc_base_ctx = 0;

/*------------- Mock Functions ----------------*/
fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t *icc_ctx, void *payload_buffer, size_t buffer_size, size_t *output_recv_bytes)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(buffer_size);
    
    //! prepare the hsp mbox response
    kng_hsp_mailbox_msg* msg = (kng_hsp_mailbox_msg*)payload_buffer;
    if (msg->header.cmd == HSP_MAILBOX_CMD_GET_VARIABLE_REQ)
    {
        //! Update resp cmd code
        msg->header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_RSP;
        //! Simulate HSP copying get var response data to shared memory region
        uint32_t shared_mem_data_offset = sizeof(struct hsp_mbox_get_variable) + sizeof("TestGetVar");
        memcpy((void*)(rmss_shared_ram_region + shared_mem_data_offset), (void*)test_rsp_data, sizeof(test_rsp_data));
    }
    else if (msg->header.cmd == HSP_MAILBOX_CMD_SET_VARIABLE_REQ){
        //! Update resp cmd code
        msg->header.cmd = HSP_MAILBOX_CMD_SET_VARIABLE_RSP;
    }
    msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
    *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);

    return FPFW_ICC_BASE_STATUS_SUCCESS;
}

fpfw_icc_base_ctx_t* __wrap_get_icc_base_ctx(void)
{
    //! dummy icc base ctx for test
    return (fpfw_icc_base_ctx_t*)&dummy_icc_base_ctx;
}

bool __wrap_system_info_is_hsp_present()
{
    return true;
}

/*------------- Test Setup/Teardown ----------------*/
static int teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    //! Reset the shared memory region
    memset(rmss_shared_ram_region, 0, sizeof(rmss_shared_ram_region));
    return 0;
}

/*------------- Test Cases ----------------*/
TEST_FUNCTION(test_variable_services_sync_get_variable, nullptr, teardown)
{
    //! client buffer to store the response data
    uint8_t client_buffer[5] = {0};

    //! prepare the get variable request
    var_service_req_params_t get_var_req = {
        .variable_name_ptr = (uint16_t*)"TestGetVar",
        .variable_name_size = sizeof("TestGetVar"),
        .vendor_namespace_guid = TEST_GUID,
        .data_size = sizeof(client_buffer),
        .data = client_buffer,
    };

    //! Invoke the FUT
    variable_service_sync_get_variable(&shared_mem, &get_var_req);

    //! verify the client buffer is updated with the test response data
    assert_memory_equal(client_buffer, test_rsp_data, sizeof(client_buffer));
}

TEST_FUNCTION(test_variable_services_sync_set_variable, nullptr, teardown)
{
    //! client's data to be written to the shared memory region
    uint8_t client_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    //! prepare the set variable request
    var_service_req_params_t set_var_req = {
        .variable_name_ptr = (uint16_t*)"TestSetVar",
        .variable_name_size = sizeof("TestSetVar"),
        .vendor_namespace_guid = TEST_GUID,
        .data_size = sizeof(client_data),
        .data = client_data,
        .attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    };

    //! Invoke the FUT
    variable_service_sync_set_variable(&shared_mem, &set_var_req);

    //! verify the shared memory is updated with the client data
    uint32_t shared_mem_data_offset = sizeof(struct hsp_mbox_set_variable) + sizeof("TestSetVar");
    assert_memory_equal((void*)(rmss_shared_ram_region + shared_mem_data_offset), client_data, sizeof(client_data));
}

}