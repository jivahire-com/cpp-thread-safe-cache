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
#include <gtimer_prodfw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
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

void __wrap_gtimer_prodfw_init(gtimer_prodfw_init_config_t* config)
{
    check_expected_ptr(config);
}

/* Tests */
TEST_FUNCTION(test_gtimer_init_soc, NULL, NULL)
{

    gtimer_prodfw_init_config_t test_config = {
        .counter_control_base = SCP_TOP_GEN_CNTR_CTRL_ADDRESS,
        .timer_control_base = SCP_TOP_SCP_TIMER_CTRL_ADDRESS,
        .timer_base_address = SCP_TOP_SCP_TIMER_BASE_ADDRESS,
        .frequency_hz = (250 * 1000 * 1000),
        .scaling_factor = 4,
        .timer_irq = HW_INT_SCP_GENERIC_TIMER_INT,
    };
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    expect_memory(__wrap_gtimer_prodfw_init, config, &test_config, sizeof(gtimer_prodfw_init_config_t));

    _fpfw_component_gtimer.init_fn();
}

TEST_FUNCTION(test_gtimer_init_fpga, NULL, NULL)
{
    gtimer_prodfw_init_config_t test_config = {
        .counter_control_base = SCP_TOP_GEN_CNTR_CTRL_ADDRESS,
        .timer_control_base = SCP_TOP_SCP_TIMER_CTRL_ADDRESS,
        .timer_base_address = SCP_TOP_SCP_TIMER_BASE_ADDRESS,
        .frequency_hz = (10 * 1000 * 1000),
        .scaling_factor = 1,
        .timer_irq = HW_INT_SCP_GENERIC_TIMER_INT,
    };
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);

    expect_memory(__wrap_gtimer_prodfw_init, config, &test_config, sizeof(gtimer_prodfw_init_config_t));

    _fpfw_component_gtimer.init_fn();
}

} /* extern C */
