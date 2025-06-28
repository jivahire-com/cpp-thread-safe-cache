//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_bandgap.cpp
 * Bandgap tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h>
#include <bandgap.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <stddef.h>

/*-- Symbolic Constant Macros (defines) --*/
#define REF_MACRO_TRIM_REG_ADDR (0x15000e8)
#define REF_MACRO_CTRL_REG_ADDR (0x15000a4)
#define SLOPE_TRIM_TEST_VALUE   (0x0001)
#define OFFSET_TRIM_TEST_VALUE  (0x0002)
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected(addr);
    function_called();
    return mock_type(uint32_t);
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    check_expected(addr);
    check_expected(data);
    function_called();
}
}

TEST_FUNCTION(test_configure_bandgap_trim, NULL, NULL)
{
    scp_exp_csr_ref_macro_trim_reg ref_macro_trim = {{0}};
    ref_macro_trim.slope_trim = (uint32_t)SLOPE_TRIM_TEST_VALUE;
    ref_macro_trim.offset_trim = (uint32_t)OFFSET_TRIM_TEST_VALUE;

    expect_value(__wrap_mmio_write32, addr, (uint32_t)REF_MACRO_TRIM_REG_ADDR);
    expect_value(__wrap_mmio_write32, data, ref_macro_trim.as_uint32);
    expect_function_call(__wrap_mmio_write32);

    configure_bandgap_trim(SLOPE_TRIM_TEST_VALUE, OFFSET_TRIM_TEST_VALUE);
}

TEST_FUNCTION(enable_bandgap_circuitry, NULL, NULL)
{
    scp_exp_csr_ref_macro_ctrl_reg ref_macro_ctrl = {{0}};
    ref_macro_ctrl.enable = 1; // Enable the bandgap circuitry

    expect_value(__wrap_mmio_read32, addr, (uint32_t)REF_MACRO_CTRL_REG_ADDR);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    expect_value(__wrap_mmio_write32, addr, (uint32_t)REF_MACRO_CTRL_REG_ADDR);
    expect_value(__wrap_mmio_write32, data, ref_macro_ctrl.as_uint32);
    expect_function_call(__wrap_mmio_write32);

    enable_bandgap_circuitry(true);
}