//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Unit tests for gtimer initialization.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <fpfw_init.h>
#include <gtimer.h>
#include <idsw.h>
#include <idsw_kng.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_gtimer;

/*------------- Functions ----------------*/
/* Mocks */
KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

void __wrap_gtimer_init(uintptr_t counter_control_base, uint32_t frequency_hz, uint8_t scaling_factor)
{
    check_expected(counter_control_base);
    check_expected(frequency_hz);
    check_expected(scaling_factor);
}

/* Tests */
TEST_FUNCTION(test_gtimer_init_soc, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    expect_value(__wrap_gtimer_init, counter_control_base, SCP_TOP_GEN_CNTR_CTRL_ADDRESS);
    /* Has to be 250 MHz */
    expect_value(__wrap_gtimer_init, frequency_hz, (250 * 1000 * 1000));
    /* Scaling factor must be 4 */
    expect_value(__wrap_gtimer_init, scaling_factor, 4);

    _fpfw_component_gtimer.init_fn();
}

TEST_FUNCTION(test_gtimer_init_fpga, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);

    expect_value(__wrap_gtimer_init, counter_control_base, SCP_TOP_GEN_CNTR_CTRL_ADDRESS);
    /* Has to be 10 MHz */
    expect_value(__wrap_gtimer_init, frequency_hz, (10 * 1000 * 1000));
    /* Scaling factor on FPGA is being set to 1 */
    expect_value(__wrap_gtimer_init, scaling_factor, 1);

    _fpfw_component_gtimer.init_fn();
}

} /* extern C */
