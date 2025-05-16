//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddrss.cpp
 * ddrss tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FPFwInterrupts.h>
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <cmn_config.h>
#include <ddrss.h>
#include <ddrss_intu.h>
#include <ddrss_lib.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <silibs_ap_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static const int HW_INT_DDRSS0(106);
static const int HW_INT_DDRSS5(111);
static uint32_t ddrss_num[6] = {0, 1, 2, 3, 4, 5};

extern uint32_t g_ddr_intu_sts;
extern uint32_t g_intu_enable;
extern uint32_t g_phy_int_sts;
extern uint32_t g_mc_intu_sts;
extern uint32_t g_mc_intu_dest_enable;
extern bool g_mmio_read32_mocktype;
extern bool g_should_check_cper_section;

/*------------- Functions ----------------*/
static int setup(void** state)
{
    FPFW_UNUSED(state);
    g_ddr_intu_sts = 0;
    g_intu_enable = 0;
    g_phy_int_sts = 0;
    g_mc_intu_sts = 0;
    g_mc_intu_dest_enable = 0;
    g_mmio_read32_mocktype = false;

    return 0;
}

static int teardown(void** state)
{
    FPFW_UNUSED(state);
    g_ddr_intu_sts = 0;
    g_intu_enable = 0;
    g_phy_int_sts = 0;
    g_mc_intu_sts = 0;
    g_mc_intu_dest_enable = 0;
    g_mmio_read32_mocktype = false;

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_prod_ddrss_lib_init_partial_or_skip, setup, teardown)
{
    KNG_DIE_ID test_die = (KNG_DIE_ID)0;
    cmn800_snf_to_mc_config_t cmn800_snf_to_mc_config;
    int i = 0;

    // ddrss init is skipped on svp, therefore no expectations
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    for (int this_irq_num = HW_INT_DDRSS0; this_irq_num <= HW_INT_DDRSS5; this_irq_num++)
    {
        i = (this_irq_num - HW_INT_DDRSS0);

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, prod_ddrss_interrupt_handler);
        expect_value(__wrap_nvic_irq_set_isr_with_param, ddrss_num, ddrss_num[i]);

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    will_return_always(__wrap_system_info_is_warm_start, false);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_0);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_1);
    will_return_always(__wrap_ddrss_atu_map_cfg_space, 0x12345678);
    will_return(__wrap_cmn800_generate_ddr_mc_map_from_cached_config, &cmn800_snf_to_mc_config);

    will_return(__wrap_ddrss_init, SILIBS_SUCCESS);
    expect_value(__wrap_ddrss_atu_unmap_cfg_space, die_num, DIE_1);

    prod_ddrss_lib_init(test_die);

    // ddrss init is skipped if not on an FPGA, therefore no expectations
    idsw_set_platform_sdv(PLATFORM_UNDEFINED);

    for (int this_irq_num = HW_INT_DDRSS0; this_irq_num <= HW_INT_DDRSS5; this_irq_num++)
    {
        i = (this_irq_num - HW_INT_DDRSS0);

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, prod_ddrss_interrupt_handler);
        expect_value(__wrap_nvic_irq_set_isr_with_param, ddrss_num, ddrss_num[i]);

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }
    prod_ddrss_lib_init(test_die);
}

TEST_FUNCTION(test_ddrss_lib_init_fpga, setup, teardown)
{
    cmn800_snf_to_mc_config_t cmn800_snf_to_mc_config;
    KNG_DIE_ID test_die = (KNG_DIE_ID)1;
    int i = 0;

    // initialize the CFG
    cmn800_snf_to_mc_config.is_numa_enabled = 1;
    cmn800_snf_to_mc_config.map_size = 0;
    memset(cmn800_snf_to_mc_config.ddr_mc_map, 0xff, sizeof(cmn800_snf_to_mc_config.ddr_mc_map));
    cmn800_snf_to_mc_config.hash_select = 0;

    // set up die id
    idsw_set_die_id(test_die);

    // ddrss init is not skipped on the Big FPGA, and the
    // atu map is fixed so no atu map / un map calls
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    for (int this_irq_num = HW_INT_DDRSS0; this_irq_num <= HW_INT_DDRSS5; this_irq_num++)
    {
        i = (this_irq_num - HW_INT_DDRSS0);

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, prod_ddrss_interrupt_handler);
        expect_value(__wrap_nvic_irq_set_isr_with_param, ddrss_num, ddrss_num[i]);

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }

    will_return(__wrap_idhw_is_single_die_boot_en, true);
    will_return(__wrap_system_info_is_warm_start, false);
    will_return(__wrap_cmn800_generate_ddr_mc_map_from_cached_config, &cmn800_snf_to_mc_config);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_0);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_1);
    will_return_always(__wrap_ddrss_atu_map_cfg_space, 0x12345678);
    will_return(__wrap_ddrss_init, SILIBS_SUCCESS);
    expect_value(__wrap_ddrss_atu_unmap_cfg_space, die_num, DIE_0);

    prod_ddrss_lib_init(test_die);
}

TEST_FUNCTION(test_ddrss_lib_init_fpga_warm_start, setup, teardown)
{
    cmn800_snf_to_mc_config_t cmn800_snf_to_mc_config;
    KNG_DIE_ID test_die = (KNG_DIE_ID)1;
    int i = 0;
    extern bool g_should_check_reset_reason_cfg_knobs;
    g_should_check_reset_reason_cfg_knobs = true;

    // Get same starting values for knobs as code under test
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    // initialize the CFG
    cmn800_snf_to_mc_config.is_numa_enabled = 1;
    cmn800_snf_to_mc_config.map_size = 0;
    memset(cmn800_snf_to_mc_config.ddr_mc_map, 0xff, sizeof(cmn800_snf_to_mc_config.ddr_mc_map));
    cmn800_snf_to_mc_config.hash_select = 0;

    // set up die id
    idsw_set_die_id(test_die);

    // ddrss init is not skipped on the Big FPGA, and the
    // atu map is fixed so no atu map / un map calls
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    for (int this_irq_num = HW_INT_DDRSS0; this_irq_num <= HW_INT_DDRSS5; this_irq_num++)
    {
        i = (this_irq_num - HW_INT_DDRSS0);

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, prod_ddrss_interrupt_handler);
        expect_value(__wrap_nvic_irq_set_isr_with_param, ddrss_num, ddrss_num[i]);

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }

    will_return(__wrap_idhw_is_single_die_boot_en, true);
    test_ddrss_knobs.ext_knobs.ddrss_mask = 0x03F;
    test_ddrss_knobs.die_id = (DIE_INSTANCE)test_die;

    will_return(__wrap_system_info_is_warm_start, true);
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_WARM;

    will_return(__wrap_cmn800_generate_ddr_mc_map_from_cached_config, &cmn800_snf_to_mc_config);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_0);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_1);
    will_return_always(__wrap_ddrss_atu_map_cfg_space, 0x12345678);

    expect_value(__wrap_ddrss_init, cfg_knobs->reset_reason, DDRSS_SYS_RESET_WARM);
    will_return(__wrap_ddrss_init, SILIBS_SUCCESS);
    expect_value(__wrap_ddrss_atu_unmap_cfg_space, die_num, DIE_0);

    prod_ddrss_lib_init(test_die);
    g_should_check_reset_reason_cfg_knobs = false;
}

TEST_FUNCTION(test_ddrss_lib_init_emu, setup, teardown)
{
    cmn800_snf_to_mc_config_t cmn800_snf_to_mc_config;
    KNG_DIE_ID test_die = (KNG_DIE_ID)0;
    int i = 0;

    // initialize the CFG
    cmn800_snf_to_mc_config.is_numa_enabled = 0;
    cmn800_snf_to_mc_config.map_size = 0;
    memset(cmn800_snf_to_mc_config.ddr_mc_map, 0xff, sizeof(cmn800_snf_to_mc_config.ddr_mc_map));
    cmn800_snf_to_mc_config.hash_select = 0;

    // set up die id
    idsw_set_die_id(test_die);

    // ddrss init is not skipped on the Big FPGA, and the
    // atu map is fixed so no atu map / un map calls
    idsw_set_platform_sdv(PLATFORM_EMU_1D);
    for (int this_irq_num = HW_INT_DDRSS0; this_irq_num <= HW_INT_DDRSS5; this_irq_num++)
    {
        i = (this_irq_num - HW_INT_DDRSS0);

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, prod_ddrss_interrupt_handler);
        expect_value(__wrap_nvic_irq_set_isr_with_param, ddrss_num, ddrss_num[i]);

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_system_info_is_warm_start, false);
    will_return(__wrap_cmn800_generate_ddr_mc_map_from_cached_config, &cmn800_snf_to_mc_config);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_0);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_1);
    will_return_always(__wrap_ddrss_atu_map_cfg_space, 0x12345678);
    will_return(__wrap_ddrss_init, SILIBS_SUCCESS);
    expect_value(__wrap_ddrss_atu_unmap_cfg_space, die_num, DIE_1);

    prod_ddrss_lib_init(test_die);
}

TEST_FUNCTION(test_ddrss_lib_init_rvp, setup, teardown)
{
    cmn800_snf_to_mc_config_t cmn800_snf_to_mc_config;
    KNG_DIE_ID test_die = (KNG_DIE_ID)1;
    int i = 0;

    // initialize the CFG
    cmn800_snf_to_mc_config.is_numa_enabled = 1;
    cmn800_snf_to_mc_config.map_size = 0;
    memset(cmn800_snf_to_mc_config.ddr_mc_map, 0xff, sizeof(cmn800_snf_to_mc_config.ddr_mc_map));
    cmn800_snf_to_mc_config.hash_select = 0;

    // set up die id
    idsw_set_die_id(test_die);
    idsw_set_platform_sdv(PLATFORM_RVP_EVT_SILICON);
    for (int this_irq_num = HW_INT_DDRSS0; this_irq_num <= HW_INT_DDRSS5; this_irq_num++)
    {
        i = (this_irq_num - HW_INT_DDRSS0);

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, prod_ddrss_interrupt_handler);
        expect_value(__wrap_nvic_irq_set_isr_with_param, ddrss_num, ddrss_num[i]);

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }

    will_return(__wrap_idhw_is_single_die_boot_en, true);
    will_return(__wrap_system_info_is_warm_start, false);
    will_return(__wrap_cmn800_generate_ddr_mc_map_from_cached_config, &cmn800_snf_to_mc_config);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_0);
    expect_value(__wrap_ddrss_atu_map_cfg_space, die_num, DIE_1);
    will_return_always(__wrap_ddrss_atu_map_cfg_space, 0x12345678);
    will_return(__wrap_ddrss_init, SILIBS_SUCCESS);
    expect_value(__wrap_ddrss_atu_unmap_cfg_space, die_num, DIE_0);

    prod_ddrss_lib_init(test_die);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_unexpected_interrupt, setup, teardown)
{
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_HSP_INT); // This is unexpected
    g_intu_enable = 0xFFFFFFFF;                     // This is a mask

    // Vendor CPER
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_err_sec_mem_vendor_t));

    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_MC0_HSP_INT));
    prod_ddrss_interrupt_handler((void*)&ddrss_num[1]);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_MC0_CRI_INT, setup, teardown)
{
    // Test MC0_CRI_INT
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_CRI_INT);
    g_intu_enable = 0xFFFFFFFF; // This is a mask
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    expect_function_call(__wrap_ras_arm_agent_probe);
    expect_function_call(__wrap_ras_print_record);
    expect_function_call(__wrap_ddrss_convert_ras_rec_to_cper);

    // Std. CPER
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));

    // Vendor CPER
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_err_sec_mem_vendor_t));

    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_MC0_CRI_INT));
    prod_ddrss_interrupt_handler((void*)&ddrss_num[0]);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_MC1_CRI_INT, setup, teardown)
{
    // Test MC1_CRI_INT
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC1_CRI_INT);
    g_intu_enable = 0xFFFFFFFF; // This is a mask
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_E_PARAM);
    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_MC1_CRI_INT));
    prod_ddrss_interrupt_handler((void*)&ddrss_num[5]);

    // Test MC1_CRI_INT
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC1_CRI_INT);
    g_intu_enable = 0xFFFFFFFF; // This is a mask
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    expect_function_call(__wrap_ras_arm_agent_probe);
    expect_function_call(__wrap_ras_print_record);
    expect_function_call(__wrap_ddrss_convert_ras_rec_to_cper);

    // Std. CPER
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));

    // Vendor CPER
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_err_sec_mem_vendor_t));

    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_MC1_CRI_INT));
    prod_ddrss_interrupt_handler((void*)&ddrss_num[5]);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_DDRSS_INTU_SRA_ERI, setup, teardown)
{
    g_mmio_read32_mocktype = true;

    // Test DDRSS_INTU_SRA_ERI
    g_ddr_intu_sts = (1 << DDRSS_INTU_SRA_ERI);
    g_intu_enable = 0xFFFFFFFF; // This is a mask

    will_return(__wrap_mmio_read32, 1); // This is the (non-zero) mock return value for the MMIO_READ32
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_E_PARAM);
    will_return(__wrap_mmio_read32, 1); // This is the (non-zero) mock return value for the second MMIO_READ32
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_E_PARAM);
    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_SRA_ERI));
    prod_ddrss_interrupt_handler((void*)&ddrss_num[0]);

    // Test DDRSS_INTU_SRA_ERI - other path
    g_ddr_intu_sts = (1 << DDRSS_INTU_SRA_ERI);
    g_intu_enable = 0xFFFFFFFF; // This is a mask

    will_return(__wrap_mmio_read32, 1); // This is the (non-zero) mock return value for the MMIO_READ32
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    expect_function_call(__wrap_ras_arm_agent_probe);
    expect_function_call(__wrap_ras_print_record);
    expect_function_call(__wrap_ddrss_convert_ras_rec_to_cper);

    // Std. CPER
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));

    // Vendor CPER
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_err_sec_mem_vendor_t));

    will_return(__wrap_mmio_read32, 1); // This is the (non-zero) mock return value for the second MMIO_READ32
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    expect_function_call(__wrap_ras_arm_agent_probe);
    expect_function_call(__wrap_ras_print_record);
    expect_function_call(__wrap_ddrss_convert_ras_rec_to_cper);

    // Std. CPER
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));

    // Vendor CPER
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_err_sec_mem_vendor_t));

    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_SRA_ERI));
    prod_ddrss_interrupt_handler((void*)&ddrss_num[0]);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_phy, setup, teardown)
{
    // Test PHY interrupts
    g_ddr_intu_sts = (1 << DDRSS_INTU_PHY_IRQ);
    g_intu_enable = 0xFFFFFFFF; // This is a mask
    g_phy_int_sts = csr_PhyTrngFailEn_MASK | csr_PhyTrngCmpltEn_MASK | csr_PhyInitCmpltEn_MASK |
                    csr_PhyAcsmParityErrEn_MASK | csr_PhyPIEParityErrEn_MASK | csr_PhyRdfPtrChkErrEn_MASK |
                    csr_PhyEccEn_MASK | csr_PhyPIEProgErrEn_MASK | csr_PhyTxPPTEn_MASK | csr_PhyAlertEn_MASK;

    // Vendor CPER - ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_err_sec_mem_vendor_t));

    expect_value(__wrap_ddrss_clear_phy_interrupt_status, phy_int_sts, g_phy_int_sts);
    expect_any_always(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask);
    prod_ddrss_interrupt_handler((void*)&ddrss_num[0]);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_mc, setup, teardown)
{
    // DDRSS_INTU_MC0_SCP_INT
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_SCP_INT);
    g_intu_enable = 0xFFFFFFFF;         // This is a mask
    g_mc_intu_dest_enable = 0xFFFFFFFF; // So is this
    g_mc_intu_sts = (1 << DDRSS_INTU_MC_FEDFLUSHDONE) | (1 << DDRSS_INTU_MC_RMTELEMETRYAVAIL);

    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, g_ddr_intu_sts);
    prod_ddrss_interrupt_handler((void*)&ddrss_num[0]);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_others, setup, teardown)
{
    g_should_check_cper_section = true;
    acpi_err_sec_mem_vendor_t ddr_vendor_cper = {0};

    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_HSP_INT) | (1 << DDRSS_INTU_PLL_INTERRUPT_OUT) |
                     (1 << DDRSS_INTU_PCR_PAR_ERR) | (1 << DDRSS_INTU_INTU_PAR_ERR);
    g_intu_enable = 0xFFFFFFFF; // This is a mask

    // Vendor CPER #1 - not set by ddrss_convert_ras_rec_to_cper
    ddr_vendor_cper.module = 6;
    ddr_vendor_cper.valid_module = 1;
    ddr_vendor_cper.device = 1;
    ddr_vendor_cper.valid_device = 1;
    ddr_vendor_cper.error_type = DDRSS_CPER_ERROR_UNKNOWN;
    ddr_vendor_cper.valid_error_type = 1;
    ddr_vendor_cper.error_status.error_type = ddr_vendor_cper.error_type;
    ddr_vendor_cper.error_status.data = 1;
    ddr_vendor_cper.valid_error_status = 1;
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_memory(__wrap_hm_submit_cper, err_record_section, &ddr_vendor_cper, sizeof(acpi_err_sec_mem_vendor_t));
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));

    // Vendor CPER #2 - not set by ddrss_convert_ras_rec_to_cper
    memset(&ddr_vendor_cper, 0, sizeof(ddr_vendor_cper));
    ddr_vendor_cper.vendor_err_info.misc.ddr_intu_pll_out = 1;
    ddr_vendor_cper.vendor_err_info.misc.valid_ddr_intu_pll_out = 1;
    ddr_vendor_cper.error_type = DDRSS_CPER_INTU_PLL_INTERRUPT;
    ddr_vendor_cper.module = 6;
    ddr_vendor_cper.valid_module = 1;
    ddr_vendor_cper.device = 1;
    ddr_vendor_cper.valid_device = 1;
    ddr_vendor_cper.valid_error_type = 1;
    ddr_vendor_cper.error_status.error_type = ddr_vendor_cper.error_type;
    ddr_vendor_cper.error_status.control = 1;
    ddr_vendor_cper.valid_error_status = 1;
    ddr_vendor_cper.valid_vendor_err_info = 1;
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_memory(__wrap_hm_submit_cper, err_record_section, &ddr_vendor_cper, sizeof(acpi_err_sec_mem_vendor_t));
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_err_sec_mem_vendor_t));

    // Vendor CPER #3 - not set by ddrss_convert_ras_rec_to_cper
    memset(&ddr_vendor_cper, 0, sizeof(ddr_vendor_cper));
    ddr_vendor_cper.vendor_err_info.misc.ddr_pcr_par_err = 1;
    ddr_vendor_cper.vendor_err_info.misc.valid_ddr_pcr_par_err = 1;
    ddr_vendor_cper.error_type = DDRSS_CPER_INTU_PCR_PAR_ERR;
    ddr_vendor_cper.module = 6;
    ddr_vendor_cper.valid_module = 1;
    ddr_vendor_cper.device = 1;
    ddr_vendor_cper.valid_device = 1;
    ddr_vendor_cper.valid_error_type = 1;
    ddr_vendor_cper.error_status.error_type = ddr_vendor_cper.error_type;
    ddr_vendor_cper.error_status.control = 1;
    ddr_vendor_cper.valid_error_status = 1;
    ddr_vendor_cper.valid_vendor_err_info = 1;
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_memory(__wrap_hm_submit_cper, err_record_section, &ddr_vendor_cper, sizeof(acpi_err_sec_mem_vendor_t));
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_err_sec_mem_vendor_t));

    // Vendor CPER #4 - not set by ddrss_convert_ras_rec_to_cper
    memset(&ddr_vendor_cper, 0, sizeof(ddr_vendor_cper));
    ddr_vendor_cper.vendor_err_info.misc.ddr_intu_par_err = 1;
    ddr_vendor_cper.vendor_err_info.misc.valid_ddr_intu_par_err = 1;
    ddr_vendor_cper.error_type = DDRSS_CPER_INTU_PAR_ERR;
    ddr_vendor_cper.module = 6;
    ddr_vendor_cper.valid_module = 1;
    ddr_vendor_cper.device = 1;
    ddr_vendor_cper.valid_device = 1;
    ddr_vendor_cper.valid_error_type = 1;
    ddr_vendor_cper.error_status.error_type = ddr_vendor_cper.error_type;
    ddr_vendor_cper.error_status.control = 1;
    ddr_vendor_cper.valid_error_status = 1;
    ddr_vendor_cper.valid_vendor_err_info = 1;
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_memory(__wrap_hm_submit_cper, err_record_section, &ddr_vendor_cper, sizeof(acpi_err_sec_mem_vendor_t));
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_err_sec_mem_vendor_t));

    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, g_ddr_intu_sts);
    prod_ddrss_interrupt_handler((void*)&ddrss_num[0]);
    g_should_check_cper_section = false;
}

// Test prod_ddrss_interrupt_pending

TEST_FUNCTION(test_prod_ddrss_get_intr_event_cper, setup, teardown)
{
    uint32_t mc = 0;
    silibs_status_t status = 0;
    uint32_t intr_event = 0;
    acpi_err_sec_mem_vendor_t ddr_cper = {};

    intr_event = DDRSS_INTU_MC_MEDIAECSTRANSPCHANGED;
    status = prod_ddrss_get_intr_event_cper(mc, intr_event, &ddr_cper);
    assert_int_equal(status, SILIBS_SUCCESS);
    assert_int_equal(ddr_cper.vendor_err_info.misc.mc_evt_ecs_trandchanged, 1);
    assert_int_equal(ddr_cper.vendor_err_info.misc.valid_mc_evt_ecs_trandchanged, 1);
    assert_int_equal(ddr_cper.error_type, DDRSS_CPER_EVT_ECS_TRANS_CHANGED);

    intr_event = DDRSS_INTU_MC_MEDIAREFTEMPCHANGED;
    memset(&ddr_cper, 0, sizeof(ddr_cper));
    status = prod_ddrss_get_intr_event_cper(mc, intr_event, &ddr_cper);
    assert_int_equal(status, SILIBS_SUCCESS);
    assert_int_equal(ddr_cper.vendor_err_info.misc.mc_evtref_tempchanged, 1);
    assert_int_equal(ddr_cper.vendor_err_info.misc.valid_mc_evtref_tempchanged, 1);
    assert_int_equal(ddr_cper.error_type, DDRSS_CPER_EVT_REF_TEMP_CHANGED);

    intr_event = DDRSS_INTU_MC_MEDIAREFTEMPHIGH;
    memset(&ddr_cper, 0, sizeof(ddr_cper));
    status = prod_ddrss_get_intr_event_cper(mc, intr_event, &ddr_cper);
    assert_int_equal(status, SILIBS_SUCCESS);
    assert_int_equal(ddr_cper.vendor_err_info.misc.mc_evtref_temphigh, 1);
    assert_int_equal(ddr_cper.vendor_err_info.misc.valid_mc_evtref_temphigh, 1);
    assert_int_equal(ddr_cper.error_type, DDRSS_CPER_EVT_REF_TEMP_HIGH);

    intr_event = DDRSS_INTU_PLL_INTERRUPT_OUT;
    memset(&ddr_cper, 0, sizeof(ddr_cper));
    status = prod_ddrss_get_intr_event_cper(mc, intr_event, &ddr_cper);
    assert_int_equal(status, SILIBS_SUCCESS);
    assert_int_equal(ddr_cper.vendor_err_info.misc.ddr_intu_pll_out, 1);
    assert_int_equal(ddr_cper.vendor_err_info.misc.valid_ddr_intu_pll_out, 1);
    assert_int_equal(ddr_cper.error_type, DDRSS_CPER_INTU_PLL_INTERRUPT);

    intr_event = DDRSS_INTU_PCR_PAR_ERR;
    memset(&ddr_cper, 0, sizeof(ddr_cper));
    status = prod_ddrss_get_intr_event_cper(mc, intr_event, &ddr_cper);
    assert_int_equal(status, SILIBS_SUCCESS);
    assert_int_equal(ddr_cper.vendor_err_info.misc.ddr_pcr_par_err, 1);
    assert_int_equal(ddr_cper.vendor_err_info.misc.valid_ddr_pcr_par_err, 1);
    assert_int_equal(ddr_cper.error_type, DDRSS_CPER_INTU_PCR_PAR_ERR);

    intr_event = DDRSS_INTU_INTU_PAR_ERR;
    memset(&ddr_cper, 0, sizeof(ddr_cper));
    status = prod_ddrss_get_intr_event_cper(mc, intr_event, &ddr_cper);
    assert_int_equal(status, SILIBS_SUCCESS);
    assert_int_equal(ddr_cper.vendor_err_info.misc.ddr_intu_par_err, 1);
    assert_int_equal(ddr_cper.vendor_err_info.misc.valid_ddr_intu_par_err, 1);
    assert_int_equal(ddr_cper.error_type, DDRSS_CPER_INTU_PAR_ERR);

    intr_event = 0xFF;
    memset(&ddr_cper, 0, sizeof(ddr_cper));
    status = prod_ddrss_get_intr_event_cper(mc, intr_event, &ddr_cper);
    assert_int_equal(status, SILIBS_E_DEVICE);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_pending, setup, teardown)
{
    KNG_DIE_ID test_die = (KNG_DIE_ID)0;

    // Set up die id for test
    idsw_set_die_id(test_die);

    // Test case 1: No interrupt pending
    g_ddr_intu_sts = 0;
    g_intu_enable = 0xFFFFFFFF;

    bool result = prod_ddrss_interrupt_pending((void*)&ddrss_num[0]);
    assert_int_equal(result, false);

    // Test case 2: Interrupt pending but masked
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_CRI_INT);
    g_intu_enable = 0;

    result = prod_ddrss_interrupt_pending((void*)&ddrss_num[1]);
    assert_int_equal(result, false);

    // Test case 3: Interrupt pending and enabled
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_CRI_INT);
    g_intu_enable = 0xFFFFFFFF;

    result = prod_ddrss_interrupt_pending((void*)&ddrss_num[2]);
    assert_int_equal(result, true);

    // Test case 4: Multiple interrupts with some enabled
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_CRI_INT) | (1 << DDRSS_INTU_PHY_IRQ);
    g_intu_enable = (1 << DDRSS_INTU_PHY_IRQ);

    result = prod_ddrss_interrupt_pending((void*)&ddrss_num[3]);
    assert_int_equal(result, true);

    // Test case 5: Test with different die ID
    test_die = (KNG_DIE_ID)1;
    idsw_set_die_id(test_die);
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC1_CRI_INT);
    g_intu_enable = 0xFFFFFFFF;

    result = prod_ddrss_interrupt_pending((void*)&ddrss_num[4]);
    assert_int_equal(result, true);
}
}