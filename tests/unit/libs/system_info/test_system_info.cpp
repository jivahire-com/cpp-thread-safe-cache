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
    uint16_t request_cmd = output_message->header.cmd;

    output_message->header.cmd = mock_type(uint16_t);

    if (request_cmd == HSP_MAILBOX_CMD_GET_SECURITY_STATE_REQ)
    {
        output_message->policy_status_rsp.policy_status.security_state = mock_type(uint8_t);
        output_message->policy_status_rsp.policy_status.mission_mode = mock_type(bool);
        output_message->policy_status_rsp.policy_status.cli_enable = mock_type(bool);
        output_message->policy_status_rsp.policy_status.watchdog_enable = mock_type(bool);
    }
    else if (request_cmd == HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_REQ)
    {
        output_message->boot_profile_rsp.boot_profile = mock_type(uint8_t);
    }

    *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);

    return FPFW_ICC_BASE_STATUS_SUCCESS;
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected(addr);
    return mock_type(uint32_t);
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(uint8_t);
}

TEST_FUNCTION(test_get_board_id, nullptr, nullptr)
{
    // Set up expectations
    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;
    boot_meta_data.BoardId = 15;
    boot_meta_data.BoardIdRework = 10;
    boot_meta_data.SocPos = 0x1;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    uint8_t board_id = system_info_get_board_id();

    // Verify expectations
    assert_true(board_id == 15);

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);
    uint8_t board_rework_id = system_info_get_board_rework_id();
    // verify rework ID
    assert_true(board_rework_id == 10);

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);
    uint8_t soc_pos = system_info_get_soc_position();
    // verify soc position
    assert_true(soc_pos == 1);
}

TEST_FUNCTION(system_info_init_hsp_present, nullptr, nullptr)
{
    // Set up expectations
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

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
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

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
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

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
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

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

TEST_FUNCTION(system_info_init_mission_mode, nullptr, nullptr)
{
    // Set up expectations for mission mode return false
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 4;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_mission_mode() == true);
}

TEST_FUNCTION(system_info_init_cli_enable, nullptr, nullptr)
{
    // Set up expectations for cli_enable return true
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_cli_enable() == true);

    // Set up expectations for cli_enable return false
    icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_cli_enable() == false);

    // Set up expectations for icc_base_ctx is NULL
    icc_base_ctx = NULL;

    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_cli_enable() == true);
}

TEST_FUNCTION(system_info_init_watchdog_disable_allowed, nullptr, nullptr)
{
    // Set up expectations for watchdog_enable return false
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_watchdog_disable_allowed() == true);

    // Set up expectations for watchdog_enable return true
    icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_watchdog_disable_allowed() == false);
}

TEST_FUNCTION(system_info_init_reset_reason, nullptr, nullptr)
{
    // Set up expectations for reset reason is HSP_FIRMWARE_RESET_REASON_CRASH
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x00);

    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 2;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_reset_reason() == 2);
}

TEST_FUNCTION(test_system_info_init_complete, nullptr, nullptr)
{
    // Verify default status
    assert_true(system_info_is_init_complete() == false);

    // Call API under test
    system_info_set_init_complete();

    // Verify API expectations
    assert_true(system_info_is_init_complete() == true);
}

TEST_FUNCTION(test_system_info_get_platform, nullptr, nullptr)
{
    // Set up expectations for PLATFORM_FPGA
    will_return(__wrap_idsw_get_platform_sdv, 0x30);

    // Verify API expectations
    assert_true(system_info_get_platform() == 0x30);

    // Set up expectations for PLATFORM_SVP_SIM
    will_return(__wrap_idsw_get_platform_sdv, 0x41);

    // Verify API expectations
    assert_true(system_info_get_platform() == 0x41);
}

TEST_FUNCTION(system_info_init_bmc_profile, nullptr, nullptr)
{
    // Set up expectations
    fpfw_icc_base_ctx_t* icc_base_ctx = (fpfw_icc_base_ctx_t*)0x1234;
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_SECURITY_STATE_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_SECURITY_STATE_SECURE);
    // mission_mode
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // cli_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, true);
    // watchdog_enable
    will_return(__wrap_fpfw_icc_base_send_recv_sync, false);
    // boot profile query
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_GET_BOOT_PROFILE_CMD_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, 0x01);

    HSP_BOOT_METADATA boot_meta_data;
    boot_meta_data.MetadataVersion = 1;
    boot_meta_data.ResetReason = 7;

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, boot_meta_data.AsUint32);

    // Call API under test
    system_info_init(icc_base_ctx);

    // Verify expectations
    assert_true(system_info_get_bmc_profile() == 0x01);
}

} // extern "C"
