//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_i3c_controller.cpp
 * i3c_controller tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, check_expected, CmockaWra...

extern "C" {
#include "dw_i3c.h"                  // for i3c_instance_t, dat_entry_t
#include "silibs_scp_exp_top_regs.h" // for SCP_EXP_TOP_I3C_0_ADDRESS, SCP_...
#include "silibs_scp_top_regs.h"     // for SCP_TOP_SCP_EXP_ADDRESS

#include <FPFwInterrupts.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <ddr_i3c.h>
#include <i3c_controller.h> // for cmn800_sequence_params_t
#include <idsw.h>           // for idsw_set_platform_sdv, PLATFORM_UN...
#include <idsw_kng.h>       // for KNG_DIE_ID, _KNG_PLAT_ID
#include <interrupts.h>
#include <kng_soc_constants.h> // for NUM_DIE
#include <mscp_exp_spi_synchronize_dies.h>
#include <nvic.h>
#include <stdint.h> // for int32_t, uint32_t

/*-- Symbolic Constant Macros (defines) --*/
#define SCP_I3C0_CSR_ADDRESS SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_I3C_0_ADDRESS
#define SCP_I3C1_CSR_ADDRESS SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_I3C_1_ADDRESS

#define RETURN_I3C_FAIL        (-1)
#define RETURN_I3C_SUCCESS     (SILIBS_SUCCESS)
#define BUGCHECK_MOCK_RETURN() (setjmp(cd_mock_jump_buf))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
int return_i3c_initialize = RETURN_I3C_SUCCESS;
int return_i3c_master_dat_config = RETURN_I3C_SUCCESS;
int return_i3c_master_set_aasa = RETURN_I3C_SUCCESS;
bool simulate_single_die = true;
uint8_t g_die_num = SOC_D0;
uint8_t g_dimm_cap_per_ch = DIMM_40_BIT_CH_32GB;
uint8_t g_dimm_sku = DDR5_RDIMM_2Rx4_16Gb_64GB;
uint8_t mock_ddrss_index = 0xFF;
uint8_t mock_dimm_capacity = 0xFF;
uint8_t mock_dimm_sku = 0xFF;

static jmp_buf cd_mock_jump_buf;

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Handle noreturn, allowing control to return to test
    longjmp(cd_mock_jump_buf, 1);
}

//
// Setup and Teardown Functions
//
static int setup_svp_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    simulate_single_die = true;
    return 0;
}

static int setup_svp_platform_dual_die(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    simulate_single_die = false;
    return 0;
}

static int setup_fpga_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    simulate_single_die = true;
    return 0;
}

static int setup_fpga_platform_dual_die(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    simulate_single_die = false;
    return 0;
}

static int setup_fpga_rvp_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE_RVP);
    simulate_single_die = true;
    return 0;
}

static int setup_fpga_rvp_platform_dual_die(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE_RVP);
    simulate_single_die = false;
    return 0;
}

static int setup_soc_platform_dual_die(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_RVP_EVT_SILICON);
    simulate_single_die = false;
    return 0;
}

static int setup_undefined_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_UNDEFINED);
    return_i3c_initialize = RETURN_I3C_SUCCESS;
    return_i3c_master_dat_config = RETURN_I3C_SUCCESS;
    return_i3c_master_set_aasa = RETURN_I3C_SUCCESS;
    simulate_single_die = false;
    g_die_num = SOC_D0;
    mock_ddrss_index = 0xFF;
    mock_dimm_capacity = 0xFF;
    mock_dimm_sku = 0xFF;
    return 0;
}

//
// Mocks
//
int __wrap_i3c_initialize(i3c_instance_t* instance, i3c_config_t* i3c_config)
{
    check_expected_ptr(instance);
    check_expected(i3c_config->register_base_addr);
    check_expected(i3c_config->instance_type);
    check_expected(i3c_config->address);
    check_expected(i3c_config->index);
    check_expected(i3c_config->i3c_core_clk_freq_in_mhz);
    check_expected(i3c_config->i3c_speed_in_khz);
    function_called();
    return return_i3c_initialize;
}

int __wrap_i3c_register_notification_callback(i3c_instance_t* instance, i3c_notification_callback callback, void* context)
{
    check_expected_ptr(instance);
    check_expected(callback);
    check_expected(context);

    function_called();
    return 0;
}

int __wrap_i3c_master_dat_config(i3c_instance_t* instance, const dat_entry_t* config_table, uint8_t config_table_length)
{
    check_expected_ptr(instance);
    assert_non_null(config_table);
    check_expected(config_table_length);

    function_called();
    return return_i3c_master_dat_config;
}

int __wrap_i3c_master_set_aasa(i3c_instance_t* instance, uint8_t i3c_speed, i3c_cmd_callback callback, void* context)
{
    check_expected_ptr(instance);
    check_expected(i3c_speed);
    check_expected(callback);
    check_expected(context);

    function_called();
    return return_i3c_master_set_aasa;
}

void __wrap_i3c_master_register_ibi_handler(i3c_instance_t* instance, i3c_master_ibi_received_callback callback, void* context)
{
    check_expected_ptr(instance);
    check_expected(callback);
    check_expected(context);

    function_called();
}

nvic_status_t __wrap_nvic_irq_set_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* parameter)
{
    check_expected(irq_num);
    check_expected(isr);
    FPFW_UNUSED(parameter);

    return (NVIC_STATUS_SUCCESS);
}

nvic_status_t __wrap_nvic_irq_clear_pending(uint32_t irq_num)
{
    check_expected(irq_num);
    return (NVIC_STATUS_SUCCESS);
}

nvic_status_t __wrap_nvic_irq_enable(uint32_t irq_num)
{
    check_expected(irq_num);
    return (NVIC_STATUS_SUCCESS);
}

void __wrap_ddr_i3c_interface_set_instance(i3c_instance_t* instance_0, i3c_instance_t* instance_1)
{
    check_expected_ptr(instance_0);
    check_expected_ptr(instance_1);
    function_called();
}

int32_t __wrap_ddr_i3c_interface_power_up_pmic(i3c_instance_t* instance, i3c_cmd_t* s_i3c_cmd)
{
    FPFW_UNUSED(s_i3c_cmd);
    check_expected_ptr(instance);
    function_called();
    return 0;
}

int32_t __wrap_ddr_i3c_interface_power_up_pmic_on(i3c_instance_t* instance, i3c_cmd_t* s_i3c_cmd)
{
    FPFW_UNUSED(s_i3c_cmd);
    check_expected_ptr(instance);
    function_called();
    return 0;
}

int32_t __wrap_ddr_i3c_interface_read_dimms_detected(i3c_cmd_t* s_i3c_cmd, uint32_t* ddrss_en)
{
    FPFW_UNUSED(s_i3c_cmd);
    *ddrss_en = (g_die_num == SOC_D1) ? 0xFC0 : 0x3F;
    function_called();
    return mock_type(int);
}

int __wrap_mscp_exp_spi_invalidate_region(int die_id)
{
    check_expected(die_id);
    function_called();
    return 0;
}

int __wrap_mscp_exp_spi_write_d0_to_d1_data(mscp_exp_spi_sync_point_data_t* sync_point_data, int die_id)
{
    check_expected_ptr(sync_point_data);
    check_expected(die_id);
    function_called();
    return 0;
}

int __wrap_mscp_exp_spi_read_d1_to_d0_data(mscp_exp_spi_sync_point_data_t* sync_point_data, int die_id)
{
    check_expected_ptr(sync_point_data);
    check_expected(die_id);
    function_called();
    return 0;
}

int __wrap_mscp_exp_spi_write_d1_to_d0_data(mscp_exp_spi_sync_point_data_t* sync_point_data, int die_id)
{
    check_expected_ptr(sync_point_data);
    check_expected(die_id);
    function_called();
    return 0;
}

int __wrap_mscp_exp_spi_read_d0_to_d1_data(mscp_exp_spi_sync_point_data_t* sync_point_data, int die_id)
{
    check_expected_ptr(sync_point_data);
    check_expected(die_id);
    function_called();
    return 0;
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    function_called();
    return simulate_single_die;
}

int32_t __wrap_ddr_i3c_interface_read_dimm_capacity(i3c_cmd_t* s_i3c_cmd, uint8_t ddrss_index, uint8_t* data, uint8_t* dimm_sku)
{
    assert_non_null(s_i3c_cmd);
    FPFW_UNUSED(ddrss_index);
    assert_non_null(data);
    assert_non_null(dimm_sku);
    function_called();
    if ((mock_dimm_capacity != 0xFF) && (mock_ddrss_index == ddrss_index))
    {
        *data = mock_dimm_capacity;
    }

    if ((mock_dimm_sku != 0xFF) && (mock_ddrss_index == ddrss_index))
    {
        *dimm_sku = mock_dimm_sku;
    }

    return mock_type(int);
}

void __wrap_i3c_master_set_cfg_knobs(lib_i3c_cfg_t* p_lib_i3c_cfg)
{
    check_expected(p_lib_i3c_cfg->i3c_speed);
    check_expected(p_lib_i3c_cfg->reg_scl_ext_termn_lcnt_timing);
    check_expected(p_lib_i3c_cfg->reg_sda_hold_switch_dly_timing);
    check_expected(p_lib_i3c_cfg->timing_override);
    check_expected(p_lib_i3c_cfg->reg_scl_i3c_od_timing);
    check_expected(p_lib_i3c_cfg->reg_scl_i3c_pp_timing);
    check_expected(p_lib_i3c_cfg->reg_scl_i2c_fm_timing);
    check_expected(p_lib_i3c_cfg->reg_scl_i2c_fmp_timing);
    check_expected(p_lib_i3c_cfg->reg_scl_ext_lcnt_timing);
    check_expected(p_lib_i3c_cfg->vdd);
    check_expected(p_lib_i3c_cfg->vddq);
    function_called();
}

void i3c_master_set_cfg_knobs_default_expect(void)
{
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->i3c_speed, I3C_SPEED_SDR4);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->reg_scl_ext_termn_lcnt_timing, 0xF);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->reg_sda_hold_switch_dly_timing, 0x50000);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->timing_override, false);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->reg_scl_i3c_od_timing, 0xA0010);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->reg_scl_i3c_pp_timing, 0xA000A);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->reg_scl_i2c_fm_timing, 0x100010);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->reg_scl_i2c_fmp_timing, 0x100010);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->reg_scl_ext_lcnt_timing, 0x20202020);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->vdd, 0x0);
    expect_value(__wrap_i3c_master_set_cfg_knobs, p_lib_i3c_cfg->vddq, 0x0);
}

// Tests
// Test when i3c_initialize fails for Instance 0
TEST_FUNCTION(test_i3c_controller_svp_die_0_i3c0_initialize_fail, setup_svp_platform, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    uint8_t index_i3c0 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };

    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);

    // Set up expectations
    return_i3c_initialize = RETURN_I3C_FAIL;
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, return_i3c_initialize);
}

// Test when i3c_master_dat_config fails for Instance 0
TEST_FUNCTION(test_i3c_controller_svp_die_0_i3c0_master_dat_config_fail, setup_svp_platform, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    uint8_t index_i3c0 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };

    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);

    // Set up expectations
    // Instance 0
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    return_i3c_master_dat_config = RETURN_I3C_FAIL;
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, return_i3c_master_dat_config);
}

// Test when i3c_master_set_aasa fails for Instance 0
TEST_FUNCTION(test_i3c_controller_svp_die_0_i3c0_master_set_aasa_fail, setup_svp_platform, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    index_i3c1 = KNG_SOC_DIE_0_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };

    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);

    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    return_i3c_master_set_aasa = RETURN_I3C_FAIL;
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, return_i3c_master_set_aasa);
}

// Die-0 SVP Boot Single-Die
TEST_FUNCTION(test_i3c_controller_svp_die_0_single_die, setup_svp_platform, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    index_i3c1 = KNG_SOC_DIE_0_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-0 SVP Boot Dual-Die
TEST_FUNCTION(test_i3c_controller_svp_die_0_dual_die, setup_svp_platform_dual_die, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    index_i3c1 = KNG_SOC_DIE_0_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // mscp_exp_spi_invalidate_region
    expect_value(__wrap_mscp_exp_spi_invalidate_region, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_invalidate_region);

    // mscp_exp_spi_read_d1_to_d0_data
    i3c_test_sync.data_d1_to_d0_data = (die_num == SOC_D0) ? (0xFC0) : (0x3F);
    i3c_test_sync.data_d1_to_d0_data |= ((g_dimm_sku & MASK_DIMM_SKU) << SHIFT_DIMM_SKU) |
                                        ((g_dimm_cap_per_ch & MASK_DIMM_CAP) << SHIFT_DIMM_CAP);
    i3c_test_sync.data_ack_d1_to_d0_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_read_d1_to_d0_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_read_d1_to_d0_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_read_d1_to_d0_data);

    // mscp_exp_spi_write_d0_to_d1_data
    i3c_test_sync.data_d0_to_d1_data = (die_num == SOC_D0) ? (0x3F) : (0xFC0);
    i3c_test_sync.data_d0_to_d1_data |= ((g_dimm_sku & MASK_DIMM_SKU) << SHIFT_DIMM_SKU) |
                                        ((g_dimm_cap_per_ch & MASK_DIMM_CAP) << SHIFT_DIMM_CAP);
    i3c_test_sync.data_ack_d0_to_d1_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_write_d0_to_d1_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_write_d0_to_d1_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_write_d0_to_d1_data);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-1 SVP Boot Dual-Die
TEST_FUNCTION(test_i3c_controller_svp_die_1, setup_svp_platform_dual_die, setup_undefined_platform)
{
    uint8_t die_num = SOC_D1;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_1_I3C0;
    index_i3c1 = KNG_SOC_DIE_1_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // mscp_exp_spi_invalidate_region
    expect_value(__wrap_mscp_exp_spi_invalidate_region, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_invalidate_region);

    // mscp_exp_spi_write_d1_to_d0_data
    i3c_test_sync.data_d0_to_d1_data = (die_num == SOC_D0) ? (0x3F) : (0xFC0);
    i3c_test_sync.data_d0_to_d1_data |= ((g_dimm_sku & MASK_DIMM_SKU) << SHIFT_DIMM_SKU) |
                                        ((g_dimm_cap_per_ch & MASK_DIMM_CAP) << SHIFT_DIMM_CAP);
    i3c_test_sync.data_ack_d0_to_d1_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_write_d1_to_d0_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_write_d1_to_d0_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_write_d1_to_d0_data);

    // mscp_exp_spi_read_d0_to_d1_data
    i3c_test_sync.data_d1_to_d0_data = (die_num == SOC_D0) ? (0xFC0) : (0x3F);
    i3c_test_sync.data_d1_to_d0_data |= ((g_dimm_sku & MASK_DIMM_SKU) << SHIFT_DIMM_SKU) |
                                        ((g_dimm_cap_per_ch & MASK_DIMM_CAP) << SHIFT_DIMM_CAP);
    expect_value(__wrap_mscp_exp_spi_read_d0_to_d1_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_read_d0_to_d1_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_read_d0_to_d1_data);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-0 FPGA Boot Single-Die
TEST_FUNCTION(test_i3c_controller_fpga_die_0_single_die, setup_fpga_platform, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    index_i3c1 = KNG_SOC_DIE_0_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, NULL);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-0 FPGA Boot Dual-Die
TEST_FUNCTION(test_i3c_controller_fpga_die_0_dual_die, setup_fpga_platform_dual_die, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    index_i3c1 = KNG_SOC_DIE_0_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, NULL);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // mscp_exp_spi_invalidate_region
    expect_value(__wrap_mscp_exp_spi_invalidate_region, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_invalidate_region);

    // mscp_exp_spi_read_d1_to_d0_data
    i3c_test_sync.data_d1_to_d0_data = 0x0;
    expect_value(__wrap_mscp_exp_spi_read_d1_to_d0_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_read_d1_to_d0_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_read_d1_to_d0_data);

    // mscp_exp_spi_write_d0_to_d1_data
    i3c_test_sync.data_d0_to_d1_data = 0x0;
    i3c_test_sync.data_ack_d0_to_d1_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_write_d0_to_d1_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_write_d0_to_d1_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_write_d0_to_d1_data);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-1 FPGA Boot Dual-Die
TEST_FUNCTION(test_i3c_controller_fpga_die_1_dual_die, setup_fpga_platform_dual_die, setup_undefined_platform)
{
    uint8_t die_num = SOC_D1;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_1_I3C0;
    index_i3c1 = KNG_SOC_DIE_1_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, NULL);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // mscp_exp_spi_invalidate_region
    expect_value(__wrap_mscp_exp_spi_invalidate_region, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_invalidate_region);

    // mscp_exp_spi_write_d1_to_d0_data
    i3c_test_sync.data_d1_to_d0_data = 0x0;
    i3c_test_sync.data_ack_d1_to_d0_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_write_d1_to_d0_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_write_d1_to_d0_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_write_d1_to_d0_data);

    // mscp_exp_spi_read_d0_to_d1_data
    i3c_test_sync.data_d0_to_d1_data = 0x0;
    i3c_test_sync.data_ack_d0_to_d1_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_read_d0_to_d1_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_read_d0_to_d1_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_read_d0_to_d1_data);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-0 FPGA RVP Boot Single-Die
TEST_FUNCTION(test_i3c_controller_fpga_rvp_die_0_single_die, setup_fpga_rvp_platform, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    index_i3c1 = KNG_SOC_DIE_0_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-0 FPGA RVP Boot Dual-Die
TEST_FUNCTION(test_i3c_controller_fpga_rvp_die_0_dual_die, setup_fpga_rvp_platform_dual_die, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    index_i3c1 = KNG_SOC_DIE_0_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // mscp_exp_spi_invalidate_region
    expect_value(__wrap_mscp_exp_spi_invalidate_region, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_invalidate_region);

    // mscp_exp_spi_read_d1_to_d0_data
    expect_value(__wrap_mscp_exp_spi_read_d1_to_d0_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_read_d1_to_d0_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_read_d1_to_d0_data);

    // mscp_exp_spi_write_d0_to_d1_data
    i3c_test_sync.data_d0_to_d1_data = 0x0;
    i3c_test_sync.data_ack_d0_to_d1_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_write_d0_to_d1_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_write_d0_to_d1_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_write_d0_to_d1_data);

    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-1 FPGA Boot Single-Die
TEST_FUNCTION(test_i3c_controller_fpga_die_1, setup_fpga_platform, setup_undefined_platform)
{
    uint8_t die_num = SOC_D1;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_1_I3C0;
    index_i3c1 = KNG_SOC_DIE_1_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, NULL);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-1 FPGA RVP Boot Single-Die
TEST_FUNCTION(test_i3c_controller_fpga_rvp_die_1, setup_fpga_rvp_platform, setup_undefined_platform)
{
    uint8_t die_num = SOC_D1;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_1_I3C0;
    index_i3c1 = KNG_SOC_DIE_1_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-1 FPGA RVP Boot Dual-Die
TEST_FUNCTION(test_i3c_controller_fpga_rvp_die_1_dual_die, setup_fpga_rvp_platform_dual_die, setup_undefined_platform)
{
    uint8_t die_num = SOC_D1;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_1_I3C0;
    index_i3c1 = KNG_SOC_DIE_1_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // mscp_exp_spi_invalidate_region
    expect_value(__wrap_mscp_exp_spi_invalidate_region, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_invalidate_region);

    // mscp_exp_spi_write_d1_to_d0_data
    i3c_test_sync.data_d0_to_d1_data = 0x0;
    i3c_test_sync.data_ack_d0_to_d1_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_write_d1_to_d0_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_write_d1_to_d0_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_write_d1_to_d0_data);

    // mscp_exp_spi_read_d0_to_d1_data
    expect_value(__wrap_mscp_exp_spi_read_d0_to_d1_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_read_d0_to_d1_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_read_d0_to_d1_data);

    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-0 SoC Boot Dual-Die
TEST_FUNCTION(test_i3c_controller_soc_die_0_dual_die, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint8_t die_num = SOC_D0;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    index_i3c1 = KNG_SOC_DIE_0_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    uint32_t ddrss_en = 0x3F;
    uint8_t ddrss_index = 0x0;
    for (ddrss_index = 0; ddrss_index < DDRSS_DIMM_MAX; ddrss_index++)
    {
        // Convert the ddrss_en which is 1-hot encoding to ddrss_index which is a enum
        if (ddrss_en & (1 << ddrss_index))
        {
            will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);
        }
    }
    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // mscp_exp_spi_invalidate_region
    expect_value(__wrap_mscp_exp_spi_invalidate_region, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_invalidate_region);

    // mscp_exp_spi_read_d1_to_d0_data
    i3c_test_sync.data_d1_to_d0_data = (die_num == SOC_D0) ? 0xFC0 : 0x3C;
    i3c_test_sync.data_ack_d1_to_d0_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_read_d1_to_d0_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_read_d1_to_d0_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_read_d1_to_d0_data);

    // mscp_exp_spi_write_d0_to_d1_data
    i3c_test_sync.data_d0_to_d1_data = (die_num == SOC_D0) ? 0x3C : 0xFC0;
    i3c_test_sync.data_ack_d0_to_d1_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_write_d0_to_d1_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_write_d0_to_d1_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_write_d0_to_d1_data);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-1 SoC Boot Dual-Die
TEST_FUNCTION(test_i3c_controller_soc_die_1_dual_die, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint8_t die_num = SOC_D1;
    g_die_num = die_num;
    i3c_instance_t* instance_0 = get_i3c0();
    i3c_instance_t* instance_1 = get_i3c1();
    uint8_t index_i3c0 = 0x0;
    uint8_t index_i3c1 = 0x0;
    index_i3c0 = KNG_SOC_DIE_1_I3C0;
    index_i3c1 = KNG_SOC_DIE_1_I3C1;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_SDR4_KHZ,
    };
    // Set up expectations
    // Instance 0
    i3c_master_set_cfg_knobs_default_expect();
    expect_function_call(__wrap_i3c_master_set_cfg_knobs);
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_0);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config0.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config0.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config0.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config0.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config0.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config0.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_0);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c0_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_0);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Instance 1
    // i3c_initialize
    expect_value(__wrap_i3c_initialize, instance, instance_1);
    expect_value(__wrap_i3c_initialize, i3c_config->register_base_addr, i3c_config1.register_base_addr);
    expect_value(__wrap_i3c_initialize, i3c_config->instance_type, i3c_config1.instance_type);
    expect_value(__wrap_i3c_initialize, i3c_config->address, i3c_config1.address);
    expect_value(__wrap_i3c_initialize, i3c_config->index, i3c_config1.index);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_core_clk_freq_in_mhz, i3c_config1.i3c_core_clk_freq_in_mhz);
    expect_value(__wrap_i3c_initialize, i3c_config->i3c_speed_in_khz, i3c_config1.i3c_speed_in_khz);
    expect_function_call(__wrap_i3c_initialize);

    // i3c_register_notification_callback
    expect_value(__wrap_i3c_register_notification_callback, instance, instance_1);
    expect_any(__wrap_i3c_register_notification_callback, callback);
    expect_any(__wrap_i3c_register_notification_callback, context);
    expect_function_call(__wrap_i3c_register_notification_callback);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)i3c1_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_TARGET_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // ddr_i3c_interface_set_instance
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_0, instance_0);
    expect_value(__wrap_ddr_i3c_interface_set_instance, instance_1, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_set_instance);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_0);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_power_up_pmic_on
    expect_value(__wrap_ddr_i3c_interface_power_up_pmic_on, instance, instance_1);
    expect_function_call(__wrap_ddr_i3c_interface_power_up_pmic_on);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // ddr_i3c_interface_read_dimms_detected
    will_return(__wrap_ddr_i3c_interface_read_dimms_detected, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimms_detected);

    uint32_t ddrss_en = 0x3F;
    uint8_t ddrss_index = 0x0;
    for (ddrss_index = 0; ddrss_index < DDRSS_DIMM_MAX; ddrss_index++)
    {
        // Convert the ddrss_en which is 1-hot encoding to ddrss_index which is a enum
        if (ddrss_en & (1 << ddrss_index))
        {
            will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);
        }
    }
    // ddr_i3c_interface_read_dimm_capacity
    will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
    expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);

    // idhw_is_single_die_boot_en
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    // mscp_exp_spi_invalidate_region
    expect_value(__wrap_mscp_exp_spi_invalidate_region, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_invalidate_region);

    // mscp_exp_spi_write_d1_to_d0_data
    i3c_test_sync.data_d1_to_d0_data = (die_num == SOC_D1) ? 0xFC0 : 0x3C;
    i3c_test_sync.data_ack_d1_to_d0_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_write_d1_to_d0_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_write_d1_to_d0_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_write_d1_to_d0_data);

    // mscp_exp_spi_read_d0_to_d1_data
    i3c_test_sync.data_d0_to_d1_data = (die_num == SOC_D1) ? 0x3C : 0xFC0;
    i3c_test_sync.data_ack_d0_to_d1_data = SPI_SYNC_DATA_VALID;
    expect_value(__wrap_mscp_exp_spi_read_d0_to_d1_data, sync_point_data, &i3c_test_sync);
    expect_value(__wrap_mscp_exp_spi_read_d0_to_d1_data, die_id, die_num);
    expect_function_call(__wrap_mscp_exp_spi_read_d0_to_d1_data);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Test for verifying the dimm capacity on die 0
TEST_FUNCTION(test_i3c_controller_soc_die_0_verify_dimm_fail, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint32_t ddrss_en = 0xF000;

    // Call the function under test
    int status = i3c_controller_verify_dimm_on_current_die(ddrss_en);
    assert_int_equal(status, SILIBS_E_NOMEM);
}

// Test for verifying the dimm capacity on die 0 on SVP and it should succeed
TEST_FUNCTION(test_i3c_controller_svp_die_0_verify_dimm, setup_svp_platform_dual_die, setup_undefined_platform)
{
    uint32_t ddrss_en = 0xF000;

    // Call the function under test
    int status = i3c_controller_verify_dimm_on_current_die(ddrss_en);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Test for verifying the dimm capacity read on die 0 failed
TEST_FUNCTION(test_i3c_controller_soc_die_0_verify_dimm_capacity_Read_Fail, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint32_t ddrss_en = 0x3F;
    uint8_t ddrss_index = 0x0;
    for (ddrss_index = 0; ddrss_index < DDRSS_DIMM_MAX; ddrss_index++)
    {
        // Convert the ddrss_en which is 1-hot encoding to ddrss_index which is a enum
        if (ddrss_en & (1 << ddrss_index))
        {
            if (ddrss_index == 5)
            {
                will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_E_NOMEM);
            }
            else
            {
                will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
            }
            expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);
        }
    }
    // Call the function under test
    if (!BUGCHECK_MOCK_RETURN())
    {
        int status = i3c_controller_verify_dimm_on_current_die(ddrss_en);
        assert_int_equal(status, SILIBS_SUCCESS);
    }
}

// Test for verifying the dimm capacity Check on die 0 failed
TEST_FUNCTION(test_i3c_controller_soc_die_0_verify_dimm_capacity_Check_Fail, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint32_t ddrss_en = 0x3F;
    uint8_t ddrss_index = 0x0;
    for (ddrss_index = 0; ddrss_index < DDRSS_DIMM_MAX; ddrss_index++)
    {
        // Convert the ddrss_en which is 1-hot encoding to ddrss_index which is a enum
        if (ddrss_en & (1 << ddrss_index))
        {
            if (ddrss_index == 5)
            {
                mock_ddrss_index = ddrss_index;
                mock_dimm_capacity = 0xFE;
            }
            will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);
        }
    }
    // Call the function under test
    int status = i3c_controller_verify_dimm_on_current_die(ddrss_en);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Test for verifying the dimm sku Check on die 0 failed
TEST_FUNCTION(test_i3c_controller_soc_die_0_verify_dimm_sku_Check_Fail, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint32_t ddrss_en = 0x3F;
    uint8_t ddrss_index = 0x0;
    for (ddrss_index = 0; ddrss_index < DDRSS_DIMM_MAX; ddrss_index++)
    {
        // Convert the ddrss_en which is 1-hot encoding to ddrss_index which is a enum
        if (ddrss_en & (1 << ddrss_index))
        {
            if (ddrss_index == 1)
            {
                mock_ddrss_index = ddrss_index;
                mock_dimm_sku = 0xFE;
            }
            will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);
        }
    }
    // Call the function under test
    int status = i3c_controller_verify_dimm_on_current_die(ddrss_en);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Test for verifying the dimm capacity on die 0
TEST_FUNCTION(test_i3c_controller_soc_die_0_verify_dimm, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint32_t ddrss_en = 0x3F;
    uint8_t ddrss_index = 0x0;
    for (ddrss_index = 0; ddrss_index < DDRSS_DIMM_MAX; ddrss_index++)
    {
        // Convert the ddrss_en which is 1-hot encoding to ddrss_index which is a enum
        if (ddrss_en & (1 << ddrss_index))
        {
            will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);
        }
    }
    // Call the function under test
    int status = i3c_controller_verify_dimm_on_current_die(ddrss_en);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Test for verifying the dimm capacity on die 1
TEST_FUNCTION(test_i3c_controller_soc_die_1_verify_dimm, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint32_t ddrss_en = 0xFC0;
    uint8_t ddrss_index = 0x0;
    for (ddrss_index = 0; ddrss_index < DDRSS_DIMM_MAX; ddrss_index++)
    {
        // Convert the ddrss_en which is 1-hot encoding to ddrss_index which is a enum
        if (ddrss_en & (1 << ddrss_index))
        {
            will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);
        }
    }
    // Call the function under test
    int status = i3c_controller_verify_dimm_on_current_die(ddrss_en);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Test for verifying the dimm capacity on die 0 and die 1
TEST_FUNCTION(test_i3c_controller_soc_verify_dimm, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint32_t ddrss_en = 0xFFF;
    uint8_t ddrss_index = 0x0;
    for (ddrss_index = 0; ddrss_index < DDRSS_DIMM_MAX; ddrss_index++)
    {
        // Convert the ddrss_en which is 1-hot encoding to ddrss_index which is a enum
        if (ddrss_en & (1 << ddrss_index))
        {
            will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);
        }
    }
    // Call the function under test
    int status = i3c_controller_verify_dimm_on_current_die(ddrss_en);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Test for verifying the dimm capacity on die 0 and die 1
TEST_FUNCTION(test_i3c_controller_soc_verify_dimm_no_ddrss_enabled, setup_soc_platform_dual_die, setup_undefined_platform)
{
    uint32_t ddrss_en = 0x0;
    uint8_t ddrss_index = 0x0;
    for (ddrss_index = 0; ddrss_index < DDRSS_DIMM_MAX; ddrss_index++)
    {
        // Convert the ddrss_en which is 1-hot encoding to ddrss_index which is a enum
        if (ddrss_en & (1 << ddrss_index))
        {
            will_return(__wrap_ddr_i3c_interface_read_dimm_capacity, SILIBS_SUCCESS);
            expect_function_call(__wrap_ddr_i3c_interface_read_dimm_capacity);
        }
    }
    // Call the function under test
    int status = i3c_controller_verify_dimm_on_current_die(ddrss_en);
    assert_int_equal(status, SILIBS_E_NOMEM);
}
}
