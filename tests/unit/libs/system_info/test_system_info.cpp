//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_corebits.cpp
 * Corebits tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <fpfw_icc_base.h>        // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#define __NO_CSR_TYPEDEFS__
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    assert_non_null(icc_ctx);
    assert_non_null(payload_buffer);
    assert_true(buffer_size >= sizeof(kng_hsp_mailbox_msg));

    kng_hsp_mailbox_msg* output_message = (kng_hsp_mailbox_msg*)payload_buffer;
    output_message->header.cmd = mock_type(uint16_t);
    output_message->policy_status_rsp.policy_status.security_state = mock_type(uint8_t);

    *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);

    return FPFW_ICC_BASE_STATUS_SUCCESS;
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected(addr);
    return mock_type(uint32_t);
}

TEST_FUNCTION(test_get_board_id, nullptr, nullptr)
{
    // Set up expectations
    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;
    boot_meta_data.BoardId = 15;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    uint8_t board_id = system_info_get_board_id();

    // Verify expectations
    assert_true(board_id == 15);
    assert_true(BOARD_ID_GET_REWORK_REV(board_id) == 7);
    assert_true(BOARD_ID_GET_DESIGN_PHASE(board_id) == 1);
    assert_true(BOARD_ID_GET_SOC_POSITION(board_id) == 0);
}

TEST_FUNCTION(system_info_init_hsp_present, nullptr, nullptr)
{
    // Set up expectations
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);

    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;

    expect_value(__wrap_mmio_read32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_is_hsp_present() == true);
}

TEST_FUNCTION(system_info_init_warm_start, nullptr, nullptr)
{
    // Set up expectations
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);

    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 3;

    expect_value(__wrap_mmio_read32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_is_warm_start() == true);
}

TEST_FUNCTION(system_info_init_secure_mode, nullptr, nullptr)
{
    // Set up expectations
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);

    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_security_state() == HSP_SECURITY_STATE_SECURE);
}

TEST_FUNCTION(system_info_init_nonsecure_mode, nullptr, nullptr)
{
    // Set up expectations
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_TEST);

    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_security_state() != HSP_SECURITY_STATE_SECURE);
}
} // extern "C"
