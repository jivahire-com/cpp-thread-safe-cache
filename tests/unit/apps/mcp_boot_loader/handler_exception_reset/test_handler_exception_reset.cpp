//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_handler_exception_reset.cpp
 * arch reset handler unit test code for MCP
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>    // for assert_true, CmockaWrapperTest, TEST_F...
#include <cstdint>            // for uint8_t, uint32_t
#include <string.h>           // for memset
#include <vcruntime_string.h> // for memcmp

extern "C" {
#include <kingsgate_boot.h> // for BITMASK_WARM_BOOT, kingsgate_boot_meta...

/*------------- Typedefs -----------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/
extern void arch_exception_reset(void);
extern void disable_mscp_watchdog(void);

/*-- Declarations (Statics and globals) --*/
kingsgate_boot_metadata_t test_meta_data_t;

void* MCP_MSCP_EXP_SRAM0_ADDR = &test_meta_data_t;

extern const uint32_t MCP_TOP_MCP_INST_RAM_SIZE = (512 * 1024);
extern const uint32_t MCP_TOP_MCP_DATA_RAM_SIZE = (512 * 1024);

uint8_t test_itc_ram[MCP_TOP_MCP_INST_RAM_SIZE];
uint8_t test_dtc_ram[MCP_TOP_MCP_DATA_RAM_SIZE];

static uint8_t cmp_itc_ram[MCP_TOP_MCP_INST_RAM_SIZE] = {0};
static uint8_t cmp_dtc_ram[MCP_TOP_MCP_DATA_RAM_SIZE] = {0};

uint8_t* MCP_TOP_MCP_INST_RAM_ADDRESS = &test_itc_ram[0];
uint8_t* MCP_TOP_MCP_DATA_RAM_ADDRESS = &test_dtc_ram[0];

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap__start()
{
    return;
}

void __wrap_wdog_cmsdk_apb_disable()
{
    function_called();
}

void __wrap_wdog_cmsdk_apb_lock_unlock(bool lock)
{
    check_expected(lock);
    function_called();
}
//
// Tests
//
TEST_FUNCTION(test_arch_exception_reset_cold_boot, nullptr, nullptr)
{
    test_meta_data_t.reset_reason &= ~(BITMASK_WARM_BOOT);

    memset(test_itc_ram, 0xAB, MCP_TOP_MCP_INST_RAM_SIZE);
    memset(test_dtc_ram, 0xCD, MCP_TOP_MCP_DATA_RAM_SIZE);

    memset(cmp_itc_ram, 0x0, MCP_TOP_MCP_INST_RAM_SIZE);
    memset(cmp_dtc_ram, 0x0, MCP_TOP_MCP_DATA_RAM_SIZE);

    // Call API under test
    arch_exception_reset();

    // Both ITC RAM and DTC RAM should be cleared
    assert_true(memcmp(test_itc_ram, cmp_itc_ram, MCP_TOP_MCP_INST_RAM_SIZE) == 0);
    assert_true(memcmp(test_dtc_ram, cmp_dtc_ram, MCP_TOP_MCP_DATA_RAM_SIZE) == 0);
}

TEST_FUNCTION(test_arch_exception_reset_warm_boot, nullptr, nullptr)
{

    test_meta_data_t.reset_reason |= BITMASK_WARM_BOOT;

    memset(test_itc_ram, 0xAB, MCP_TOP_MCP_INST_RAM_SIZE);
    memset(test_dtc_ram, 0xCD, MCP_TOP_MCP_DATA_RAM_SIZE);

    memset(cmp_itc_ram, 0x0, MCP_TOP_MCP_INST_RAM_SIZE);
    memset(cmp_dtc_ram, 0xCD, MCP_TOP_MCP_DATA_RAM_SIZE);

    // Call API under test
    arch_exception_reset();

    // Only ITC RAM should be cleared and not DTC RAM
    assert_true(memcmp(test_itc_ram, cmp_itc_ram, MCP_TOP_MCP_INST_RAM_SIZE) == 0);
    assert_true(memcmp(test_dtc_ram, cmp_dtc_ram, MCP_TOP_MCP_DATA_RAM_SIZE) == 0);
}

TEST_FUNCTION(test_nmi_disables_watchdog, nullptr, nullptr)
{
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, false);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    disable_mscp_watchdog();
}
}