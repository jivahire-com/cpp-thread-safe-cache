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
#include <fpfw_cfg_mgr.h>
#include <interrupts.h>
#include <kng_soc_constants.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdint.h>

extern "C" {
#include <error_handler.h>
#include <vab.h>
#include <vab_intu.h>
#include <vab_irq.h>
#include <vab_regs.h>
#include <vab_rpss_top_regs.h>

vab_knobs_t __real_config_get_vab_knobs(void);
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
    vab_knobs_t vab_config_knobs = __real_config_get_vab_knobs();
    will_return(__wrap_config_get_vab_knobs, &vab_config_knobs);
    expect_function_call(__wrap_config_get_vab_knobs);

    status = vab_common_init(0);
    assert_int_equal(status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_successful_init_all_vabs, NULL, NULL)
{

    // int status;
    uint16_t vabs_to_init = 0xfff;
    vab_knobs_t vab_config_knobs = __real_config_get_vab_knobs();
    will_return(__wrap_config_get_vab_knobs, &vab_config_knobs);
    expect_function_call(__wrap_config_get_vab_knobs);

    // Setup expectations for all VABs
    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);

            expect_value(__wrap_vab_init, vab_init_params->security_state, SECURITY_STATE_NON_SECURE);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->sh_cfg, 1);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->mt_cfg, 1);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->mem_attr, 0b1111);
            expect_value(__wrap_vab_init, vab_init_params->system_counter_delay, 0);
            expect_value(__wrap_vab_init, vab_init_params->vab_resolved_base_addr, 0x0);
            expect_value(__wrap_vab_init, vab_init_params->vab_configure_intu, true);
            expect_value(__wrap_vab_init, vab_init_params->vab_id, vab_id);

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

    vab_knobs_t vab_config_knobs = __real_config_get_vab_knobs();
    will_return(__wrap_config_get_vab_knobs, &vab_config_knobs);
    expect_function_call(__wrap_config_get_vab_knobs);

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

    vab_knobs_t vab_config_knobs = __real_config_get_vab_knobs();
    will_return(__wrap_config_get_vab_knobs, &vab_config_knobs);
    expect_function_call(__wrap_config_get_vab_knobs);

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);

            expect_value(__wrap_vab_init, vab_init_params->security_state, SECURITY_STATE_NON_SECURE);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->sh_cfg, 1);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->mt_cfg, 1);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->mem_attr, 0b1111);
            expect_value(__wrap_vab_init, vab_init_params->system_counter_delay, 0);
            expect_value(__wrap_vab_init, vab_init_params->vab_resolved_base_addr, 0x0);
            expect_value(__wrap_vab_init, vab_init_params->vab_configure_intu, true);
            expect_value(__wrap_vab_init, vab_init_params->vab_id, vab_id);
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

TEST_FUNCTION(test_rpss_vab_isr_rpss_pin_set, NULL, NULL)
{
    rpss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB1_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_rpss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_value(__wrap_rpss_irq_callback, irq_num, HW_INT_VAB1_COMBINED_SCP_INT);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);

    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_rpss_vab_isr_intu0_cri_pin_set, NULL, NULL)
{
    rpss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB2_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_rpss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, 11);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_isr(&mock_isr_ctx);
    }
}

TEST_FUNCTION(test_rpss_vab_isr_intu1_cri_pin_set, NULL, NULL)
{
    rpss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB3_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_rpss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, 11);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_isr(&mock_isr_ctx);
    }
}

TEST_FUNCTION(test_sdmss_vab_isr_no_pins_set, NULL, NULL)
{
    sdmss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_sdmss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);
    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_sdmss_vab_isr_intu0_cri_pin_set, NULL, NULL)
{
    sdmss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_sdmss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, 2);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_isr(&mock_isr_ctx);
    }
}

TEST_FUNCTION(test_sdmss_vab_isr_intu1_cri_pin_set, NULL, NULL)
{
    sdmss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_sdmss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, 2);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, 1);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_isr(&mock_isr_ctx);
    }
}

TEST_FUNCTION(test_cdedss_vab_isr_no_pins_set, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB5_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_cdedss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);

    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_cdedss_vab_isr_intu0_cri_pin_set, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB5_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_cdedss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, 2);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_isr(&mock_isr_ctx);
    }
}

TEST_FUNCTION(test_cdedss_vab_isr_intu1_cri_pin_set, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB5_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_cdedss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, 2);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_isr(&mock_isr_ctx);
    }
}

TEST_FUNCTION(test_enable_d0_vab_isr, NULL, NULL)
{
    uint16_t vabs_to_init = ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) |
                             (1 << D0_VAB3_RPSS3) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS));

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);
            expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, vab_irq_nums[vab_id]);
            expect_value(__wrap_nvic_irq_clear_pending, irq_num, vab_irq_nums[vab_id]);
            expect_value(__wrap_nvic_irq_enable, irq_num, vab_irq_nums[vab_id]);
        }
    }

    enable_vab_isrs(vabs_to_init);
}

TEST_FUNCTION(test_enable_d1_vab_isr, NULL, NULL)
{
    uint16_t vabs_to_init = ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) |
                             (1 << D1_VAB3_RPSS3) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS));

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);
            expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, vab_irq_nums[vab_id]);
            expect_value(__wrap_nvic_irq_clear_pending, irq_num, vab_irq_nums[vab_id]);
            expect_value(__wrap_nvic_irq_enable, irq_num, vab_irq_nums[vab_id]);
        }
    }

    enable_vab_isrs(vabs_to_init);
}

TEST_FUNCTION(test_enable_vab_isr_bad_init_flag, NULL, NULL)
{
    uint16_t vabs_to_init = (1 << MAX_VAB_INSTANCES);

    enable_vab_isrs(vabs_to_init);
}

TEST_FUNCTION(test_enable_vab_isr_no_vabs, NULL, NULL)
{
    enable_vab_isrs(0);
}
