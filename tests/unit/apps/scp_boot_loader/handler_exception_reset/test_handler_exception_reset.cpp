//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_handler_exception_reset.cpp
 * arch reset handler unit test code for SCP
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <kingsgate_boot.h>

/*------------- Typedefs -----------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/
extern void arch_exception_reset(void);

/*-- Declarations (Statics and globals) --*/
kingsgate_boot_metadata_t test_meta_data_t;
void* SCP_MSCP_EXP_SRAM0_ADDR = &test_meta_data_t;

extern const uint32_t SCP_TOP_SCP_INST_RAM_SIZE = (512 * 1024);
extern const uint32_t SCP_TOP_SCP_DATA_RAM_SIZE = (512 * 1024);

uint8_t test_itc_ram[SCP_TOP_SCP_INST_RAM_SIZE];
uint8_t test_dtc_ram[SCP_TOP_SCP_DATA_RAM_SIZE];

static uint8_t cmp_itc_ram[SCP_TOP_SCP_INST_RAM_SIZE] = {0};
static uint8_t cmp_dtc_ram[SCP_TOP_SCP_DATA_RAM_SIZE] = {0};

uint8_t* SCP_TOP_SCP_INST_RAM_ADDRESS = &test_itc_ram[0];
uint8_t* SCP_TOP_SCP_DATA_RAM_ADDRESS = &test_dtc_ram[0];

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap__start()
{
    return;
}
//
// Tests
//
TEST_FUNCTION(test_arch_exception_reset_cold_boot, nullptr, nullptr)
{
    test_meta_data_t.ResetReason &= ~(BITMASK_WARM_BOOT);

    memset(test_itc_ram, 0xAB, SCP_TOP_SCP_INST_RAM_SIZE);
    memset(test_dtc_ram, 0xCD, SCP_TOP_SCP_DATA_RAM_SIZE);

    memset(cmp_itc_ram, 0x0, SCP_TOP_SCP_INST_RAM_SIZE);
    memset(cmp_dtc_ram, 0x0, SCP_TOP_SCP_DATA_RAM_SIZE);

    // Call API under test
    arch_exception_reset();

    // Both ITC RAM and DTC RAM should be cleared
    assert_true(memcmp(test_itc_ram, cmp_itc_ram, SCP_TOP_SCP_INST_RAM_SIZE) == 0);
    assert_true(memcmp(test_dtc_ram, cmp_dtc_ram, SCP_TOP_SCP_DATA_RAM_SIZE) == 0);
}

TEST_FUNCTION(test_arch_exception_reset_warm_boot, nullptr, nullptr)
{

    test_meta_data_t.ResetReason |= BITMASK_WARM_BOOT;

    memset(test_itc_ram, 0xAB, SCP_TOP_SCP_INST_RAM_SIZE);
    memset(test_dtc_ram, 0xCD, SCP_TOP_SCP_DATA_RAM_SIZE);

    memset(cmp_itc_ram, 0x0, SCP_TOP_SCP_INST_RAM_SIZE);
    memset(cmp_dtc_ram, 0xCD, SCP_TOP_SCP_DATA_RAM_SIZE);

    // Call API under test
    arch_exception_reset();

    // Only ITC RAM should be cleared and not DTC RAM
    assert_true(memcmp(test_itc_ram, cmp_itc_ram, SCP_TOP_SCP_INST_RAM_SIZE) == 0);

    assert_true(memcmp(test_dtc_ram, cmp_dtc_ram, SCP_TOP_SCP_DATA_RAM_SIZE) == 0);
}
}