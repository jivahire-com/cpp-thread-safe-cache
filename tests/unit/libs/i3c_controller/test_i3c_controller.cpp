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
#include <nvic.h>
#include <stdint.h> // for int32_t, uint32_t

/*-- Symbolic Constant Macros (defines) --*/
#define SCP_I3C0_CSR_ADDRESS SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_I3C_0_ADDRESS
#define SCP_I3C1_CSR_ADDRESS SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_I3C_1_ADDRESS

#define RETURN_I3C_FAIL    (-1)
#define RETURN_I3C_SUCCESS (SILIBS_SUCCESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
int return_i3c_initialize = RETURN_I3C_SUCCESS;
int return_i3c_master_dat_config = RETURN_I3C_SUCCESS;
int return_i3c_master_set_aasa = RETURN_I3C_SUCCESS;
static const int UNUSED_NON_ZERO_MAGIC_NUMBER = 10;

/*------------- Functions ----------------*/
//
// Setup and Teardown Functions
//
static int setup_svp_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    return 0;
}

static int setup_fpga_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    return 0;
}

static int setup_fpga_rvp_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE_RVP);
    return 0;
}

static int setup_undefined_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_UNDEFINED);
    return_i3c_initialize = RETURN_I3C_SUCCESS;
    return_i3c_master_dat_config = RETURN_I3C_SUCCESS;
    return_i3c_master_set_aasa = RETURN_I3C_SUCCESS;
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
    check_expected_ptr(parameter);
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

// Tests
// Test when i3c_initialize fails for Instance 0
TEST_FUNCTION(test_i3c_controller_svp_die_0_i3c0_initialize_fail, setup_svp_platform, setup_undefined_platform)
{
    uint8_t die_num = 0;
    i3c_instance_t* instance_0 = get_i3c0();
    uint8_t index_i3c0 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_FMP_KHZ,
    };

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
    uint8_t die_num = 0;
    i3c_instance_t* instance_0 = get_i3c0();
    uint8_t index_i3c0 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_FMP_KHZ,
    };

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
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, UNUSED_NON_ZERO_MAGIC_NUMBER);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    return_i3c_master_dat_config = RETURN_I3C_FAIL;
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_SLAVE_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, return_i3c_master_dat_config);
}

// Test when i3c_master_set_aasa fails for Instance 0
TEST_FUNCTION(test_i3c_controller_svp_die_0_i3c0_master_set_aasa_fail, setup_svp_platform, setup_undefined_platform)
{
    uint8_t die_num = 0;
    i3c_instance_t* instance_0 = get_i3c0();
    uint8_t index_i3c0 = 0x0;
    index_i3c0 = KNG_SOC_DIE_0_I3C0;
    i3c_config_t i3c_config0 = {
        .register_base_addr = SCP_I3C0_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_0,
        .index = index_i3c0,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_FMP_KHZ,
    };

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
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, UNUSED_NON_ZERO_MAGIC_NUMBER);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_SLAVE_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

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

// Die-0 SVP Boot
TEST_FUNCTION(test_i3c_controller_svp_die_0, setup_svp_platform, setup_undefined_platform)
{
    uint8_t die_num = 0;
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
        .i3c_speed_in_khz = I3C_SPEED_FMP_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_FMP_KHZ,
    };
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
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, UNUSED_NON_ZERO_MAGIC_NUMBER);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_SLAVE_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

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
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, UNUSED_NON_ZERO_MAGIC_NUMBER);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_SLAVE_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-1 SVP Boot
TEST_FUNCTION(test_i3c_controller_svp_die_1, setup_svp_platform, setup_undefined_platform)
{
    uint8_t die_num = 1;
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
        .i3c_speed_in_khz = I3C_SPEED_FMP_KHZ,
    };
    i3c_config_t i3c_config1 = {
        .register_base_addr = SCP_I3C1_CSR_ADDRESS,
        .instance_type = I3C_MASTER,
        .address = MASTER_DYNAMIC_ADDRESS_1,
        .index = index_i3c1,
        .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
        .i3c_speed_in_khz = I3C_SPEED_FMP_KHZ,
    };
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
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, UNUSED_NON_ZERO_MAGIC_NUMBER);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_0_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_0_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_0);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_SLAVE_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_0);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

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
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, UNUSED_NON_ZERO_MAGIC_NUMBER);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_I3C_CTRL_1_INT);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_I3C_CTRL_1_INT);

    // i3c_master_dat_config
    expect_value(__wrap_i3c_master_dat_config, instance, instance_1);
    expect_value(__wrap_i3c_master_dat_config, config_table_length, NUM_OF_SLAVE_DEVICES);
    expect_function_call(__wrap_i3c_master_dat_config);

    // i3c_master_set_aasa
    expect_value(__wrap_i3c_master_set_aasa, instance, instance_1);
    expect_value(__wrap_i3c_master_set_aasa, i3c_speed, I3C_SPEED_I2C_FM);
    expect_any(__wrap_i3c_master_set_aasa, callback);
    expect_any(__wrap_i3c_master_set_aasa, context);
    expect_function_call(__wrap_i3c_master_set_aasa);

    // i3c_master_register_ibi_handler
    expect_value(__wrap_i3c_master_register_ibi_handler, instance, instance_1);
    expect_any(__wrap_i3c_master_register_ibi_handler, callback);
    expect_any(__wrap_i3c_master_register_ibi_handler, context);
    expect_function_call(__wrap_i3c_master_register_ibi_handler);

    // Call the function under test
    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-0 FPGA Boot
TEST_FUNCTION(test_i3c_controller_fpga_die_0, setup_fpga_platform, setup_undefined_platform)
{
    uint8_t die_num = 0;

    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-0 FPGA RVP Boot
TEST_FUNCTION(test_i3c_controller_fpga_rvp_die_0, setup_fpga_rvp_platform, setup_undefined_platform)
{
    uint8_t die_num = 0;

    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-1 FPGA Boot
TEST_FUNCTION(test_i3c_controller_fpga_die_1, setup_fpga_platform, setup_undefined_platform)
{
    uint8_t die_num = 1;

    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}

// Die-1 FPGA RVP Boot
TEST_FUNCTION(test_i3c_controller_fpga_rvp_die_1, setup_fpga_rvp_platform, setup_undefined_platform)
{
    uint8_t die_num = 1;

    int status = i3c_controller(die_num);
    assert_int_equal(status, SILIBS_SUCCESS);
}
}