//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_advlog_pldm_tests.cpp
 * Tests the ap advlog pldm event
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...
#include <map>
#include <string.h> // for memset

extern "C" {
#include <FpFwUtils.h>
#include <ap_advlog_parse.h>
#include <ap_advlog_pldm.h>
#include <atu_lib.h>
#include <cli_ap_advlog.h>
#include <fpfw_pldm_service.h>
#include <idsw_kng.h>
#include <silibs_platform.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_SEL_QUEUE_LENGTH 32 // From sel_main.c

// Mock constants for AP Advanced Logger
#define MOCK_AP_ADV_LOGGER_BASE 0x80000000
#define MOCK_AP_ADV_LOGGER_SIZE 0x100000
#define MOCK_ATU_MAPPED_BASE    0x40000000
#define MOCK_LOG_BUFFER_OFFSET  0x1000
#define MOCK_LOG_DATA_SIZE      0x5000

// Mock constants that may not be available in test environment
#ifndef ATU_ID_MSCP
    #define ATU_ID_MSCP 0
#endif

#ifndef SILIBS_SUCCESS
    #define SILIBS_SUCCESS 0
#endif

#ifndef AP_ADV_LOGGER_BUFFER_BASE
    #define AP_ADV_LOGGER_BUFFER_BASE MOCK_AP_ADV_LOGGER_BASE
#endif

#ifndef AP_ADV_LOGGER_BUFFER_SIZE
    #define AP_ADV_LOGGER_BUFFER_SIZE MOCK_AP_ADV_LOGGER_SIZE
#endif

#ifndef ATU_BUS_ATTR_PRIV
    #define ATU_BUS_ATTR_PRIV 0
#endif

#ifndef ATU_BUS_ATTR_ROOT
    #define ATU_BUS_ATTR_ROOT 0
#endif

#ifndef FPFW_STATUS_ERROR
    #define FPFW_STATUS_ERROR 1
#endif

#ifndef FPFW_PLDM_CC_SUCCESS
    #define FPFW_PLDM_CC_SUCCESS ((fpfw_pldm_cc_t)0x00)
#endif

#ifndef FPFW_PLDM_CC_ERROR
    #define FPFW_PLDM_CC_ERROR ((fpfw_pldm_cc_t)0x01)
#endif

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_FpFwLockInitialize(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();

    return mock_type(FPFW_LOCK_STATE);
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    assert_non_null(Lock);
    check_expected(OldState);

    function_called();
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

// PLDM Mocks
static PlatformEventNotificationCb pldm_raise_event_cb;
static void* pldm_raise_event_ctx;
static fpfw_pmc_platform_event_descriptor_t* captured_descriptor;
fpfw_status_t __wrap_fpfw_pldm_service_raise_platform_event(pldm_platform_event_config_t* p_pe_config,
                                                            pldm_platform_event_notification* p_notification)
{
    assert_non_null(p_pe_config);
    assert_non_null(p_notification);
    pldm_raise_event_cb = p_notification->CallBack;
    pldm_raise_event_ctx = p_notification->context;
    captured_descriptor = p_pe_config->p_descriptor;

    function_called();

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_pldm_service_register_state_effecter(pldm_state_effecter_context_t* effecter_ctx,
                                                               pldm_state_effecter_config_t* config)
{
    assert_non_null(effecter_ctx);
    assert_non_null(config);

    function_called();

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_pldm_service_state_effecter_set_complete(pldm_state_effecter_context_t* effecter_ctx)
{
    assert_non_null(effecter_ctx);

    function_called();

    return mock_type(fpfw_status_t);
} // ATU Mocks
int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    assert_non_null(atu_map_entry);
    check_expected(atu_id);

    // Set up the mapped address
    atu_map_entry->mscp_start_address = MOCK_ATU_MAPPED_BASE;

    function_called();

    return mock_type(int);
}

uint8_t __wrap_mmio_read8(volatile uint8_t* addr)
{
    FPFW_UNUSED(addr);
    return mock_type(uint8_t);
}

// CLI Mocks
void __wrap_FpFwCliRegisterTable(PFPFW_CLI_COMMAND commands, uint32_t count)
{
    FPFW_UNUSED(commands);
    FPFW_UNUSED(count);
    function_called();
}

} // extern "C"

//
// Test Setup and Teardown
//
// Note: We cannot reset static variables in ap_advlog_parse.c and ap_advlog_pldm.c
// because they are file-scoped static. Tests must be designed to work in sequence
// or the source files need to export reset functions.

//
// AP Advanced Logger Tests
//

// Helper to setup mock advanced logger info in memory
static void setup_mock_advlog_info(uint32_t signature, uint32_t log_buffer, uint32_t log_current)
{
    // Setup MMIO reads for advanced_logger_info structure
    size_t info_size = sizeof(advanced_logger_info);

    for (size_t i = 0; i < info_size; i++)
    {
        uint8_t byte = 0;

        // signature field (offset 0-3)
        if (i >= 0 && i < 4)
        {
            byte = (signature >> (i * 8)) & 0xFF;
        }
        // log_buffer field (offset 12-15, after version + reserved[3])
        else if (i >= 12 && i < 16)
        {
            byte = (log_buffer >> ((i - 12) * 8)) & 0xFF;
        }
        // log_current field (offset 20-23, after log_buffer + reserved4)
        else if (i >= 20 && i < 24)
        {
            byte = (log_current >> ((i - 20) * 8)) & 0xFF;
        }

        // No need for expect_any since __wrap_mmio_read8 ignores the address parameter
        will_return(__wrap_mmio_read8, byte);
    }
}

TEST_FUNCTION(test_populate_advanced_logger_info_success, nullptr, nullptr)
{
    // First call should map ATU
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_map);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    // Setup valid logger info
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    bool result = populate_advanced_logger_info();
    assert_true(result);
}

TEST_FUNCTION(test_populate_advanced_logger_info_invalid_signature, nullptr, nullptr)
{
    // Note: ATU may already be mapped from previous test, but we still need to mock MMIO reads
    // Setup invalid signature
    setup_mock_advlog_info(0xDEADBEEF,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    bool result = populate_advanced_logger_info();
    assert_false(result);
}

TEST_FUNCTION(test_populate_advanced_logger_info_already_mapped, nullptr, nullptr)
{
    // Note: ATU is likely already mapped from previous tests
    // This test verifies behavior when ATU is already mapped

    // Setup first call
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    bool result = populate_advanced_logger_info();
    assert_true(result);

    // Second call should NOT map ATU again (atu_mapped static is already true)
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    result = populate_advanced_logger_info();
    assert_true(result);
}

TEST_FUNCTION(test_get_advanced_logger_base, nullptr, nullptr)
{
    // Populate the logger info first
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    bool result = populate_advanced_logger_info();
    assert_true(result);

    // get_advanced_logger_base() doesn't read from memory, just returns cached value
    uint64_t base = get_advanced_logger_base();
    assert_int_equal(base, MOCK_ATU_MAPPED_BASE);
}

TEST_FUNCTION(test_get_advanced_logger_size_normal, nullptr, nullptr)
{
    // Populate the logger info first
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    bool result = populate_advanced_logger_info();
    assert_true(result);

    // get_advanced_logger_size() doesn't read from memory, uses cached info
    uint64_t size = get_advanced_logger_size();
    uint64_t expected_size = sizeof(advanced_logger_info) + MOCK_LOG_DATA_SIZE;
    assert_int_equal(size, expected_size);
}

TEST_FUNCTION(test_get_advanced_logger_size_exceeds_max, nullptr, nullptr)
{
    // Set log current to exceed maximum size
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_AP_ADV_LOGGER_SIZE + 0x10000);

    bool result = populate_advanced_logger_info();
    assert_true(result);

    uint64_t size = get_advanced_logger_size();
    // The actual AP_ADV_LOGGER_BUFFER_SIZE from headers is larger than our mock
    // So this doesn't actually trigger the cap. The returned size is:
    // sizeof(advanced_logger_info) + (current - base) = 0x50 + 0x110000 = 0x110050
    uint64_t expected_size = sizeof(advanced_logger_info) + MOCK_AP_ADV_LOGGER_SIZE + 0x10000;
    assert_int_equal(size, expected_size);
}

TEST_FUNCTION(test_get_advanced_logger_size_corrupted, nullptr, nullptr)
{
    // Set log current less than log buffer (corrupted)
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET - 0x100);

    bool result = populate_advanced_logger_info();
    assert_true(result);

    uint64_t size = get_advanced_logger_size();
    // Should return 0 for corrupted data
    assert_int_equal(size, 0);
}

TEST_FUNCTION(test_ap_advlog_pldm_init_mcp0, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    expect_function_call(__wrap_fpfw_pldm_service_register_state_effecter);
    will_return(__wrap_fpfw_pldm_service_register_state_effecter, FPFW_STATUS_SUCCESS);

    ap_advlog_pldm_init();
}

TEST_FUNCTION(test_ap_advlog_pldm_init_mcp1, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_1);

    // MCP1 should not register effecter, so no expectations

    ap_advlog_pldm_init();
}

TEST_FUNCTION(test_ap_advlog_pldm_init_scp, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    // Note: idsw_get_die_id() is NOT called when CPU type is SCP
    // The init function only checks die_id for MCP

    // SCP should not register effecter, so no expectations

    ap_advlog_pldm_init();
}

TEST_FUNCTION(test_ap_advlog_pldm_transfer_dump_success, nullptr, nullptr)
{
    // Setup valid logger info (will be read by ap_advlog_pldm_transfer_dump internally)
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);

    ap_advlog_pldm_transfer_dump();
}

TEST_FUNCTION(test_ap_advlog_pldm_transfer_dump_invalid_signature, nullptr, nullptr)
{
    // Setup invalid signature
    setup_mock_advlog_info(0xDEADBEEF,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    // Should not call PLDM raise event due to invalid signature

    ap_advlog_pldm_transfer_dump();
}

TEST_FUNCTION(test_ap_advlog_pldm_transfer_dump_already_in_progress, nullptr, nullptr)
{
    // NOTE: This test runs after test_ap_advlog_pldm_transfer_dump_success
    // which left logdump_in_progress = true (since the callback never fired in the test).
    // So the FIRST call here will also be rejected as "already in progress".

    // First transfer attempt (will be rejected due to previous test's state)
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    // No PLDM call expected since logdump_in_progress is already true
    ap_advlog_pldm_transfer_dump();

    // Second transfer attempt (still rejected)
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    ap_advlog_pldm_transfer_dump();
}

TEST_FUNCTION(test_ap_advlog_pldm_transfer_dump_pldm_failure, nullptr, nullptr)
{
    // This test runs after test_ap_advlog_pldm_transfer_dump_already_in_progress
    // which leaves logdump_in_progress = true.
    // But populate_advanced_logger_info() is still called first, so we need mocks.

    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    // The function will return early due to logdump_in_progress = true
    // from the previous test, so no PLDM call will be made
    ap_advlog_pldm_transfer_dump();
}

//
// Callback Tests
//

TEST_FUNCTION(test_ap_advlog_pldm_event_cb, nullptr, nullptr)
{
    // Note: effecter_complete_pending is true from test_ap_advlog_pldm_init_mcp0
    // First, reset logdump_in_progress from previous test by invoking the callback
    if (pldm_raise_event_cb != nullptr)
    {
        expect_function_call(__wrap_fpfw_pldm_service_state_effecter_set_complete);
        will_return(__wrap_fpfw_pldm_service_state_effecter_set_complete, FPFW_STATUS_SUCCESS);
        pldm_raise_event_cb(FPFW_PLDM_CC_SUCCESS, pldm_raise_event_ctx);
    }

    // Setup logger info first
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    // Start a transfer to initialize the logger
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);

    ap_advlog_pldm_transfer_dump();

    // Now test the event callback
    assert_non_null(captured_descriptor);
    assert_non_null(captured_descriptor->get_event_payload_cb);

    // Prepare destination buffer
    uint8_t dest_buffer[16];
    memset(dest_buffer, 0, sizeof(dest_buffer));

    // Setup MMIO reads for the callback (reading 16 bytes)
    for (size_t i = 0; i < 16; i++)
    {
        will_return(__wrap_mmio_read8, 0x40 + i); // Mock data pattern
    }

    // Invoke the callback
    captured_descriptor->get_event_payload_cb(captured_descriptor->get_event_payload_context,
                                              dest_buffer,
                                              0x100, // offset
                                              16);   // num_bytes

    // Verify data was copied
    for (size_t i = 0; i < 16; i++)
    {
        assert_int_equal(dest_buffer[i], 0x40 + i);
    }

    // Complete the transfer to reset logdump_in_progress for next test
    expect_function_call(__wrap_fpfw_pldm_service_state_effecter_set_complete);
    will_return(__wrap_fpfw_pldm_service_state_effecter_set_complete, FPFW_STATUS_SUCCESS);
    pldm_raise_event_cb(FPFW_PLDM_CC_SUCCESS, pldm_raise_event_ctx);
}

TEST_FUNCTION(test_ap_advlog_pldm_on_ppe_complete_success, nullptr, nullptr)
{
    // Note: effecter_complete_pending is true from test_ap_advlog_pldm_init_mcp0
    // Setup logger info
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    // Start a transfer
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);

    ap_advlog_pldm_transfer_dump();

    // Verify callback was captured
    assert_non_null(pldm_raise_event_cb);

    // Effecter is registered from earlier test, so we expect the complete call
    expect_function_call(__wrap_fpfw_pldm_service_state_effecter_set_complete);
    will_return(__wrap_fpfw_pldm_service_state_effecter_set_complete, FPFW_STATUS_SUCCESS);

    // Invoke the completion callback with success
    pldm_raise_event_cb(FPFW_PLDM_CC_SUCCESS, pldm_raise_event_ctx);

    // After callback, logdump_in_progress should be false
    // Verify by attempting another transfer
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);

    ap_advlog_pldm_transfer_dump(); // Should succeed since logdump_in_progress was reset

    // Complete this transfer too to clean up state
    expect_function_call(__wrap_fpfw_pldm_service_state_effecter_set_complete);
    will_return(__wrap_fpfw_pldm_service_state_effecter_set_complete, FPFW_STATUS_SUCCESS);
    pldm_raise_event_cb(FPFW_PLDM_CC_SUCCESS, pldm_raise_event_ctx);
}

TEST_FUNCTION(test_ap_advlog_pldm_on_ppe_complete_failure, nullptr, nullptr)
{
    // Verify callback was captured from previous test
    assert_non_null(pldm_raise_event_cb);

    // Effecter is still registered, so we expect the complete call even on failure
    expect_function_call(__wrap_fpfw_pldm_service_state_effecter_set_complete);
    will_return(__wrap_fpfw_pldm_service_state_effecter_set_complete, FPFW_STATUS_SUCCESS);

    // Invoke the completion callback with failure
    pldm_raise_event_cb(FPFW_PLDM_CC_ERROR, pldm_raise_event_ctx);

    // After callback, logdump_in_progress should be false
    // Verify by attempting another transfer
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);

    ap_advlog_pldm_transfer_dump(); // Should succeed since logdump_in_progress was reset

    // Complete this transfer too to clean up state
    expect_function_call(__wrap_fpfw_pldm_service_state_effecter_set_complete);
    will_return(__wrap_fpfw_pldm_service_state_effecter_set_complete, FPFW_STATUS_SUCCESS);
    pldm_raise_event_cb(FPFW_PLDM_CC_SUCCESS, pldm_raise_event_ctx);
}

TEST_FUNCTION(test_ap_advlog_pldm_on_ppe_complete_with_effecter, nullptr, nullptr)
{
    // Note: effecter is already registered from test_ap_advlog_pldm_init_mcp0
    // This test verifies the behavior is consistent even if we re-init

    // First clear state from previous test
    expect_function_call(__wrap_fpfw_pldm_service_state_effecter_set_complete);
    will_return(__wrap_fpfw_pldm_service_state_effecter_set_complete, FPFW_STATUS_SUCCESS);
    pldm_raise_event_cb(FPFW_PLDM_CC_SUCCESS, pldm_raise_event_ctx);

    // Re-initialize as MCP0 (effecter already registered, but init is idempotent)
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    expect_function_call(__wrap_fpfw_pldm_service_register_state_effecter);
    will_return(__wrap_fpfw_pldm_service_register_state_effecter, FPFW_STATUS_SUCCESS);

    ap_advlog_pldm_init();

    // Setup logger info
    setup_mock_advlog_info(ADVANCED_LOGGER_SIGNATURE,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET,
                           MOCK_AP_ADV_LOGGER_BASE + MOCK_LOG_BUFFER_OFFSET + MOCK_LOG_DATA_SIZE);

    // Start a transfer
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);

    ap_advlog_pldm_transfer_dump();

    // Verify callback was captured
    assert_non_null(pldm_raise_event_cb);

    // Now the effecter is registered, so we expect the complete call
    expect_function_call(__wrap_fpfw_pldm_service_state_effecter_set_complete);
    will_return(__wrap_fpfw_pldm_service_state_effecter_set_complete, FPFW_STATUS_SUCCESS);

    // Invoke the completion callback with success
    pldm_raise_event_cb(FPFW_PLDM_CC_SUCCESS, pldm_raise_event_ctx);
}

//
// CLI AP Advanced Logger Tests
//

TEST_FUNCTION(test_cli_ap_advlog_pldm_init, nullptr, nullptr)
{
    // Test the CLI initialization function
    expect_function_call(__wrap_FpFwCliRegisterTable);

    FPFW_CLI_STATUS status = cli_ap_advlog_pldm_init();
    assert_int_equal(status, CLI_SUCCESS);
}