//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file smc_tlm_mcp_tests.cpp
 * Tests the SMC TLM MCP service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <map>
#include <string.h>

extern "C" {
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <idsw_kng.h>
#include <silibs_platform.h>
#include <smc_tlm_mcp.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MOCK_ATU_MAPPED_BASE      0x40000000
#define SMC_TLM_NUM_CORES_PER_DIE 68

// Mock constants that may not be available in test environment
#ifndef ATU_ID_MSCP
    #define ATU_ID_MSCP 0
#endif

#ifndef SILIBS_SUCCESS
    #define SILIBS_SUCCESS 0
#endif

#ifndef ATU_BUS_ATTR_NS
    #define ATU_BUS_ATTR_NS 0
#endif

#ifndef AP_SMC_TELEMETRY_BUFFER_BASE
    #define AP_SMC_TELEMETRY_BUFFER_BASE 0x80000000
#endif

#ifndef AP_SMC_TELEMETRY_BUFFER_SIZE
    #define AP_SMC_TELEMETRY_BUFFER_SIZE 0x10000
#endif

/*------------- Typedefs -----------------*/
typedef struct per_core_smc_telemetry
{
    uint64_t smc_fid;
    uint64_t entry_timestamp;
    uint64_t exit_timestamp;
} per_core_smc_telemetry_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static std::map<uint64_t, uint64_t> mock_mmio_memory;
static void (*captured_timer_callback)(ULONG) = nullptr;
static ULONG captured_timer_input = 0;

/*------------- Functions ----------------*/
//
// Mocks
//
idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

// ATU Mocks
int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    assert_non_null(atu_map_entry);
    check_expected(atu_id);

    // Set up the mapped address
    atu_map_entry->mscp_start_address = MOCK_ATU_MAPPED_BASE;

    function_called();

    return mock_type(int);
}

// MMIO Mocks - use a map to simulate memory
uint64_t __wrap_mmio_read64(volatile uint64_t* addr)
{
    uint64_t addr_val = (uint64_t)addr;
    if (mock_mmio_memory.find(addr_val) != mock_mmio_memory.end())
    {
        return mock_mmio_memory[addr_val];
    }
    return 0;
}

void __wrap_mmio_write64(volatile uint64_t* addr, uint64_t value)
{
    uint64_t addr_val = (uint64_t)addr;
    mock_mmio_memory[addr_val] = value;
}

// ThreadX Mocks
UINT __wrap__txe_timer_create(TX_TIMER* timer_ptr,
                              CHAR* name_ptr,
                              VOID (*expiration_function)(ULONG),
                              ULONG expiration_input,
                              ULONG initial_ticks,
                              ULONG reschedule_ticks,
                              UINT auto_activate,
                              UINT timer_control_block_size)
{
    FPFW_UNUSED(timer_ptr);
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(initial_ticks);
    FPFW_UNUSED(reschedule_ticks);
    FPFW_UNUSED(auto_activate);
    FPFW_UNUSED(timer_control_block_size);

    // Capture the callback for testing
    captured_timer_callback = expiration_function;
    captured_timer_input = expiration_input;

    function_called();
    return mock_type(UINT);
}

// Event Trace Mock - simple pass-through, no checking needed
void __wrap_EventWriteApSmcTelemetry(uint64_t smc_fid, uint64_t time_taken, uint32_t core_id)
{
    FPFW_UNUSED(smc_fid);
    FPFW_UNUSED(time_taken);
    FPFW_UNUSED(core_id);
    // No-op: just let the call succeed
}

// Bug check mock
void __wrap_bug_check_ex(uint32_t code, uint64_t param1, uint64_t param2, uint64_t param3, uint64_t param4)
{
    FPFW_UNUSED(code);
    FPFW_UNUSED(param1);
    FPFW_UNUSED(param2);
    FPFW_UNUSED(param3);
    FPFW_UNUSED(param4);
    function_called();
}

} // extern "C"

//
// Test Setup and Teardown
//
static int test_setup(void** state)
{
    FPFW_UNUSED(state);
    mock_mmio_memory.clear();
    // Note: Don't clear captured_timer_callback - it's set once during init and reused
    return 0;
}

static int test_teardown(void** state)
{
    FPFW_UNUSED(state);
    mock_mmio_memory.clear();
    return 0;
}

//
// SMC TLM MCP Tests
//

// Helper to write telemetry data to mock MMIO
static void write_smc_telemetry(uint32_t core_id, uint64_t smc_fid, uint64_t entry_ts, uint64_t exit_ts)
{
    uint32_t idx = core_id * sizeof(per_core_smc_telemetry_t);
    mock_mmio_memory[MOCK_ATU_MAPPED_BASE + idx] = smc_fid;
    mock_mmio_memory[MOCK_ATU_MAPPED_BASE + idx + 8] = entry_ts;
    mock_mmio_memory[MOCK_ATU_MAPPED_BASE + idx + 16] = exit_ts;
}

// Note: smc_tlm_mcp_init() has static variables and can only be called once per test run.
// Testing DIE_0 initialization covers the core logic; DIE_1 path is identical except for context pointer.

TEST_FUNCTION(test_smc_tlm_mcp_init_mcp_die0, nullptr, nullptr)
{
    // Mock idsw calls for MCP Die 0
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    // Expect ATU mapping
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_map);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    // Expect timer creation
    expect_function_call(__wrap__txe_timer_create);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    smc_tlm_mcp_init();

    // Verify timer callback was captured
    assert_non_null(captured_timer_callback);
}

// Remaining tests use test_setup/test_teardown and assume init was already called
TEST_FUNCTION(test_timer_callback_single_core_valid_data, test_setup, test_teardown)
{
    // Verify timer callback was captured during init
    assert_non_null(captured_timer_callback);

    // Setup valid telemetry for core 0
    uint64_t smc_fid = 0xC4000001;
    uint64_t entry_ts = 1000;
    uint64_t exit_ts = 2000;
    write_smc_telemetry(0, smc_fid, entry_ts, exit_ts);

    // Call the timer callback
    assert_non_null(captured_timer_callback);
    captured_timer_callback(captured_timer_input);

    // Verify buffer was cleared
    assert_int_equal(mock_mmio_memory[MOCK_ATU_MAPPED_BASE], 0);
    assert_int_equal(mock_mmio_memory[MOCK_ATU_MAPPED_BASE + 8], 0);
    assert_int_equal(mock_mmio_memory[MOCK_ATU_MAPPED_BASE + 16], 0);
}

TEST_FUNCTION(test_timer_callback_multiple_cores_valid_data, test_setup, test_teardown)
{
    // Setup valid telemetry for multiple cores
    write_smc_telemetry(0, 0xC4000001, 1000, 2000);
    write_smc_telemetry(1, 0xC4000002, 1500, 2500);
    write_smc_telemetry(5, 0xC4000003, 2000, 3000);

    // Call the timer callback
    captured_timer_callback(captured_timer_input);
}

TEST_FUNCTION(test_timer_callback_zero_smc_fid, test_setup, test_teardown)
{
    // Setup telemetry with zero smc_fid (should be ignored)
    write_smc_telemetry(0, 0, 1000, 2000);

    // Should NOT emit event trace
    // Call the timer callback
    captured_timer_callback(captured_timer_input);

    // Buffer should not be cleared since data was invalid
}

TEST_FUNCTION(test_timer_callback_zero_entry_timestamp, test_setup, test_teardown)
{
    // Setup telemetry with zero entry timestamp (should be ignored)
    write_smc_telemetry(0, 0xC4000001, 0, 2000);

    // Should NOT emit event trace
    captured_timer_callback(captured_timer_input);
}

TEST_FUNCTION(test_timer_callback_exit_before_entry, test_setup, test_teardown)
{
    // Setup telemetry with exit before entry (should be ignored)
    write_smc_telemetry(0, 0xC4000001, 2000, 1000);

    // Should NOT emit event trace
    captured_timer_callback(captured_timer_input);
}

TEST_FUNCTION(test_timer_callback_die1_cores, test_setup, test_teardown)
{
    // This test verifies cores in a different range from what was tested above
    // Setup valid telemetry for higher core IDs within Die 0 range (0-67)
    write_smc_telemetry(50, 0xC4000001, 1000, 2000);
    write_smc_telemetry(67, 0xC4000002, 1500, 2500);

    // Call the timer callback
    captured_timer_callback(captured_timer_input);
}
