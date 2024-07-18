//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Unit tests for the common VAB initialization library.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <atu_lib.h>
#include <cstddef>
#include <idsw_kng.h>
#include <interrupts.h>
#include <kng_soc_constants.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdint.h>

extern "C" {
#include <error_handler.h>
#include <vab.h>
#include <vab_irq.h>
#include <vab_regs.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint32_t vab_irq_nums[MAX_VAB_INSTANCES] = {
    HW_INT_VAB0_COMBINED_SCP_INT,
    HW_INT_VAB1_COMBINED_SCP_INT,
    HW_INT_VAB2_COMBINED_SCP_INT,
    HW_INT_VAB3_COMBINED_SCP_INT,
    HW_INT_VAB0_COMBINED_SCP_INT,
    HW_INT_VAB1_COMBINED_SCP_INT,
    HW_INT_VAB2_COMBINED_SCP_INT,
    HW_INT_VAB3_COMBINED_SCP_INT,
    HW_INT_VAB4_COMBINED_SCP_INT,
    HW_INT_VAB4_COMBINED_SCP_INT,
    HW_INT_VAB5_COMBINED_SCP_INT,
    HW_INT_VAB5_COMBINED_SCP_INT,
};

/*------------- Functions ----------------*/
TEST_FUNCTION(test_skip_all_vabs, NULL, NULL)
{

    int status;
    status = vab_common_init(0);
    assert_int_equal(status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_successful_init_all_vabs, NULL, NULL)
{

    // int status;
    uint16_t vabs_to_init = 0xfff;

    // Setup expectations for all VABs
    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);

            expect_value(__wrap_vab_init, vab_init_params->security_state, SECURITY_STATE_NON_SECURE);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->sh_cfg, 1);
            expect_value(__wrap_vab_init, vab_init_params->system_counter_delay, 0);
            expect_value(__wrap_vab_init, vab_init_params->vab_resolved_base_addr, 0x0);
            expect_value(__wrap_vab_init, vab_init_params->vab_configure_intu, true);

            expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

            expect_function_call(__wrap_vab_init);
        }
    }
    vab_common_init(vabs_to_init);
}

TEST_FUNCTION(test_atu_map_fail, NULL, NULL)
{
    uint16_t vabs_to_init = 0xfff;
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_E_INIT);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_common_init(vabs_to_init);
    }
}

TEST_FUNCTION(test_atu_unmap_fail, NULL, NULL)
{
    uint16_t vabs_to_init = 0x1;

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);

            expect_value(__wrap_vab_init, vab_init_params->security_state, SECURITY_STATE_NON_SECURE);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->sh_cfg, 1);
            expect_value(__wrap_vab_init, vab_init_params->system_counter_delay, 0);
            expect_value(__wrap_vab_init, vab_init_params->vab_resolved_base_addr, 0x0);
            expect_value(__wrap_vab_init, vab_init_params->vab_configure_intu, true);
            expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_unmap, SILIBS_E_INIT);

            expect_function_call(__wrap_vab_init);
        }
    }

    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_common_init(vabs_to_init);
    }
}

TEST_FUNCTION(test_rpss_vab_isr, NULL, NULL)
{
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB3_COMBINED_SCP_INT;
    mock_isr_ctx.intu0_base = VAB_VAB_FW_AGG0_ADDRESS;
    mock_isr_ctx.intu0_sts = VAB_RPSS_COMBINED_INT;
    mock_isr_ctx.intu1_base = VAB_VAB_FW_AGG1_ADDRESS;
    mock_isr_ctx.intu1_sts = 0;

    expect_value(__wrap_intu_get_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG0_ADDRESS);
    expect_value(__wrap_intu_get_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG1_ADDRESS);
    expect_value(__wrap_rpss_irq_callback, irq_num, HW_INT_VAB3_COMBINED_SCP_INT);
    expect_value(__wrap_intu_clear_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG0_ADDRESS);
    expect_value(__wrap_intu_clear_interrupt_status, intr_pin_mask, VAB_INTU_CLEAR_MASK);
    expect_value(__wrap_intu_clear_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG1_ADDRESS);
    expect_value(__wrap_intu_clear_interrupt_status, intr_pin_mask, VAB_INTU_CLEAR_MASK);
    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_vab_isr, NULL, NULL)
{
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    mock_isr_ctx.intu0_base = VAB_VAB_FW_AGG0_ADDRESS;
    mock_isr_ctx.intu0_sts = VAB_RPSS_COMBINED_INT;
    mock_isr_ctx.intu1_base = VAB_VAB_FW_AGG1_ADDRESS;
    mock_isr_ctx.intu0_sts = 0;

    expect_value(__wrap_intu_get_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG0_ADDRESS);
    expect_value(__wrap_intu_get_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG1_ADDRESS);
    expect_value(__wrap_intu_clear_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG0_ADDRESS);
    expect_value(__wrap_intu_clear_interrupt_status, intr_pin_mask, VAB_INTU_CLEAR_MASK);
    expect_value(__wrap_intu_clear_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG1_ADDRESS);
    expect_value(__wrap_intu_clear_interrupt_status, intr_pin_mask, VAB_INTU_CLEAR_MASK);
    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_enable_vab_isr, NULL, NULL)
{
    uint16_t vabs_to_init = 0xfff;

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);
            expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, vab_irq_nums[vab_id]);
            expect_value(__wrap_intu_clear_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG0_ADDRESS);
            expect_value(__wrap_intu_clear_interrupt_status, intr_pin_mask, VAB_INTU_CLEAR_MASK);
            expect_value(__wrap_intu_clear_interrupt_status, intu_base_addr, VAB_VAB_FW_AGG1_ADDRESS);
            expect_value(__wrap_intu_clear_interrupt_status, intr_pin_mask, VAB_INTU_CLEAR_MASK);
            expect_value(__wrap_nvic_irq_clear_pending, irq_num, vab_irq_nums[vab_id]);
            expect_value(__wrap_nvic_irq_enable, irq_num, vab_irq_nums[vab_id]);
        }
    }

    enable_vab_isrs(vabs_to_init);
}

TEST_FUNCTION(test_enable_vab_isr_on_fpga, NULL, NULL)
{
    uint16_t vabs_to_init = 0xfff;

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    /*
     * Skip init on FPGA for now as there are RTL issues with parity error
     * signals spuriously triggering the interrupt.
     */
    enable_vab_isrs(vabs_to_init);
}
