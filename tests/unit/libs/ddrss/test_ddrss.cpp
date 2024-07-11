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
#include <ddrss.h>
#include <ddrss_intu.h>
#include <ddrss_lib.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <nvic.h>
#include <silibs_ap_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static const int HW_INT_DDRSS0(106);
static const int HW_INT_DDRSS5(111);

extern uint32_t g_ddr_intu_sts;
extern uint32_t g_intu_enable;
extern uint32_t g_phy_int_sts;
extern uint32_t g_mc_intu_sts;
extern uint32_t g_mc_intu_dest_enable;
extern bool g_mmio_read32_mocktype;

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
TEST_FUNCTION(test_prod_ddrss_lib_init_skip, setup, teardown)
{

    KNG_DIE_ID test_die = (KNG_DIE_ID)0;
    ddrss_isr_params_t params = {.die_id = test_die, .ddrss_num = 0};

    // ddrss init is skipped on svp, therefore no expectations
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    for (int this_irq_num = HW_INT_DDRSS0; this_irq_num <= HW_INT_DDRSS5; this_irq_num++)
    {
        params.ddrss_num = (this_irq_num - HW_INT_DDRSS0);

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, prod_ddrss_interrupt_handler);
        expect_memory(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)&params, sizeof(params));

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }
    prod_ddrss_lib_init(test_die);

    // ddrss init is skipped if not on an FPGA, therefore no expectations
    idsw_set_platform_sdv(PLATFORM_UNDEFINED);
    for (int this_irq_num = HW_INT_DDRSS0; this_irq_num <= HW_INT_DDRSS5; this_irq_num++)
    {
        params.ddrss_num = (this_irq_num - HW_INT_DDRSS0);

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, prod_ddrss_interrupt_handler);
        expect_memory(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)&params, sizeof(params));

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }
    prod_ddrss_lib_init(test_die);
}

TEST_FUNCTION(test_ddrss_lib_init_fpga, setup, teardown)
{

    KNG_DIE_ID test_die = (KNG_DIE_ID)1;
    ddrss_isr_params_t params = {.die_id = test_die, .ddrss_num = 0};

    // ddrss init is not skipped on the Big FPGA, and the
    // atu map is fixed so no atu map / un map calls
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    for (int this_irq_num = HW_INT_DDRSS0; this_irq_num <= HW_INT_DDRSS5; this_irq_num++)
    {
        params.ddrss_num = (this_irq_num - HW_INT_DDRSS0);

        // FPFwCoreInterruptRegisterCallback
        expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_set_isr_with_param, isr, prod_ddrss_interrupt_handler);
        expect_memory(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)&params, sizeof(params));

        // FPFwCoreInterruptEnableVector
        expect_value(__wrap_nvic_irq_clear_pending, irq_num, this_irq_num);
        expect_value(__wrap_nvic_irq_enable, irq_num, this_irq_num);
    }

    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_ddrss_init, SILIBS_SUCCESS);
    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    prod_ddrss_lib_init(test_die);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_unexpected_interrupt, setup, teardown)
{
    ddrss_isr_params_t params = {.die_id = DIE_0, .ddrss_num = 3};
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_HSP_INT); // This is unexpected
    g_intu_enable = 0xFFFFFFFF;                     // This is a mask
    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_MC0_HSP_INT));
    prod_ddrss_interrupt_handler((void*)&params);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_MC0_CRI_INT, setup, teardown)
{
    ddrss_isr_params_t params = {.die_id = DIE_0, .ddrss_num = 1};

    // Test MC0_CRI_INT
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_CRI_INT);
    g_intu_enable = 0xFFFFFFFF; // This is a mask
    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_MC0_CRI_INT));
    will_return(__wrap_ddrss_probe_ras_agent, SILIBS_E_PARAM);
    prod_ddrss_interrupt_handler((void*)&params);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_MC1_CRI_INT, setup, teardown)
{
    ddrss_isr_params_t params = {.die_id = DIE_0, .ddrss_num = 1};

    // Test MC1_CRI_INT
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC1_CRI_INT);
    g_intu_enable = 0xFFFFFFFF; // This is a mask
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_E_PARAM);
    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_MC1_CRI_INT));
    prod_ddrss_interrupt_handler((void*)&params);

    // Test MC1_CRI_INT
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC1_CRI_INT);
    g_intu_enable = 0xFFFFFFFF; // This is a mask
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    expect_function_call(__wrap_ras_arm_agent_probe);
    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_MC1_CRI_INT));
    prod_ddrss_interrupt_handler((void*)&params);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_DDRSS_INTU_SRA_ERI, setup, teardown)
{
    g_mmio_read32_mocktype = true;
    ddrss_isr_params_t params = {.die_id = DIE_0, .ddrss_num = 1};

    // Test DDRSS_INTU_SRA_ERI
    g_ddr_intu_sts = (1 << DDRSS_INTU_SRA_ERI);
    g_intu_enable = 0xFFFFFFFF; // This is a mask

    will_return(__wrap_mmio_read32, 1); // This is the (non-zero) mock return value for the MMIO_READ32
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_E_PARAM);
    will_return(__wrap_mmio_read32, 1); // This is the (non-zero) mock return value for the second MMIO_READ32
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_E_PARAM);
    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_SRA_ERI));
    prod_ddrss_interrupt_handler((void*)&params);

    // Test DDRSS_INTU_SRA_ERI - other path
    g_ddr_intu_sts = (1 << DDRSS_INTU_SRA_ERI);
    g_intu_enable = 0xFFFFFFFF; // This is a mask

    will_return(__wrap_mmio_read32, 1); // This is the (non-zero) mock return value for the MMIO_READ32
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    expect_function_call(__wrap_ras_arm_agent_probe);
    will_return(__wrap_mmio_read32, 1); // This is the (non-zero) mock return value for the second MMIO_READ32
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    expect_function_call(__wrap_ras_arm_agent_probe);
    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, (1 << DDRSS_INTU_SRA_ERI));
    prod_ddrss_interrupt_handler((void*)&params);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_phy, setup, teardown)
{
    ddrss_isr_params_t params = {.die_id = DIE_0, .ddrss_num = 3};

    // Test PHY interrupts
    g_ddr_intu_sts = (1 << DDRSS_INTU_PHY_IRQ);
    g_intu_enable = 0xFFFFFFFF; // This is a mask
    g_phy_int_sts = csr_PhyAcsmParityErrEn_MASK | csr_PhyPIEParityErrEn_MASK | csr_PhyRdfPtrChkErrEn_MASK |
                    csr_PhyEccEn_MASK | csr_PhyPIEProgErrEn_MASK | csr_PhyTxPPTEn_MASK | csr_PhyAlertEn_MASK;
    expect_value(__wrap_ddrss_clear_phy_interrupt_status, phy_int_sts, g_phy_int_sts);
    expect_any_always(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask);
    prod_ddrss_interrupt_handler((void*)&params);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_mc, setup, teardown)
{
    ddrss_isr_params_t params = {.die_id = DIE_0, .ddrss_num = 3};

    // DDRSS_INTU_MC0_SCP_INT
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_SCP_INT);
    g_intu_enable = 0xFFFFFFFF;         // This is a mask
    g_mc_intu_dest_enable = 0xFFFFFFFF; // So is this
    g_mc_intu_sts = (1 << DDRSS_INTU_MC_FEDFLUSHDONE) | (1 << DDRSS_INTU_MC_RMTELEMETRYAVAIL);

    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, g_ddr_intu_sts);
    prod_ddrss_interrupt_handler((void*)&params);
}

TEST_FUNCTION(test_prod_ddrss_interrupt_handler_others, setup, teardown)
{
    ddrss_isr_params_t params = {.die_id = DIE_0, .ddrss_num = 3};
    g_ddr_intu_sts = (1 << DDRSS_INTU_MC0_HSP_INT) | (1 << DDRSS_INTU_PLL_INTERRUPT_OUT) |
                     (1 << DDRSS_INTU_PCR_PAR_ERR) | (1 << DDRSS_INTU_INTU_PAR_ERR);
    g_intu_enable = 0xFFFFFFFF; // This is a mask
    expect_value(__wrap_ddrss_ddr_intu_clear_interrupt, intr_mask, g_ddr_intu_sts);
    prod_ddrss_interrupt_handler((void*)&params);
}
}