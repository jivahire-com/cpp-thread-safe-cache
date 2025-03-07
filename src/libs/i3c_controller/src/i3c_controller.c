//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file i3c_controller.c
 *  This modules initializes I3C Controller in KNG SOC
 */

/*------------- Includes -----------------*/
#include <FPFwInterrupts.h>
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <MboxPrimitives.h> // for FPFW_MBX_PAYLOAD, FpFwMailbox...
#include <bug_check.h>
#include <ddr_i3c.h>
#include <fpfw_cfg_mgr.h>
#include <i3c_controller.h>
#include <idhw.h>     // for idhw_is_single_die_boot_en
#include <idsw.h>     // for idsw_get_platform_sdv,
#include <idsw_kng.h> // for PLATFORM_FPGA_LARGE
#include <interrupts.h>
#include <kng_soc_constants.h> // for NUM_DIE
#include <mscp_exp_spi_synchronize_dies.h>
#include <silibs_status.h> // for SILIBS_SUCCESS
#include <spi_bridge.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <stdio.h>   // for CRITICAL_PRINT
#include <system_info.h>
#include <tx_api.h>

/*------------- Defines -----------------*/
#define SCP_I3C0_CSR_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_I3C_0_ADDRESS)
#define SCP_I3C1_CSR_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_I3C_1_ADDRESS)

#define MOD_NAME   "[MOD_DW_I3C] "
#define MOD_NAME_I "[MOD_DW_I3C_%d] "

// An i3c_instance structure.
static i3c_instance_t i3c0 = {0};
static i3c_instance_t i3c1 = {0};
i3c_instance_t* i3c_instance[NUM_I3C_INSTANCES] = {&i3c0, &i3c1};
const int MEANINGLESS_NUMBER_I3C = 10;
static int unused_parameter_not_null = MEANINGLESS_NUMBER_I3C;

/*-- Declarations (Statics and globals) --*/
static uint32_t g_ddrss_en = 0x0;
static uint8_t g_dimm_cap_per_ch = 0x0;
static uint8_t g_dimm_sku = 0x0;

/*------------- Functions ----------------*/
/**
 * @brief I3C0 ISR handler
 *
 *      This function is called when I3C0 interrupt is triggered.
 *      It calls the I3C ISR handler in the lower level library
 * @param context
 */
void i3c0_isr(void* context)
{
    UNUSED(context);
    i3c_common_intr_handler(&i3c0);
}

/**
 * @brief I3C1 ISR handler
 *
 *      This function is called when I3C1 interrupt is triggered.
 *      It calls the I3C ISR handler in the lower level library
 * @param context
 */
void i3c1_isr(void* context)
{
    UNUSED(context);
    i3c_common_intr_handler(&i3c1);
}

/**
 * @brief Get I3C0 instance
 *
 * @return i3c_instance_t*
 */
i3c_instance_t* get_i3c0()
{
    return &i3c0;
}

/**
 * @brief Get I3C1 instance
 *
 * @return i3c_instance_t*
 */
i3c_instance_t* get_i3c1()
{
    return &i3c1;
}

/*
 * Called when a target device initiates an IBI. This can be a Target Interrupt Request (SIR) or a mastership request (MR).
 */
static void i3c_controller_ibi_callback(uint8_t device_address, uint8_t ibi_type, void* context)
{
    UNUSED(context);
    UNUSED(device_address);
    UNUSED(ibi_type);

    CRITICAL_PRINT(MOD_NAME "%s - Begin", __func__);
}

/*
 * Called for I3C Controller notifications from lower layer library.
 */
static void i3c_controller_notification_callback(uint8_t notification, void* context)
{
    UNUSED(context);
    UNUSED(notification);

    CRITICAL_PRINT(MOD_NAME "%s - %d", __func__, (int)notification);
}

bool is_i3c_supported()
{
    bool is_supported = false;
    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();
    switch (platform_id)
    {
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
    case PLATFORM_SVP_SIM:
    case PLATFORM_SVP_MIN_CONFIG_SIM:
    case PLATFORM_RVP_EVT_SILICON:
        is_supported = true;
        break;
    default:
        is_supported = false;
        break;
    }
    return is_supported;
}

uint32_t get_i3c_dimm_detected(void)
{
    return g_ddrss_en;
}

uint8_t get_i3c_dimm_cap_per_ch(void)
{
    return g_dimm_cap_per_ch;
}

uint8_t get_i3c_dimm_sku(void)
{
    return g_dimm_sku;
}

void i3c_controller_read_cfg_knobs_from_spi(void)
{
    // Read the Config Knobs from SPI
    lib_i3c_cfg_t temp_lib_i3c_cfg = {0};
    temp_lib_i3c_cfg.i3c_speed = config_get_i3c_speed();
    temp_lib_i3c_cfg.reg_scl_ext_termn_lcnt_timing = config_get_reg_scl_ext_termn_lcnt_timing();
    temp_lib_i3c_cfg.reg_sda_hold_switch_dly_timing = config_get_reg_sda_hold_switch_dly_timing();
    temp_lib_i3c_cfg.timing_override = config_get_timing_override();
    temp_lib_i3c_cfg.reg_scl_i3c_od_timing = config_get_reg_scl_i3c_od_timing();
    temp_lib_i3c_cfg.reg_scl_i3c_pp_timing = config_get_reg_scl_i3c_pp_timing();
    temp_lib_i3c_cfg.reg_scl_i2c_fm_timing = config_get_reg_scl_i2c_fm_timing();
    temp_lib_i3c_cfg.reg_scl_i2c_fmp_timing = config_get_reg_scl_i2c_fmp_timing();
    temp_lib_i3c_cfg.reg_scl_ext_lcnt_timing = config_get_reg_scl_ext_lcnt_timing();

    INFO_PRINT("i3c_speed 0x%x\n", temp_lib_i3c_cfg.i3c_speed);
    INFO_PRINT("reg_scl_ext_termn_lcnt_timing 0x%x\n", temp_lib_i3c_cfg.reg_scl_ext_termn_lcnt_timing);
    INFO_PRINT("reg_sda_hold_switch_dly_timing 0x%x\n", temp_lib_i3c_cfg.reg_sda_hold_switch_dly_timing);
    INFO_PRINT("timing_override 0x%x\n", temp_lib_i3c_cfg.timing_override);
    INFO_PRINT("reg_scl_i3c_od_timing 0x%x\n", temp_lib_i3c_cfg.reg_scl_i3c_od_timing);
    INFO_PRINT("reg_scl_i3c_pp_timing 0x%x\n", temp_lib_i3c_cfg.reg_scl_i3c_pp_timing);
    INFO_PRINT("reg_scl_i2c_fm_timing 0x%x\n", temp_lib_i3c_cfg.reg_scl_i2c_fm_timing);
    INFO_PRINT("reg_scl_i2c_fmp_timing 0x%x\n", temp_lib_i3c_cfg.reg_scl_i2c_fmp_timing);
    INFO_PRINT("reg_scl_ext_lcnt_timing 0x%x\n", temp_lib_i3c_cfg.reg_scl_ext_lcnt_timing);

    // Set the Config Knobs in lib_i3c
    i3c_master_set_cfg_knobs(&temp_lib_i3c_cfg);
}
/*
 * @brief I3C Controller initialization
 *
 *   This function initializes I3C Controller for the specified die number.
 *   It initializes the I3C Controller controller, registers the notification callback, registers the ISRs, configures the Device Address Table (DAT),
 *   sets all addresses to static addresses, and registers the IBI handler.
 *
 * @param die_num
 * @return int
 */
int i3c_controller(uint8_t die_num)
{
    int status = 0;

    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);
    CRITICAL_PRINT(MOD_NAME "%s Start, die_num: [%u]\n", __func__, die_num);

    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();

    if (is_i3c_supported())
    {
        i3c_controller_read_cfg_knobs_from_spi();
        i3c_speed_t i3c_speed = config_get_i3c_speed();
        CRITICAL_PRINT("i3c_speed 0x%x\n", i3c_speed);
        uint16_t user_i3c_speed_in_khz = fnc_decode_i3c_speed_to_khz(i3c_speed);
        uint8_t index_i3c0 = 0x0;
        uint8_t index_i3c1 = 0x0;
        uint8_t plat_num_of_target_devices = NUM_OF_TARGET_DEVICES;
        uint32_t intr_status = 0;
        const dat_entry_t* i3c_dev_table = NULL;

        if (die_num == 0)
        {
            index_i3c0 = KNG_SOC_DIE_0_I3C0;
            index_i3c1 = KNG_SOC_DIE_0_I3C1;
            i3c_dev_table = i3c_dev_table_3_5;
        }
        else if (die_num == 1)
        {
            index_i3c0 = KNG_SOC_DIE_1_I3C0;
            index_i3c1 = KNG_SOC_DIE_1_I3C1;
            i3c_dev_table = i3c_dev_table_0_2;
        }

        // Update only for FPGA and NOT FPGA_RVP
        if (platform_id == PLATFORM_FPGA_LARGE)
        {
            i3c_dev_table = i3c_dev_table_0_2;
        }
        FPFW_RUNTIME_ASSERT(i3c_dev_table != NULL);

        // Initialize I3C Config Struct to Pass into I3C Library
        i3c_config_t i3c_config0 = {
            .register_base_addr = SCP_I3C0_CSR_ADDRESS,
            .instance_type = I3C_MASTER,
            .address = MASTER_DYNAMIC_ADDRESS_0,
            .index = index_i3c0,
            .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
            .i3c_speed_in_khz = user_i3c_speed_in_khz,
        };
        i3c_config_t i3c_config1 = {
            .register_base_addr = SCP_I3C1_CSR_ADDRESS,
            .instance_type = I3C_MASTER,
            .address = MASTER_DYNAMIC_ADDRESS_1,
            .index = index_i3c1,
            .i3c_core_clk_freq_in_mhz = I3C_CORE_CLOCK,
            .i3c_speed_in_khz = user_i3c_speed_in_khz,
        };
        i3c_config_t* i3c_configs[NUM_I3C_CONFIGS] = {&i3c_config0, &i3c_config1};

        uint32_t i3c_irqnums[NUM_I3C_IRQNUMS] = {HW_INT_I3C_CTRL_0_INT, HW_INT_I3C_CTRL_1_INT};

        FPFwCoreInterruptHandler i3c_irq_handlers[NUM_I3C_IRQ_HANDLERS] = {(FPFwCoreInterruptHandler)i3c0_isr,
                                                                           (FPFwCoreInterruptHandler)i3c1_isr};

        // Master init
        for (uint8_t i = 0; i < NUM_I3C_CONFIGS; i++)
        {
            status = i3c_initialize(i3c_instance[i], i3c_configs[i]);
            if (status != SILIBS_SUCCESS)
            {
                CRITICAL_PRINT(MOD_NAME "I3C%d Master Init Error\n", i);
                // Error or BUGCHECK
                goto exit;
            }

            // Register Notification Callback
            i3c_register_notification_callback(i3c_instance[i], i3c_controller_notification_callback, NULL);

            // Register ISRs
            intr_status = FPFwCoreInterruptRegisterCallback(i3c_irqnums[i],
                                                            (FPFwCoreInterruptHandler)i3c_irq_handlers[i],
                                                            (void*)&unused_parameter_not_null);
            intr_status |= FPFwCoreInterruptEnableVector(i3c_irqnums[i]);
            FPFW_RUNTIME_ASSERT(intr_status == 0);

            // Configure the Device Address Table (DAT)
            status = i3c_master_dat_config(i3c_instance[i], i3c_dev_table, plat_num_of_target_devices);
            if (status != SILIBS_SUCCESS)
            {
                // Error in configuring DAT
                CRITICAL_PRINT(MOD_NAME "I3C%d Device Addr Table Init Error\n", i);
                // Error or BUGCHECK
                goto exit;
            }

            // Register IBI handler
            i3c_master_register_ibi_handler(i3c_instance[i], i3c_controller_ibi_callback, NULL);
        }
        uint8_t num_i3c_configs_platform = NUM_I3C_CONFIGS;
        // Initialize I3C Interface
        if (platform_id == PLATFORM_FPGA_LARGE)
        {
            ddr_i3c_interface_set_instance(get_i3c0(), NULL);
            num_i3c_configs_platform = 1;
        }
        else
        {
            ddr_i3c_interface_set_instance(get_i3c0(), get_i3c1());
        }

        // Send PMIC ON
        i3c_cmd_t s_i3c_cmd = {0};
        for (uint8_t i = 0; i < num_i3c_configs_platform; i++)
        {
            status = ddr_i3c_interface_power_up_pmic_on(i3c_instance[i], &s_i3c_cmd);
            if (status != SILIBS_SUCCESS)
            {
                // Error in sending PMIC ON
                CRITICAL_PRINT(MOD_NAME "Error in sending PMIC ON, I3C 0x%x\n", i);
                // Error or BUGCHECK
                goto exit;
            }
            // Set all addresses to static addresses
            status = i3c_master_set_aasa(i3c_instance[i], I3C_SPEED_I2C_FM, NULL, NULL);
            if (status != SILIBS_SUCCESS)
            {
                // Error in setting all addresses to static addresses
                CRITICAL_PRINT(MOD_NAME "Err to set static addresses, I3C 0x%x\n", i);
                // Error or BUGCHECK
                goto exit;
            }
        }
        uint8_t dimm_cap_per_ch = 0x0;
        uint8_t dimm_sku = 0x0;
        uint32_t ddrss_en = 0;
        i3c_cmd_t s_i3c_cmd_test = {0};

        SLEEP_US(DELAY_10_MS);
        // Layout of DDR-I3C Target on SVP is not supported
        if (!(IS_PLATFORM_SVP()))
        {
            status = ddr_i3c_interface_read_dimms_detected(&s_i3c_cmd_test, &ddrss_en);
            if (status != SILIBS_SUCCESS)
            {
                CRITICAL_PRINT(MOD_NAME "DDR DIMMs Read Err, status 0x%x\n", status);
                // Error or BUGCHECK
                goto exit;
            }
            SLEEP_US(DELAY_10_MS);
            CRITICAL_PRINT(MOD_NAME "DDR DIMM Detected: 0x%x\n", ddrss_en);
            status = ddr_i3c_interface_read_dimm_capacity(&s_i3c_cmd_test, ddrss_en, &dimm_cap_per_ch, &dimm_sku);
            if (status != SILIBS_SUCCESS)
            {
                CRITICAL_PRINT(MOD_NAME "DDR DIMM Capacity/SKU Read Err, status 0x%x\n", status);
                // Error or BUGCHECK
                goto exit;
            }
            SLEEP_US(DELAY_10_MS);
            CRITICAL_PRINT(MOD_NAME "DDR DIMM Capacity: 0x%x, SKU: 0x%x\n", dimm_cap_per_ch, dimm_sku);
            g_dimm_cap_per_ch = dimm_cap_per_ch;
            g_dimm_sku = dimm_sku;
        }
        else
        {
            g_dimm_cap_per_ch = DIMM_40_BIT_CH_32GB;
            g_dimm_sku = DDR5_RDIMM_2Rx4_16Gb_64GB;
            ddrss_en = (die_num == SOC_D0) ? (0x3F) : (0xFC0);
        }

        // I3C Sync point with Remote Die on a 2-Die Config
        if (!idhw_is_single_die_boot_en()) // 2 Die
        {
            CRITICAL_PRINT("I3C Sync with Remote Die\n");
            mscp_exp_spi_invalidate_region(die_num);

            if (die_num == SOC_D0)
            {
                i3c_test_sync.data_d0_to_d1_data = ddrss_en;
                i3c_test_sync.data_ack_d0_to_d1_data = SPI_SYNC_DATA_VALID;
                ASSERT_FAIL(mscp_exp_spi_write_d0_to_d1_data(&i3c_test_sync, die_num) == SILIBS_SUCCESS);
                DEBUG_PRINT("Data written to D1 0x%x\n", i3c_test_sync.data_d0_to_d1_data);
                ASSERT_FAIL(mscp_exp_spi_read_d1_to_d0_data(&i3c_test_sync, die_num) == SILIBS_SUCCESS);
                DEBUG_PRINT("Data from D1 0x%x\n", i3c_test_sync.data_d1_to_d0_data);
                g_ddrss_en = (ddrss_en | i3c_test_sync.data_d1_to_d0_data);
            }
            else
            {
                i3c_test_sync.data_d1_to_d0_data = ddrss_en;
                i3c_test_sync.data_ack_d1_to_d0_data = SPI_SYNC_DATA_VALID;
                ASSERT_FAIL(mscp_exp_spi_write_d1_to_d0_data(&i3c_test_sync, die_num) == SILIBS_SUCCESS);
                DEBUG_PRINT("Data written to D0 0x%x\n", i3c_test_sync.data_d1_to_d0_data);
                ASSERT_FAIL(mscp_exp_spi_read_d0_to_d1_data(&i3c_test_sync, die_num) == SILIBS_SUCCESS);
                DEBUG_PRINT("Data from D0 0x%x\n", i3c_test_sync.data_d0_to_d1_data);
                g_ddrss_en = (ddrss_en | i3c_test_sync.data_d0_to_d1_data);
            }
        }
        else
        {
            g_ddrss_en = ddrss_en;
        }
    }
    else
    {
        CRITICAL_PRINT(MOD_NAME "Not supported platform\n");
    }

exit:
    CRITICAL_PRINT(MOD_NAME "%s End, die_num: [%u], status 0x%x\n", __func__, die_num, status);
    return status;
}
