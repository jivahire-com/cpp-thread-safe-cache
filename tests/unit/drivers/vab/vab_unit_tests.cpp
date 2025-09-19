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
#include <idsw_kng.h>
#include <interrupts.h>
#include <kng_soc_constants.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdint.h>

extern "C" {
#include <FpFwUtils.h>
#include <cper.h>
#include <error_handler.h>
#include <pcie_ss_common.h>
#include <pciess_int.h>
#include <vab.h>
#include <vab_atu_mappings.h>
#include <vab_intu.h>
#include <vab_irq.h>
#include <vab_irq_common.h>
#include <vab_regs.h>
#include <vab_rpss_top_regs.h>

vab_knobs_t __real_config_get_vab_knobs(void);
smmu_vab_prod_knobs_t __real_config_get_smmu_vab_knobs(void);
}

/*-- Symbolic Constant Macros (defines) --*/
#define BUGCHECK_MOCK_RETURN   (setjmp(mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

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
static jmp_buf mock_jump_buf;
static bool should_return;
static pcie_ss_entity_t mock_pcie_entity;

/*------------- Functions ----------------*/
extern "C" {
void __wrap_crash_dump_bug_check(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(p0);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    function_called();

    /* Handle noreturn, allowing control to return to test */
    if (!should_return)
    {
        longjmp(mock_jump_buf, 1);
    }
}
}

TEST_FUNCTION(test_skip_all_vabs, NULL, NULL)
{

    int status;

    smmu_vab_prod_knobs_t smmu_vab_config_knob = __real_config_get_smmu_vab_knobs();
    will_return(__wrap_config_get_smmu_vab_knobs, &smmu_vab_config_knob);
    expect_function_call(__wrap_config_get_smmu_vab_knobs);

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

    smmu_vab_prod_knobs_t smmu_vab_config_knob = __real_config_get_smmu_vab_knobs();
    will_return(__wrap_config_get_smmu_vab_knobs, &smmu_vab_config_knob);
    expect_function_call(__wrap_config_get_smmu_vab_knobs);

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
            expect_value(__wrap_vab_init, vab_init_params->vab_resolved_base_addr, 0xDEADBEEF);
            expect_value(__wrap_vab_init, vab_init_params->vab_configure_intu, true);
            expect_value(__wrap_vab_init, vab_init_params->vab_id, vab_id);
            expect_function_call(__wrap_vab_init);
        }
    }

    vab_common_init(vabs_to_init);
}

TEST_FUNCTION(test_atu_map_fail, NULL, NULL)
{
    uint16_t vabs_to_init = 0xfff;

    smmu_vab_prod_knobs_t smmu_vab_config_knob = __real_config_get_smmu_vab_knobs();
    will_return(__wrap_config_get_smmu_vab_knobs, &smmu_vab_config_knob);
    expect_function_call(__wrap_config_get_smmu_vab_knobs);

    vab_knobs_t vab_config_knobs = __real_config_get_vab_knobs();
    will_return(__wrap_config_get_vab_knobs, &vab_config_knobs);
    expect_function_call(__wrap_config_get_vab_knobs);

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_E_INIT);
    expect_function_call(__wrap_crash_dump_bug_check);
    if (!bugcheck_mock_return())
    {
        vab_common_init(vabs_to_init);
    }
}

silibs_status_t mock_ras_generic_handler(ras_error_record_t* record)
{
    assert_non_null(record);
    return SILIBS_SUCCESS;
}

TEST_FUNCTION(test_vab_ras_node_handler, NULL, NULL)
{
    for (uint8_t id = D0_VAB0_RPSS0; id < MAX_VAB_INSTANCES; id++)
    {
        SUBSYSTEM_WITH_VAB_ID vab_id = (SUBSYSTEM_WITH_VAB_ID)id;
        will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
        will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
        will_return(__wrap_ras_agent_probe, true);
        will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
        will_return(__wrap_ras_agent_probe, false);
        expect_function_call(__wrap_ras_print_record);
        will_return(__wrap_ras_arm_record_to_cper, SILIBS_SUCCESS);
        expect_function_call(__wrap_crash_dump_bug_check);
        if (!bugcheck_mock_return())
        {
            vab_ras_node_handler(vab_id, 0xDEADBEEF);
        }

        will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
        will_return(__wrap_ras_agent_probe, nullptr);
        will_return(__wrap_ras_agent_probe, true);
        will_return(__wrap_ras_agent_probe, nullptr);
        will_return(__wrap_ras_agent_probe, false);
        expect_function_call(__wrap_ras_print_record);
        will_return(__wrap_ras_arm_record_to_cper, SILIBS_SUCCESS);
        expect_function_call(__wrap_crash_dump_bug_check);
        if (!bugcheck_mock_return())
        {
            vab_ras_node_handler(vab_id, 0xDEADBEEF);
        }
    }
}

TEST_FUNCTION(test_vab_tbu0_handler, NULL, NULL)
{
    for (uint8_t id = D0_VAB0_RPSS0; id < MAX_VAB_INSTANCES; id++)
    {
        auto vab_id = (SUBSYSTEM_WITH_VAB_ID)id;
        will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
        will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
        will_return(__wrap_ras_agent_probe, true);
        will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
        will_return(__wrap_ras_agent_probe, false);
        expect_function_call(__wrap_ras_print_record);
        vab_tbu0_handler(vab_id, 0xDEADBEEF);

        will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
        will_return(__wrap_ras_agent_probe, nullptr);
        will_return(__wrap_ras_agent_probe, true);
        will_return(__wrap_ras_agent_probe, nullptr);
        will_return(__wrap_ras_agent_probe, false);
        expect_function_call(__wrap_ras_print_record);
        vab_tbu0_handler(vab_id, 0xDEADBEEF);
    }
}

TEST_FUNCTION(test_vab_tbu1_handler, NULL, NULL)
{
    for (uint8_t id = D0_VAB0_RPSS0; id < MAX_VAB_INSTANCES; id++)
    {
        auto vab_id = (SUBSYSTEM_WITH_VAB_ID)id;
        will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
        will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
        will_return(__wrap_ras_agent_probe, true);
        will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
        will_return(__wrap_ras_agent_probe, false);
        expect_function_call(__wrap_ras_print_record);
        vab_tbu1_handler(vab_id, 0xDEADBEEF);

        will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
        will_return(__wrap_ras_agent_probe, nullptr);
        will_return(__wrap_ras_agent_probe, true);
        will_return(__wrap_ras_agent_probe, nullptr);
        will_return(__wrap_ras_agent_probe, false);
        expect_function_call(__wrap_ras_print_record);
        vab_tbu1_handler(vab_id, 0xDEADBEEF);
    }
}

TEST_FUNCTION(test_vab_tcu_handler, NULL, NULL)
{
    for (uint8_t id = D0_VAB0_RPSS0; id < MAX_VAB_INSTANCES; id++)
    {
        auto vab_id = (SUBSYSTEM_WITH_VAB_ID)id;
        will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
        will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
        will_return(__wrap_ras_agent_probe, true);
        will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
        will_return(__wrap_ras_agent_probe, false);
        expect_function_call(__wrap_ras_print_record);
        vab_tcu_handler(vab_id, 0xDEADBEEF);

        will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
        will_return(__wrap_ras_agent_probe, nullptr);
        will_return(__wrap_ras_agent_probe, true);
        will_return(__wrap_ras_agent_probe, nullptr);
        will_return(__wrap_ras_agent_probe, false);
        expect_function_call(__wrap_ras_print_record);
        vab_tcu_handler(vab_id, 0xDEADBEEF);
    }
}

TEST_FUNCTION(test_vab_parity_errors, NULL, NULL)
{
    uint8_t id;
    will_return_always(__wrap_vab_get_1p_parity_status, SILIBS_SUCCESS);

    for (id = D0_VAB0_RPSS0; id < MAX_VAB_INSTANCES; id++)
    {
        auto vab_id = (SUBSYSTEM_WITH_VAB_ID)id;
        expect_function_calls(__wrap_crash_dump_bug_check, 5);
        if (!bugcheck_mock_return())
        {
            vab_fabric_parity_handler(vab_id, 0xDEADBEEF);
        }

        if (!bugcheck_mock_return())
        {
            vab_csr_parity_handler(vab_id, 0xDEADBEEF);
        }

        if (!bugcheck_mock_return())
        {
            vab_pcr_parity_handler(vab_id, 0xDEADBEEF);
        }

        if (!bugcheck_mock_return())
        {
            vab_integ_pcr_parity_handler(vab_id, 0xDEADBEEF);
        }

        if (!bugcheck_mock_return())
        {
            vab_integ_csr_parity_handler(vab_id, 0xDEADBEEF);
        }
    }
}

TEST_FUNCTION(test_vab_tower_fmu_handler, NULL, NULL)
{
    for (uint8_t id = D0_VAB0_RPSS0; id < MAX_VAB_INSTANCES; id++)
    {
        auto vab_id = (SUBSYSTEM_WITH_VAB_ID)id;
        will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
        vab_tower_handler(vab_id, 0xDEADBEEF);
    }
}

TEST_FUNCTION(test_rpss_vab_isr_no_pins_set, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB2_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_rpss_probe;

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

TEST_FUNCTION(test_rpss_ras_node_handler, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB2_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_rpss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, VAB_RPSS_INTU0_RPSS_RAS_CRI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, VAB_RPSS_INTU1_RPSS_RAS_FHI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);

    will_return(__wrap_pciess_get_entity, &mock_pcie_entity);
    will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
    will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
    will_return(__wrap_ras_agent_probe, true);
    will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
    will_return(__wrap_ras_agent_probe, false);
    expect_function_call(__wrap_ras_print_record);

    will_return(__wrap_pciess_get_entity, &mock_pcie_entity);
    will_return(__wrap_ras_arm_agent_set_base, SILIBS_SUCCESS);
    will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
    will_return(__wrap_ras_agent_probe, true);
    will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
    will_return(__wrap_ras_agent_probe, false);
    expect_function_call(__wrap_ras_print_record);

    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);

    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_rpss_combined_interrupt, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB2_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_rpss_probe;
    mock_isr_ctx.vab_id = D0_VAB2_RPSS2;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, VAB_RPSS_INTU0_RPSS_SCP_INT);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, VAB_RPSS_INTU1_RPSS_NITOWER_FHI);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);

    will_return(__wrap_pciess_get_entity, &mock_pcie_entity);
    will_return(__wrap_pciess_probe, true);
    will_return(__wrap_pciess_clear_handler, SILIBS_SUCCESS);

    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);

    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_rpss_tower_handler, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB2_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_RPSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_RPSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_rpss_probe;
    mock_isr_ctx.vab_id = D0_VAB2_RPSS2;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, VAB_RPSS_INTU0_RPSS_NITOWER_CRI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, VAB_RPSS_INTU1_RPSS_NITOWER_FHI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);

    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_sdmss_vab_isr_no_pins_set, NULL, NULL)
{
    sdmss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_SDMSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_SDMSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_sdmss_probe;

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

TEST_FUNCTION(test_sdmss_vab_isr_intu0_d2d_pin_set, NULL, NULL)
{
    sdmss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_SDMSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_SDMSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_sdmss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, VAB_SDMSS_INTU0_D2DSS_0_3_RAS_CRI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, 12);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    expect_function_call(__wrap_d2d_error_isr);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);

    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_sdmss_vab_isr_sdmss_tower_handler, NULL, NULL)
{
    sdmss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_SDMSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_SDMSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_sdmss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, VAB_SDMSS_INTU0_SDMSS_NITOWER_CRI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, VAB_SDMSS_INTU1_SDMSS_NITOWER_FHI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);
    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_sdmss_vab_isr_d2d_cfg0_tower_handler, NULL, NULL)
{
    sdmss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_SDMSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_SDMSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_sdmss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, VAB_SDMSS_INTU0_D2DSS_CFG0_NITOWER_CRI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, VAB_SDMSS_INTU1_D2DSS_CFG0_NITOWER_FHI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);
    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_sdmss_vab_isr_d2d_cfg1_tower_handler, NULL, NULL)
{
    sdmss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_SDMSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_SDMSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_sdmss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, VAB_SDMSS_INTU0_D2DSS_CFG1_NITOWER_CRI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, VAB_SDMSS_INTU1_D2DSS_CFG1_NITOWER_FHI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);
    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_cdedss_vab_isr_no_pins_set, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB5_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_CDEDSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_CDEDSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_cdedss_probe;

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

TEST_FUNCTION(test_cdedss_vab_isr_intu0_ioss_tower_fmu_handler, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB5_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_CDEDSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_CDEDSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_cdedss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, VAB_CDEDSS_IOSS_INTU0_IOSS_NITOWER_CRI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, VAB_CDEDSS_IOSS_INTU1_VAB5_NITOWER_FHI);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);

    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_cdedss_vab_isr_intu1_ioss_tower_fmu_handler, NULL, NULL)
{
    cdedss_ioss_intu_probe_t mock_probe;
    vab_isr_ctx_t mock_isr_ctx;

    mock_isr_ctx.vab_irq_num = HW_INT_VAB5_COMBINED_SCP_INT;
    mock_isr_ctx.vab_base = VAB_RPSS_TOP_VAB_ADDRESS;
    mock_isr_ctx.probe.intu0 = (vabss_int_t*)&(mock_probe.intu0);
    mock_isr_ctx.probe.num_intu0_pins = VAB_CDEDSS_INTU0_NUM_CFGS;
    mock_isr_ctx.probe.intu1 = (vabss_int_t*)&(mock_probe.intu1);
    mock_isr_ctx.probe.num_intu1_pins = VAB_CDEDSS_INTU1_NUM_CFGS;
    mock_isr_ctx.process_probe = process_vab_cdedss_probe;

    expect_value(__wrap_vabss_intu_probe, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_probe, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_probe, VAB_CDEDSS_IOSS_INTU0_IOSS_NITOWER_CRI);
    will_return(__wrap_vabss_intu_probe, false);
    will_return(__wrap_vabss_intu_probe, VAB_CDEDSS_IOSS_INTU1_IOSS_NITOWER_FHI);
    will_return(__wrap_vabss_intu_probe, true);
    will_return(__wrap_vabss_intu_probe, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_handler, SILIBS_SUCCESS);
    expect_value(__wrap_vabss_intu_clear, vab_base, VAB_RPSS_TOP_VAB_ADDRESS);
    expect_value(__wrap_vabss_intu_clear, dest, INTU_TO_SCP);
    will_return(__wrap_vabss_intu_clear, SILIBS_SUCCESS);

    vab_isr(&mock_isr_ctx);
}

TEST_FUNCTION(test_vab_error_injection, NULL, NULL)
{
    ras_einj_info_t mock_payload;
    acpi_einj_cmd_status_t ret;
    auto* params = (vab_error_inj_param_t*)(&mock_payload.param_type.error_parameters[0]);

    mock_payload.component_group = ACPI_ERROR_DOMAIN_VAB;
    mock_payload.component_type = D0_VAB0_RPSS0;
    mock_payload.component_instance = 0;

    /* Prep resolved base */
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    map_vab_instance(D0_VAB0_RPSS0);

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 0);
    params->target = VAB_ERROR_TARGET_FABRIC;
    will_return(__wrap_vab_fabric_error_trigger_by_type, SILIBS_SUCCESS);
    ret = vab_error_injection_cb(&mock_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_SUCCESS);

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 0);
    params->target = VAB_ERROR_TARGET_RAS_NODE;
    will_return(__wrap_vab_ras_trigger_by_type, SILIBS_SUCCESS);
    ret = vab_error_injection_cb(&mock_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_SUCCESS);
}

TEST_FUNCTION(test_vab_error_injection_failure_modes, NULL, NULL)
{
    ras_einj_info_t mock_payload;
    acpi_einj_cmd_status_t ret;
    auto* params = (vab_error_inj_param_t*)(&mock_payload.param_type.error_parameters[0]);

    /* Prep resolved base */
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    map_vab_instance(D0_VAB0_RPSS0);

    /* NULL einj payload */
    ret = vab_error_injection_cb(nullptr, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    /* Invalid error domain */
    mock_payload.component_group = ACPI_ERROR_DOMAIN_GIC;
    ret = vab_error_injection_cb(&mock_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    /* Die ID mismatch */
    mock_payload.component_group = ACPI_ERROR_DOMAIN_VAB;
    mock_payload.component_instance = 0;
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 1);
    ret = vab_error_injection_cb(&mock_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    /* Invalid VAB ID */
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 0);
    mock_payload.component_type = MAX_VAB_INSTANCES;
    ret = vab_error_injection_cb(&mock_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    /* Fabric injection error */
    mock_payload.component_type = D0_VAB0_RPSS0;
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 0);
    params->target = VAB_ERROR_TARGET_FABRIC;
    will_return(__wrap_vab_fabric_error_trigger_by_type, SILIBS_E_SUPPORT);
    ret = vab_error_injection_cb(&mock_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    /* RAS node injection error */
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 0);
    params->target = VAB_ERROR_TARGET_RAS_NODE;
    will_return(__wrap_vab_ras_trigger_by_type, SILIBS_E_SUPPORT);
    ret = vab_error_injection_cb(&mock_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    /* Bad error type */
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 0);
    params->target = VAB_ERROR_TARGET_INVALID;
    ret = vab_error_injection_cb(&mock_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);
}

TEST_FUNCTION(test_enable_d0_vab_isr, NULL, NULL)
{
    uint16_t vabs_to_init = ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) |
                             (1 << D0_VAB3_RPSS3) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS));

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            /* Prep resolved base */
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);
            map_vab_instance(vab_id);
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

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            /* Prep resolved base */
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);
            map_vab_instance(vab_id);
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

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    enable_vab_isrs(vabs_to_init);
}

TEST_FUNCTION(test_enable_vab_isr_no_vabs, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    enable_vab_isrs(0);
}

TEST_FUNCTION(test_enable_vab_isr_on_fpga, NULL, NULL)
{
    uint16_t vabs_to_init = ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) |
                             (1 << D0_VAB3_RPSS3) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS));

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);

    /*
     * Skip init on FPGA for now as there are issues with RAS fault handling
     * interrupts constantly triggering.
     */
    enable_vab_isrs(vabs_to_init);
}

TEST_FUNCTION(test_get_rpss_resolved_base, NULL, NULL)
{
    uintptr_t base_addr;

    /* Map all VAB instances first to initialize ATU mappings */
    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
        will_return(__wrap_atu_map, SILIBS_SUCCESS);
        map_vab_instance(vab_id);
    }

    /* Test all valid RPSS instances */
    for (uint8_t rpss_id = RPSS0; rpss_id < NUM_RPSS; rpss_id++)
    {
        base_addr = get_rpss_resolved_base((RPSS_INSTANCE)rpss_id);

        /* Verify that base address is non-zero (indicating successful mapping) */
        assert_true(base_addr != 0);
    }
}

TEST_FUNCTION(test_get_rpss_resolved_base_invalid_id, NULL, NULL)
{
    /* Test with invalid RPSS ID (NUM_RPSS or greater) */
    expect_function_call(__wrap_crash_dump_bug_check);
    if (!bugcheck_mock_return())
    {
        get_rpss_resolved_base((RPSS_INSTANCE)NUM_RPSS);
    }

    /* Test with an out-of-bounds value */
    expect_function_call(__wrap_crash_dump_bug_check);
    if (!bugcheck_mock_return())
    {
        get_rpss_resolved_base((RPSS_INSTANCE)0xFF);
    }
}

TEST_FUNCTION(test_get_vab_resolved_base, NULL, NULL)
{
    uintptr_t base_addr;

    /* Map all VAB instances first to initialize ATU mappings */
    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
        will_return(__wrap_atu_map, SILIBS_SUCCESS);
        map_vab_instance(vab_id);
    }

    /* Test all valid VAB instances */
    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        base_addr = get_vab_resolved_base(vab_id);

        /* Verify that base address matches what was set by the mock */
        assert_int_equal(base_addr, 0xDEADBEEF);
    }
}

TEST_FUNCTION(test_get_vab_resolved_base_invalid_id, NULL, NULL)
{
    /* Test with invalid VAB ID (MAX_VAB_INSTANCES or greater) */
    expect_function_call(__wrap_crash_dump_bug_check);
    if (!bugcheck_mock_return())
    {
        get_vab_resolved_base(MAX_VAB_INSTANCES);
    }

    /* Test with an out-of-bounds value */
    expect_function_call(__wrap_crash_dump_bug_check);
    if (!bugcheck_mock_return())
    {
        get_vab_resolved_base(0xFFFF);
    }
}
