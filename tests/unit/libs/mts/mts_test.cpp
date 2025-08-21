//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file time_utils_test.cpp
 * Unit test file for time utils
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // for TEST_FUNCTION, assert_true
#include <cstddef>         // for NULL
#include <cstdio>
extern "C" {
#include <icc_platform_defines.h>
#include <message_transfer_service.h>
#include <mts_platform_definitions.h>
#include <mts_platform_specialization.h>
#include <transfer_relay_protocol.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/
uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section
/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/
fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    function_called();
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(payload_buffer);
    FPFW_UNUSED(buffer_size);
    return mock_type(fpfw_status_t);
}

bool __wrap_transfer_rly_should_send_trp_msg(p_trp_msg_t trp_msg, p_trp_endpoint_t trp_endpoint)
{
    function_called();
    FPFW_UNUSED(trp_msg);
    FPFW_UNUSED(trp_endpoint);
    return mock_type(bool);
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    function_called();
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
}

void __wrap_FPFwErrorRaise(uint32_t error, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    function_called();
    FPFW_UNUSED(error);
    FPFW_UNUSED(arg0);
    FPFW_UNUSED(arg1);
    FPFW_UNUSED(arg2);
    FPFW_UNUSED(arg3);
}

/*----------------------------- Global Functions ----------------------------*/

TEST_FUNCTION(test_get_transport_info, nullptr, nullptr)
{
    mts_platform_transport_info_t info = {0, 0, 0};
    mts_platform_get_transport_info(TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX, &info);
    assert_int_equal(info.header_size, sizeof(uint32_t));
    // assert_int_equal(info.icc_dcp_command, E_MBOX_MSG_DCP);
    // assert_int_equal(info.icc_trp_command, E_MBOX_MSG_TRP);
}

TEST_FUNCTION(test_get_transport_info_fail, nullptr, nullptr)
{
    mts_platform_transport_info_t info = {0, 0, 0};
    expect_function_call(__wrap_FPFwErrorRaise);
    mts_platform_get_transport_info(TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX + 1, &info);
}

TEST_FUNCTION(test_mts_platform_send_msg_via_transport_1, nullptr, nullptr)
{
    trp_msg_t trap_msg;
    trp_endpoint_t endpoint;
    endpoint.transport_type = TRP_TRANSPORT_TYPE_ICC_ARM_MHU;
    expect_function_call(__wrap_transfer_rly_should_send_trp_msg);
    will_return(__wrap_transfer_rly_should_send_trp_msg, false);

    mts_platform_send_msg_via_transport(&trap_msg, &endpoint);
}

TEST_FUNCTION(test_mts_platform_send_msg_via_transport_2, nullptr, nullptr)
{
    trp_msg_t trap_msg;
    trp_endpoint_t endpoint;
    trap_msg.hdr.payload_size = 0;
    endpoint.transport_type = TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX;
    expect_function_call(__wrap_transfer_rly_should_send_trp_msg);
    will_return(__wrap_transfer_rly_should_send_trp_msg, true);

    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);

    mts_platform_send_msg_via_transport(&trap_msg, &endpoint);
}

TEST_FUNCTION(test_mts_platform_send_msg_via_transport_3, nullptr, nullptr)
{
    trp_msg_t trap_msg;
    trp_endpoint_t endpoint;
    trap_msg.hdr.payload_size = 0;
    endpoint.transport_type = TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX;
    expect_function_call(__wrap_transfer_rly_should_send_trp_msg);
    will_return(__wrap_transfer_rly_should_send_trp_msg, true);

    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_FAIL);

    mts_platform_send_msg_via_transport(&trap_msg, &endpoint);
}

TEST_FUNCTION(test_mts_platform_id, nullptr, nullptr)
{
    assert_int_equal(MTS_PLATFORM_PRIMARY_DIE_ID, 0);
    assert_int_equal(MTS_PLATFORM_PRIMARY_CORE_ID, 3);
    assert_int_equal(MTS_PLATFORM_HOST_DIE_ID, 0);
    assert_int_equal(MTS_PLATFORM_HOST_CORE_ID, 1);
    assert_int_equal(MTS_DCP_PLATFORM_ID, 2);
}
}