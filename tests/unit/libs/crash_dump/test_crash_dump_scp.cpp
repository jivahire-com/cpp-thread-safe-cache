//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump_scp.cpp
 * Crash dump tests for SCP
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>            // for expect_function_call, expect_n...
#include <modules/CdDumpDescriptor.h> // for _FPFwCdDumpPriority

extern "C" {
#include <cmsis_m7.h> // for SCB

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Expectations
//
void set_expectations_gpio_set_output()
{
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
}