//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_polling_tests.cpp
 * DDR Manager Polling tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include "ddr_mocks.h"

#include <FpFwUtils.h>
#include <ddr_i3c.h>
#include <ddr_manager_bwl.h>
#include <ddr_manager_events.h>
#include <ddr_manager_i.h> // for ddr_poll_dimms, ddr_worker_thread_func
#include <ddrss_intu.h>
#include <ddrss_lib.h>
#include <error_handler.h> // for set_error_handler_return
#include <idhw.h>
#include <interrupts.h>
} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
#define NUM_DIMM_PER_DIE     (6)                    // 6 DIMMs
#define NUM_SENSORS_PER_DIMM (NUM_DIMM_PER_DIE * 2) // 6 DIMMs, 2 sensors each
/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Fixtures
//
static int setup_disengaged_all(void** state)
{
    in_setup_teardown = true;
    FPFW_UNUSED(state);

    ddr_manager_disable_bwl_i3c();
    ddr_manager_disable_bwl_force();
    ddr_manager_disable_bwl_mr4();

    in_setup_teardown = false;
    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_ddr_manager_poll_below_high_to_low_thresh, setup_disengaged_all, setup_disengaged_all)
{
    // Arrange enable_bwl_i3c()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);
    will_return(__wrap_gtimer_prodfw_get_counter, 300);

    // Arrange disable_bwl_i3c()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);
    will_return(__wrap_gtimer_prodfw_get_counter, 300);

    // High temp (> 85C): 88
    for (uint8_t sens_idx = 0; sens_idx < NUM_SENSORS_PER_DIMM; sens_idx++)
    {
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, 1);

        // Low byte 8
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT7));
        // High byte 16 + 64 = 80
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);
    }

    // Act on 88 degrees C
    ddr_read_dimm_temperatures();
    check_dimm_temp_thresholds();
    assert_true(ddr_manager_get_bwl_engaged());

    // Low temp
    for (uint8_t sens_idx = 0; sens_idx < NUM_SENSORS_PER_DIMM; sens_idx++)
    {
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, 1);

        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT4 | BIT5));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT2));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);
    }

    // Act on 67 degrees C
    ddr_read_dimm_temperatures();
    check_dimm_temp_thresholds();
    assert_false(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_poll_crit_thresh, NULL, NULL)
{
    // Crit temp
    for (uint8_t sens_idx = 0; sens_idx < NUM_SENSORS_PER_DIMM; sens_idx++)
    {
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, 1);

        // Low: 1+2+4+8 = 15
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT4 | BIT5 | BIT6 | BIT7));
        // High: 16+32+64+128 = 240
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1 | BIT2 | BIT3));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);
    }

    // Expect a GPIO Read/Write for critical temperature
    expect_value(__wrap_prod_ddrss_get_intr_event_cper, mc, 0);
    expect_value(__wrap_prod_ddrss_get_intr_event_cper, intr_event, DDRSS_INTU_MC_MEDIAREFTEMPCHANGED);
    will_return(__wrap_prod_ddrss_get_intr_event_cper, SILIBS_SUCCESS);

    expect_value(__wrap_hm_submit_cper, error_domain_idx, ACPI_ERROR_DOMAIN_DDR);
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_function_call(__wrap_hm_submit_cper);

    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);

    // Act
    ddr_read_dimm_temperatures();

    expect_value(FPFwErrorRaise, error, (uint32_t)DDR_MANAGER_ET_TYPE_DIMM_EXCEEDED_CRITICAL_TEMPERATURE_THRESHOLD); // DDR_MANAGER_ET_TYPE_DIMM_EXCEEDED_CRITICAL_TEMPERATURE_THRESHOLD
    if (!set_error_handler_return())
    {
        check_dimm_temp_thresholds();
    }
}

TEST_FUNCTION(test_poll_ecc_ce_errors_already_enabled, NULL, NULL)
{
    // Arrange Die0
    KNG_DIE_ID die_num = DIE_0;
    uint32_t ddrss_mask = 0xFFF; // DDRSS 0 - 11 present (all)
    g_should_wrap_ddrss_get_ddrss_mask = true;
    uint32_t base_mc = (die_num == DIE_1) ? DDRSS_MAX_MC_NUM_PER_DIE : 0;

    will_return(__wrap_idsw_get_die_id, die_num);
    will_return_always(__wrap_ddrss_get_ddrss_mask, ddrss_mask);

    for (uint32_t mc = base_mc; mc < base_mc + DDRSS_MAX_MC_NUM_PER_DIE; mc++)
    {
        if (!(ddrss_mask & (1 << DDRSS_MC_TO_DDRSS(mc))))
        {
            continue;
        }

        will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, true); // *enable
        will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, SILIBS_SUCCESS);
    }

    // Act
    ddr_poll_ecc_ce_errors();

    // Arrange Die1
    die_num = DIE_1;
    base_mc = (die_num == DIE_1) ? DDRSS_MAX_MC_NUM_PER_DIE : 0;
    will_return(__wrap_idsw_get_die_id, die_num);

    for (uint32_t mc = base_mc; mc < base_mc + DDRSS_MAX_MC_NUM_PER_DIE; mc++)
    {
        if (!(ddrss_mask & (1 << DDRSS_MC_TO_DDRSS(mc))))
        {
            continue;
        }

        will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, true); // *enable
        will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, SILIBS_SUCCESS);
    }

    // Act
    ddr_poll_ecc_ce_errors();

    // Cleanup
    g_should_wrap_ddrss_get_ddrss_mask = false;
}

TEST_FUNCTION(test_poll_ecc_ce_errors_one_mc_disabled, NULL, NULL)
{
    // Arrange Die0
    KNG_DIE_ID die_num = DIE_0;
    uint32_t ddrss_mask = 0xFFF; // DDRSS 0 - 11 present (all)
    g_should_wrap_ddrss_get_ddrss_mask = true;
    uint32_t base_mc = (die_num == DIE_1) ? DDRSS_MAX_MC_NUM_PER_DIE : 0;

    will_return(__wrap_idsw_get_die_id, die_num);
    will_return_always(__wrap_ddrss_get_ddrss_mask, ddrss_mask);

    for (uint32_t mc = base_mc; mc < base_mc + DDRSS_MAX_MC_NUM_PER_DIE; mc++)
    {
        if (!(ddrss_mask & (1 << DDRSS_MC_TO_DDRSS(mc))))
        {
            continue;
        }

        if (mc == 2) // MC2 RAS CE interrupt disabled
        {
            will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, false); // *enable
            will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, SILIBS_SUCCESS);

            expect_value(__wrap_FPFwCoreInterruptIsEnabled, irqnum, HW_INT_DDRSS0_COMBINED_SCP_INT + DDRSS_MC_TO_DDRSS(mc));
            will_return(__wrap_FPFwCoreInterruptIsEnabled, true); // ddr_irq_en

            // Since ddr_irq_en is true, expect Disable/Enable calls
            expect_value(__wrap_FPFwCoreInterruptDisableVector, irqnum, HW_INT_DDRSS0_COMBINED_SCP_INT + DDRSS_MC_TO_DDRSS(mc));
            will_return(__wrap_FPFwCoreInterruptDisableVector, 0); // return value not used

            expect_value(__wrap_prod_ddrss_set_ras_erg_ce_interrupt_enable, enable, true);
            will_return(__wrap_prod_ddrss_set_ras_erg_ce_interrupt_enable, SILIBS_SUCCESS);

            // Since ddr_irq_en is true, expect Disable/Enable calls
            expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, HW_INT_DDRSS0_COMBINED_SCP_INT + DDRSS_MC_TO_DDRSS(mc));
            will_return(__wrap_FPFwCoreInterruptEnableVector, 0); // return value not used
        }
        else
        {
            will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, true); // *enable
            will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, SILIBS_SUCCESS);
        }
    }

    // Act
    ddr_poll_ecc_ce_errors();

    // Arrange Die1
    die_num = DIE_1;
    base_mc = (die_num == DIE_1) ? DDRSS_MAX_MC_NUM_PER_DIE : 0;
    will_return(__wrap_idsw_get_die_id, die_num);

    for (uint32_t mc = base_mc; mc < base_mc + DDRSS_MAX_MC_NUM_PER_DIE; mc++)
    {
        if (!(ddrss_mask & (1 << DDRSS_MC_TO_DDRSS(mc))))
        {
            continue;
        }

        if (mc == 12) // MC2 RAS CE interrupt disabled
        {
            will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, false); // *enable
            will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, SILIBS_SUCCESS);

            expect_value(__wrap_FPFwCoreInterruptIsEnabled, irqnum, HW_INT_DDRSS0_COMBINED_SCP_INT + DDRSS_MC_TO_DDRSS(mc));
            will_return(__wrap_FPFwCoreInterruptIsEnabled, true); // ddr_irq_en

            // Since ddr_irq_en is true, expect Disable/Enable calls
            expect_value(__wrap_FPFwCoreInterruptDisableVector, irqnum, HW_INT_DDRSS0_COMBINED_SCP_INT + DDRSS_MC_TO_DDRSS(mc));
            will_return(__wrap_FPFwCoreInterruptDisableVector, 0); // return value not used

            expect_value(__wrap_prod_ddrss_set_ras_erg_ce_interrupt_enable, enable, true);
            will_return(__wrap_prod_ddrss_set_ras_erg_ce_interrupt_enable, SILIBS_SUCCESS);

            // Since ddr_irq_en is true, expect Disable/Enable calls
            expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, HW_INT_DDRSS0_COMBINED_SCP_INT + DDRSS_MC_TO_DDRSS(mc));
            will_return(__wrap_FPFwCoreInterruptEnableVector, 0); // return value not used
        }
        else
        {
            will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, true); // *enable
            will_return(__wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable, SILIBS_SUCCESS);
        }
    }

    // Act
    ddr_poll_ecc_ce_errors();

    // Cleanup
    g_should_wrap_ddrss_get_ddrss_mask = false;
}
