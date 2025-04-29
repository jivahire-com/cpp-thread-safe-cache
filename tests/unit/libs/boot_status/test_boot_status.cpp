//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_boot_status.cpp
 * Implements test cases for Boot Status APIs
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwLock.h>             // for FPFW_LOCK, FpFwLockAcquire, FpFwLockRelease
#include <FpFwUtils.h>            // for FPFW_UNUSED
#include <boot_status.h>          // for boot_status_notify
#include <fpfw_icc_base.h>        // for fpfw_icc_base_send_sync, fpfw...
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#include <idsw_kng.h>             // for idsw_get_cpu_type
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <system_info.h> // for system_info_is_init_complete

/*-- Symbolic Constant Macros (defines) --*/
#define BUG_CHECK_RETURN_VALUE 0x1
#define BUGCHECK_MOCK_RETURN   (setjmp(cd_mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static icc_base_send_complete_notify s_stored_send_cb = NULL;
static void* s_stored_send_context = NULL;
jmp_buf cd_mock_jump_buf;

/*------------- Mock Functions ----------------*/
fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    assert_non_null(icc_ctx);
    assert_non_null(payload_buffer);
    assert_true(buffer_size > 0);

    kng_hsp_mailbox_msg* output_message = (kng_hsp_mailbox_msg*)payload_buffer;
    assert_int_equal(output_message->header.cmd, HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY);
    function_called();

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    assert_non_null(icc_ctx);
    assert_non_null(params);

    // save the passed in callback
    s_stored_send_cb = params->cb;
    kng_hsp_mailbox_msg* output_message = (kng_hsp_mailbox_msg*)params->payload_buffer;
    assert_int_equal(output_message->header.cmd, HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY);
    s_stored_send_context = params->cb_ctx;

    function_called();
    return mock_type(fpfw_status_t);
}

bool __wrap_system_info_is_init_complete()
{
    return mock_type(bool);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

_Noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Handle noreturn, allowing control to return to test
    longjmp(cd_mock_jump_buf, BUG_CHECK_RETURN_VALUE);
}

/*------------- Functions ----------------*/
static int setup(void** state)
{
    FPFW_UNUSED(state);
    uint32_t dummy = 0;
    static boot_status_icc_ctx_t boot_status_ctx = {0};
    //! Initialize the ctx to be a dummy non null for tests
    boot_status_ctx.hsp_mbx_ctx = (fpfw_icc_base_ctx_t*)&dummy;

    // Initialize any resources needed for the tests
    boot_status_init(&boot_status_ctx);
    return 0;
}

static int cleanup(void** state)
{
    FPFW_UNUSED(state);
    // Reset any resources needed for the tests
    s_stored_send_cb = NULL;
    s_stored_send_context = NULL;
    boot_status_reset();
    return 0;
}

static void test_boot_status_notify_extd_cb(void* ctx)
{
    FPFW_UNUSED(ctx);
    function_called();
}

/*------------- Test Cases ----------------*/
TEST_FUNCTION(test_boot_status_notify_extd_invalid_args_scp_die0, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    //! Set up expectations for SCP
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    mscp_boot_status_code_t unused = MSCP_BOOT_STATUS_CODE_UNUSED; //! hsp doesn't check boot status
    uint32_t err_boot_status_ex = 0;

    //! Call FUT & Expect failures
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        //! Null param
        boot_status_notify_extd(NULL, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! cmp group HSP is not supported
        err_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_HSP, MSCP_GENERIC, SCP_PRIMARY);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Comp grp & instance don't match
        err_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, MCP_PRIMARY);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! invalid subgroup, out of range
        err_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_SUBGROUP_MAX, SCP_PRIMARY);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! invalid instance, out of range instance
        err_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, MAX_COMPONENT_INSTANCE);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! out of range status, not supported for SCP led status
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_CDED1_HW_FAULT + 1);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! out of range status, not supported for SCP led status
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP0_OK - 1);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! out of range status, not supported for SCP led status
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP1_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! out of range status, not supported for SCP led status
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP1_E_DDR6_TRAINING);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! led status (is for die1) mismatch for component grp SCP_PRIMARY (die 0)
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP1_MESH_INIT_START);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Mismatch for componenet group (MCP) & instance for SCP
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_MCP, MSCP_ACCEL, SCP_PRIMARY, BOOT_STATUS_CODE_SCP0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Mismatch for componenet group (SDM) & instance for SCP
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SDM, MSCP_ACCEL, SCP_PRIMARY, BOOT_STATUS_CODE_SDM0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Mismatch for componenet group (CDED) & instance for SCP
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_CDED, MSCP_ACCEL, SCP_PRIMARY, BOOT_STATUS_CODE_SDM0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Invalid boot status code. BOOT_STATUS_CODE_SCP0_OK is not valid for SDM die 0
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SDM, MSCP_ACCEL, SDM_PRIMARY, BOOT_STATUS_CODE_SCP0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Invalid boot status code. BOOT_STATUS_CODE_SCP0_OK is not valid for CDED die 0
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_CDED, MSCP_ACCEL, CDED_PRIMARY, BOOT_STATUS_CODE_SCP0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
}

TEST_FUNCTION(test_boot_status_notify_extd_invalid_args_scp_die1, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    //! Set up expectations for SCP
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    mscp_boot_status_code_t unused = MSCP_BOOT_STATUS_CODE_UNUSED; //! hsp doesn't check boot status
    uint32_t err_boot_status_ex = 0;

    //! Call FUT & Expect failures
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        //! Invalid DIE instance, SCP die 1 should be SCP_SECONDARY
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP1_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Invalid DIE instance, SDM die 1 should be SDM_SECONDARY
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SDM, MSCP_GENERIC, SDM_PRIMARY, BOOT_STATUS_CODE_SDM1_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Invalid DIE instance, CDED die 1 should be CDED_SECONDARY
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_CDED, MSCP_GENERIC, CDED_PRIMARY, BOOT_STATUS_CODE_CDED1_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Invalid boot status code. BOOT_STATUS_CODE_CDED1_OK is not valid for SCP die 1
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_SECONDARY, BOOT_STATUS_CODE_CDED1_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Invalid boot status code. BOOT_STATUS_CODE_SCP0_OK is not valid for SDM die 1
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SDM, MSCP_ACCEL, SDM_SECONDARY, BOOT_STATUS_CODE_SCP0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Invalid boot status code. BOOT_STATUS_CODE_SCP0_OK is not valid for CDED die 1
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_CDED, MSCP_ACCEL, CDED_SECONDARY, BOOT_STATUS_CODE_SCP0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! out of range status, not supported for SCP led status
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_SECONDARY, BOOT_STATUS_CODE_SCP0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! out of range status, not supported for SCP led status
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_SECONDARY, BOOT_STATUS_CODE_SDM0_HW_FAULT);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! out of range status, not supported for SCP led status
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_SECONDARY, BOOT_STATUS_CODE_SCP_E_DDR5_TRAINING);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
}

TEST_FUNCTION(test_boot_status_notify_extd_invalid_args_mcp, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    mscp_boot_status_code_t unused = MSCP_BOOT_STATUS_CODE_UNUSED; //! hsp doesn't check boot status
    uint32_t err_boot_status_ex = 0;

    //! Set up expectations for MCP
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);

    //! Call FUT & Expect failures
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        will_return(__wrap_idsw_get_die_id, DIE_0);
        //! Invalid DIE instance, MCP die 0 should be MCP_PRIMARY
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_SECONDARY, BOOT_STATUS_CODE_MCP0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        will_return(__wrap_idsw_get_die_id, DIE_1);
        //! Invalid DIE instance, MCP die 1 should be MCP_SECONDARY
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_PRIMARY, BOOT_STATUS_CODE_MCP1_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        will_return(__wrap_idsw_get_die_id, DIE_0);
        //! Invalid boot status code. BOOT_STATUS_CODE_SCP0_OK is not valid for MCP die 0
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_PRIMARY, BOOT_STATUS_CODE_SCP0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        will_return(__wrap_idsw_get_die_id, DIE_1);
        //! Invalid boot status code. BOOT_STATUS_CODE_SCP0_OK is not valid for MCP die 1
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_SECONDARY, BOOT_STATUS_CODE_SCP0_OK);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
}

TEST_FUNCTION(test_boot_status_notify_extd_valid_state, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = NULL;
    test_req_mem.cb_ctx = NULL;
    uint32_t valid_boot_status_ex;

    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, false);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_send_sync);

    /********************************** LED status code is 0 *********************************************/
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    valid_boot_status_ex = GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, 0);
    boot_status_notify_extd(&test_req_mem, 0, valid_boot_status_ex);

    /******************************************* SCP DIE 0 *************************************************/
    uint8_t scp_die0_boot_status[] = {
        BOOT_STATUS_CODE_SCP0_OK,
        BOOT_STATUS_CODE_SCP0_MESH_INIT_START,
        BOOT_STATUS_CODE_SCP0_TOWER_INIT_START,
        BOOT_STATUS_CODE_SCP0_ACCEL_INIT_START,
        BOOT_STATUS_CODE_SCP0_DDR_INIT_START,
        BOOT_STATUS_CODE_SCP0_BOOT_COMPLETE,
        BOOT_STATUS_CODE_SCP_ACCEL_FAILED,
        BOOT_STATUS_CODE_SCP_E_DDR0_TRAINING,
        BOOT_STATUS_CODE_SCP_E_DDR1_TRAINING,
        BOOT_STATUS_CODE_SCP_E_DDR2_TRAINING,
        BOOT_STATUS_CODE_SCP_E_DDR3_TRAINING,
        BOOT_STATUS_CODE_SCP_E_DDR4_TRAINING,
        BOOT_STATUS_CODE_SCP_E_DDR5_TRAINING,
    };
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, FPFW_ARRAY_SIZE(scp_die0_boot_status));
    will_return_count(__wrap_idsw_get_die_id, DIE_0, FPFW_ARRAY_SIZE(scp_die0_boot_status));
    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(scp_die0_boot_status); i++)
    {
        valid_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, scp_die0_boot_status[i]);
        boot_status_notify_extd(&test_req_mem, 0, valid_boot_status_ex);
    }
    /******************************************* SCP DIE 1 *************************************************/
    uint8_t scp_die1_boot_status[] = {
        BOOT_STATUS_CODE_SCP_ACCEL_FAILED,
        BOOT_STATUS_CODE_SCP1_OK,
        BOOT_STATUS_CODE_SCP1_MESH_INIT_START,
        BOOT_STATUS_CODE_SCP1_TOWER_INIT_START,
        BOOT_STATUS_CODE_SCP1_ACCEL_INIT_START,
        BOOT_STATUS_CODE_SCP1_DDR_INIT_START,
        BOOT_STATUS_CODE_SCP1_BOOT_COMPLETE,
        BOOT_STATUS_CODE_SCP1_E_DDR6_TRAINING,
        BOOT_STATUS_CODE_SCP1_E_DDR7_TRAINING,
        BOOT_STATUS_CODE_SCP1_E_DDR8_TRAINING,
        BOOT_STATUS_CODE_SCP1_E_DDR9_TRAINING,
        BOOT_STATUS_CODE_SCP1_E_DDR10_TRAINING,
        BOOT_STATUS_CODE_SCP1_E_DDR11_TRAINING,
    };

    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, FPFW_ARRAY_SIZE(scp_die1_boot_status));
    will_return_count(__wrap_idsw_get_die_id, DIE_1, FPFW_ARRAY_SIZE(scp_die1_boot_status));

    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(scp_die1_boot_status); i++)
    {
        valid_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_SECONDARY, scp_die1_boot_status[i]);
        boot_status_notify_extd(&test_req_mem, 0, valid_boot_status_ex);
    }

    /******************************************* MCP DIE 0 *************************************************/
    uint8_t mcp_die0_boot_status[] = {
        BOOT_STATUS_CODE_MCP0_OK,
        BOOT_STATUS_CODE_MCP0_BOOT_COMPLETE,
    };
    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, FPFW_ARRAY_SIZE(mcp_die0_boot_status));
    will_return_count(__wrap_idsw_get_die_id, DIE_0, FPFW_ARRAY_SIZE(mcp_die0_boot_status));
    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(mcp_die0_boot_status); i++)
    {
        valid_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_PRIMARY, mcp_die0_boot_status[i]);
        boot_status_notify_extd(&test_req_mem, 0, valid_boot_status_ex);
    }
    /******************************************* MCP DIE 1 *************************************************/
    uint8_t mcp_die1_boot_status[] = {
        BOOT_STATUS_CODE_MCP1_OK,
        BOOT_STATUS_CODE_MCP1_BOOT_COMPLETE,
    };
    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, FPFW_ARRAY_SIZE(mcp_die1_boot_status));
    will_return_count(__wrap_idsw_get_die_id, DIE_1, FPFW_ARRAY_SIZE(mcp_die1_boot_status));
    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(mcp_die1_boot_status); i++)
    {
        valid_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_SECONDARY, mcp_die1_boot_status[i]);
        boot_status_notify_extd(&test_req_mem, 0, valid_boot_status_ex);
    }

    /******************************************* SDM DIE 0 *************************************************/
    uint8_t sdm_die0_boot_status[] = {
        BOOT_STATUS_CODE_SDM0_OK,
        BOOT_STATUS_CODE_SDM0_BOOT_COMPLETE,
        BOOT_STATUS_CODE_SDM0_HW_FAULT,
    };
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, FPFW_ARRAY_SIZE(sdm_die0_boot_status));
    will_return_count(__wrap_idsw_get_die_id, DIE_0, FPFW_ARRAY_SIZE(sdm_die0_boot_status));
    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(sdm_die0_boot_status); i++)
    {
        valid_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SDM, MSCP_ACCEL, SDM_PRIMARY, sdm_die0_boot_status[i]);
        boot_status_notify_extd(&test_req_mem, 0, valid_boot_status_ex);
    }

    /******************************************* SDM DIE 1 *************************************************/
    uint8_t sdm_die1_boot_status[] = {
        BOOT_STATUS_CODE_SDM1_OK,
        BOOT_STATUS_CODE_SDM1_BOOT_COMPLETE,
        BOOT_STATUS_CODE_SDM1_HW_FAULT,
    };
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, FPFW_ARRAY_SIZE(sdm_die1_boot_status));
    will_return_count(__wrap_idsw_get_die_id, DIE_1, FPFW_ARRAY_SIZE(sdm_die1_boot_status));
    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(sdm_die1_boot_status); i++)
    {
        valid_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SDM, MSCP_ACCEL, SDM_SECONDARY, sdm_die1_boot_status[i]);
        boot_status_notify_extd(&test_req_mem, 0, valid_boot_status_ex);
    }

    /******************************************* CDED DIE 0 *************************************************/
    uint8_t cded_die0_boot_status[] = {
        BOOT_STATUS_CODE_CDED0_OK,
        BOOT_STATUS_CODE_CDED0_BOOT_COMPLETE,
        BOOT_STATUS_CODE_CDED0_HW_FAULT,
    };
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, FPFW_ARRAY_SIZE(cded_die0_boot_status));
    will_return_count(__wrap_idsw_get_die_id, DIE_0, FPFW_ARRAY_SIZE(cded_die0_boot_status));
    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(cded_die0_boot_status); i++)
    {
        valid_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_CDED, MSCP_ACCEL, CDED_PRIMARY, cded_die0_boot_status[i]);
        boot_status_notify_extd(&test_req_mem, 0, valid_boot_status_ex);
    }

    /******************************************* CDED DIE 1 *************************************************/
    uint8_t cded_die1_boot_status[] = {
        BOOT_STATUS_CODE_CDED1_OK,
        BOOT_STATUS_CODE_CDED1_BOOT_COMPLETE,
        BOOT_STATUS_CODE_CDED1_HW_FAULT,
    };
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, FPFW_ARRAY_SIZE(cded_die1_boot_status));
    will_return_count(__wrap_idsw_get_die_id, DIE_1, FPFW_ARRAY_SIZE(cded_die1_boot_status));
    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(cded_die1_boot_status); i++)
    {
        valid_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_CDED, MSCP_ACCEL, CDED_SECONDARY, cded_die1_boot_status[i]);
        boot_status_notify_extd(&test_req_mem, 0, valid_boot_status_ex);
    }
}

/**
 * @brief Test for current state of the system & verify all pre reqs are met
 * for boot status notify. This includes initializing the icc base ctx,
 * hsp presence.
 */
TEST_FUNCTION(test_boot_status_notify_extd_invalid_state, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    uint32_t valid_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY);

    //! Set up expectations for SCP
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    //! hsp is not present & no init
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    }

    //! boot status init hasn't been called prior, but hsp is present
    will_return(__wrap_system_info_is_hsp_present, true);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    }
}

TEST_FUNCTION(test_boot_status_notify_extd_invalid_hsp_icc_ctx, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    uint32_t valid_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY);

    //! Set up expectations for SCP
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    //! hsp is not present & no init
    will_return(__wrap_system_info_is_hsp_present, true);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        boot_status_icc_ctx_t boot_status_ctx;
        boot_status_ctx.hsp_mbx_ctx = (fpfw_icc_base_ctx_t*)0xABCD;
        boot_status_init(&boot_status_ctx);
        boot_status_ctx.hsp_mbx_ctx = NULL;
        boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    }
}

/**
 * @brief Test to verify the boot status notify API behavior when invoked during init
 *
 */
TEST_FUNCTION(test_boot_status_notify_extd_sync, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = test_boot_status_notify_extd_cb;
    test_req_mem.cb_ctx = NULL;

    //! Set up expectations, module initialized, scp core, hsp is present
    uint32_t valid_boot_status_ex =
        GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP0_OK);
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is still in init phase
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, false);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    //! Simulate icc base sync send failure
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    }

    //! Simulate icc base sync send success
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    expect_function_call(test_boot_status_notify_extd_cb);
    //! Call FUT & Expect success
    boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
}

/**
 * @brief Test to verify the boot status notify API behavior when invoked during runtime
 *
 */
TEST_FUNCTION(test_boot_status_notify_extd_async, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = test_boot_status_notify_extd_cb;
    test_req_mem.cb_ctx = NULL;
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is in runtime phase
    uint32_t valid_boot_status_ex =
        GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP0_OK);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, true);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    //! Simulate icc base async send failure
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    }

    //! Simulate icc base async send success
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect success
    boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    //! @note: The success here indicates the async send request was queued in the dfwk successfully
    //! The actual send completion is verified in the send completion callback
    //! Simulate raising the ICC internal send completion callback, this will call the user provided cb
    expect_function_call(test_boot_status_notify_extd_cb);
    s_stored_send_cb(s_stored_send_context, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_boot_status_notify_extd_cb_invalid_arg, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = NULL;
    test_req_mem.cb_ctx = NULL;
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is in runtime phase
    uint32_t valid_boot_status_ex =
        GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP0_OK);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, true);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    //! Simulate icc base async send success
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect success
    boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);

    if (!bugcheck_mock_return())
    {
        s_stored_send_cb(s_stored_send_context, FPFW_STATUS_FAIL);
    }
    if (!bugcheck_mock_return())
    {
        s_stored_send_cb(NULL, FPFW_STATUS_SUCCESS);
    }
    if (!bugcheck_mock_return())
    {
        boot_status_req_t* req = (boot_status_req_t*)s_stored_send_context;
        req->msg.header.cmd = HSP_MAILBOX_CMD_MAX;
        s_stored_send_cb(s_stored_send_context, FPFW_STATUS_SUCCESS);
    }
}

TEST_FUNCTION(test_post_led_status_invalid_args, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    //! Set up expectations for SCP
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, 3);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    led_status_codes_t led_status = LED_STATUS_CODE_MAX;

    //! Call FUT & Expect failures
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        //! Null param
        post_led_status(NULL, led_status);
    }
    if (!bugcheck_mock_return())
    {
        //! led status is invalid & out of range
        post_led_status(&test_req_mem, led_status);
    }
    if (!bugcheck_mock_return())
    {
        //! Led status doesn't match current core supported status
        //! current core is SCP, led status is for MCP
        led_status = LED_STATUS_CODE_MCP_OK;
        post_led_status(&test_req_mem, led_status);
    }

    //! Update current core to MCP for further tests
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);
    if (!bugcheck_mock_return())
    {
        //! led status applicable for only SCP core, but current core is MCP
        led_status = LED_STATUS_CODE_SCP_MESH_INIT_START;
        post_led_status(&test_req_mem, led_status);
    }
    if (!bugcheck_mock_return())
    {
        //! Again only SCP core collects the led status from accel cores, current core is MCP
        led_status = LED_STATUS_CODE_SDM_BOOT_COMPLETE;
        post_led_status(&test_req_mem, led_status);
    }
    if (!bugcheck_mock_return())
    {
        //! SCP error code, but current core is MCP
        led_status = LED_STATUS_CODE_SCP_E_DDR0_TRAINING;
        post_led_status(&test_req_mem, led_status);
    }
}

TEST_FUNCTION(test_post_led_status_valid_args_scp_die0, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = NULL;
    test_req_mem.cb_ctx = NULL;
    led_status_codes_t led_status[] = {
        LED_STATUS_CODE_SCP_OK,
        LED_STATUS_CODE_SCP_MESH_INIT_START,
        LED_STATUS_CODE_SCP_TOWER_INIT_START,
        LED_STATUS_CODE_SCP_ACCEL_INIT_START,
        LED_STATUS_CODE_SCP_DDR_INIT_START,
        LED_STATUS_CODE_SCP_BOOT_COMPLETE,
        LED_STATUS_CODE_SDM_OK,
        LED_STATUS_CODE_SDM_BOOT_COMPLETE,
        LED_STATUS_CODE_CDED_OK,
        LED_STATUS_CODE_CDED_BOOT_COMPLETE,
        LED_STATUS_CODE_SCP_ACCEL_FAILED,
        LED_STATUS_CODE_SCP_E_DDR0_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR1_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR2_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR3_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR4_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR5_TRAINING,
        LED_STATUS_CODE_SDM_HW_FAULT,
        LED_STATUS_CODE_CDED_HW_FAULT,
    };

    //! Set up expectations, module initialized, scp core, hsp is present
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, false);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_send_sync);

    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(led_status); i++)
    {
        post_led_status(&test_req_mem, led_status[i]);
    }
}

TEST_FUNCTION(test_post_led_status_valid_args_scp_die1, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = NULL;
    test_req_mem.cb_ctx = NULL;
    led_status_codes_t led_status[] = {
        LED_STATUS_CODE_SCP_OK,
        LED_STATUS_CODE_SCP_MESH_INIT_START,
        LED_STATUS_CODE_SCP_TOWER_INIT_START,
        LED_STATUS_CODE_SCP_ACCEL_INIT_START,
        LED_STATUS_CODE_SCP_DDR_INIT_START,
        LED_STATUS_CODE_SCP_BOOT_COMPLETE,
        LED_STATUS_CODE_SDM_OK,
        LED_STATUS_CODE_SDM_BOOT_COMPLETE,
        LED_STATUS_CODE_CDED_OK,
        LED_STATUS_CODE_CDED_BOOT_COMPLETE,
        LED_STATUS_CODE_SCP_ACCEL_FAILED,
        LED_STATUS_CODE_SCP_E_DDR0_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR1_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR2_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR3_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR4_TRAINING,
        LED_STATUS_CODE_SCP_E_DDR5_TRAINING,
        LED_STATUS_CODE_SDM_HW_FAULT,
        LED_STATUS_CODE_CDED_HW_FAULT,
    };

    //! Set up expectations, module initialized, scp core, hsp is present
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, false);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_send_sync);

    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(led_status); i++)
    {
        post_led_status(&test_req_mem, led_status[i]);
    }
}

TEST_FUNCTION(test_post_led_status_valid_args_mcp_die0, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = NULL;
    test_req_mem.cb_ctx = NULL;
    led_status_codes_t led_status[] = {
        LED_STATUS_CODE_MCP_OK,
        LED_STATUS_CODE_MCP_BOOT_COMPLETE,
    };

    //! Set up expectations, module initialized, scp core, hsp is present
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, false);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_send_sync);

    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(led_status); i++)
    {
        post_led_status(&test_req_mem, led_status[i]);
    }
}

TEST_FUNCTION(test_post_led_status_valid_args_mcp_die1, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = NULL;
    test_req_mem.cb_ctx = NULL;
    led_status_codes_t led_status[] = {
        LED_STATUS_CODE_MCP_OK,
        LED_STATUS_CODE_MCP_BOOT_COMPLETE,
    };

    //! Set up expectations, module initialized, scp core, hsp is present
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, false);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_send_sync);

    for (uint8_t i = 0; i < FPFW_ARRAY_SIZE(led_status); i++)
    {
        post_led_status(&test_req_mem, led_status[i]);
    }
}

TEST_FUNCTION(test_post_led_status_invalid_state, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    led_status_codes_t led_status = LED_STATUS_CODE_SCP_OK;

    //! Set up expectations for SCP
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    //! hsp is not present & boot status init not called
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        post_led_status(&test_req_mem, led_status);
    }

    //! boot status init hasn't been called prior, but hsp is present
    will_return(__wrap_system_info_is_hsp_present, true);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        post_led_status(&test_req_mem, led_status);
    }
}

TEST_FUNCTION(test_post_led_status_sync, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = test_boot_status_notify_extd_cb;
    test_req_mem.cb_ctx = NULL;

    //! Set up expectations, module initialized, scp core, hsp is present
    led_status_codes_t led_status = LED_STATUS_CODE_SCP_OK;

    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is still in init phase
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, false);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    //! Simulate icc base sync send failure
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        post_led_status(&test_req_mem, led_status);
    }

    //! Simulate icc base sync send success
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    expect_function_call(test_boot_status_notify_extd_cb);
    //! Call FUT & Expect success
    post_led_status(&test_req_mem, led_status);
}

TEST_FUNCTION(test_post_led_status_async, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = test_boot_status_notify_extd_cb;
    test_req_mem.cb_ctx = NULL;
    led_status_codes_t led_status = LED_STATUS_CODE_SCP_OK;
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is in runtime phase
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, true);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    //! Simulate icc base async send failure
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        post_led_status(&test_req_mem, led_status);
    }

    //! Simulate icc base async send success
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect success
    post_led_status(&test_req_mem, led_status);
    //! @note: The success here indicates the async send request was queued in the dfwk successfully
    //! The actual send completion is verified in the send completion callback
    //! Simulate raising the ICC internal send completion callback, this will call the user provided cb
    expect_function_call(test_boot_status_notify_extd_cb);
    s_stored_send_cb(s_stored_send_context, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_boot_status_init_invalid_args, nullptr, nullptr)
{
    boot_status_icc_ctx_t boot_status_ctx;
    boot_status_ctx.hsp_mbx_ctx = NULL;

    //! Call FUT & Expect failures
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        //! Null param
        boot_status_init(NULL);
    }
    if (!bugcheck_mock_return())
    {
        //! Invalid icc ctx
        boot_status_init(&boot_status_ctx);
    }
}

} // extern "C"
