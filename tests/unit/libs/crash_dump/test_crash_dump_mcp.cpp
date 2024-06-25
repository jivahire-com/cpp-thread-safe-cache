//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump_mcp.cpp
 * Crash dump tests for MCP
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, expect_function_...
#include <cstdint>         // for uint32_t, uint64_t

extern "C" {
#include <../src/crash_dump_gpio.h>   // for cd_gpio_assert_cd_available, cd_gpio_as...
#include <cmsis_m7.h>                 // for SCB
#include <modules/CdDumpDescriptor.h> // for _FPFwCdDumpPriority
#include <stddef.h>                   // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
typedef struct _gpio_test_data_t
{
    bool input;
    uint32_t expected_level;
} gpio_test_data_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Expectations
//
void set_expectations_gpio_set_output()
{
    expect_function_call(__wrap_gpio_set_output);
    expect_value(__wrap_gpio_set_output, gpio_pin_id, 0x0602); // MSCP_EXP_GPIO_6 | SAFE_MODE_REQ (2)
    expect_value(__wrap_gpio_set_output, level, 1);
}

void set_expectations_crash_dump_register_default_registers()
{
    // ToDo: Add more register checks for SCP_EXP, Watchdog and Power control registers
    expect_function_call(__wrap_CdRegisterMMIORegisterSet); // SCB->MMFAR
    expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, &SCB->MMFAR);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, 1);
    expect_value(__wrap_CdRegisterMMIORegisterSet, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    expect_function_call(__wrap_CdRegisterMMIORegisterSet); // SCB->BFAR
    expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, &SCB->BFAR);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, 1);
    expect_value(__wrap_CdRegisterMMIORegisterSet, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    expect_function_call(__wrap_CdRegisterMMIORegisterSet); // SCB->HFSR
    expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, &SCB->HFSR);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, 1);
    expect_value(__wrap_CdRegisterMMIORegisterSet, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    expect_function_call(__wrap_CdRegisterMMIORegisterSet); // SCB->CFSR
    expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, &SCB->CFSR);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, 1);
    expect_value(__wrap_CdRegisterMMIORegisterSet, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

//
// Tests
//
TEST_FUNCTION(test_crash_dump_in_progress_assert, nullptr, nullptr)
{
    gpio_test_data_t test_data[] = {{
                                        .input = true,
                                        .expected_level = 0,
                                    },
                                    {
                                        .input = false,
                                        .expected_level = 1,
                                    }};

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)
    {
        // Set up expectations
        expect_function_call(__wrap_gpio_set_output);
        expect_value(__wrap_gpio_set_output, gpio_pin_id, 0x0602); // MSCP_EXP_GPIO_6 | SAFE_MODE_REQ (2)
        expect_value(__wrap_gpio_set_output, level, test_data[i].expected_level);

        // Call API under test
        cd_gpio_assert_cd_in_progress(test_data[i].input);
    }
}

TEST_FUNCTION(test_crash_dump_available_assert, nullptr, nullptr)
{
    gpio_test_data_t test_data[] = {{
                                        .input = true,
                                        .expected_level = 0,
                                    },
                                    {
                                        .input = false,
                                        .expected_level = 1,
                                    }};

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)
    {
        // Set up expectations
        expect_function_call(__wrap_gpio_set_output);
        expect_value(__wrap_gpio_set_output, gpio_pin_id, 0x0603); // MSCP_EXP_GPIO_6 | GPIO_CD_AVAILABLE (3)
        expect_value(__wrap_gpio_set_output, level, test_data[i].expected_level);

        // Call API under test
        cd_gpio_assert_cd_available(test_data[i].input);
    }
}
}