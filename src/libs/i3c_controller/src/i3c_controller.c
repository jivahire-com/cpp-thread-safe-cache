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
#include <i3c_controller.h>
#include <idhw.h>     // for idhw_is_single_die_boot_en
#include <idsw.h>     // for idsw_get_platform_sdv,
#include <idsw_kng.h> // for PLATFORM_FPGA_LARGE
#include <interrupts.h>
#include <kng_soc_constants.h> // for NUM_DIE
#include <silibs_status.h>     // for SILIBS_SUCCESS
#include <stdbool.h>           // for true
#include <stdint.h>            // for uint8_t
#include <stdio.h>             // for DEBUG_PRINT
#include <system_info.h>

/*------------- Defines -----------------*/
#define SCP_I3C0_CSR_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_I3C_0_ADDRESS)
#define SCP_I3C1_CSR_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_I3C_1_ADDRESS)

#define MOD_NAME   "[MOD_DW_I3C] "
#define MOD_NAME_I "[MOD_DW_I3C_%d] "

// An i3c_instance structure.
static i3c_instance_t i3c0 = {0};
static i3c_instance_t i3c1 = {0};
i3c_instance_t* i3c_instance[NUM_I3C_INSTANCES] = {&i3c0, &i3c1};

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

    DEBUG_PRINT(MOD_NAME "%s - Begin", __func__);
}

/*
 * Called for I3C Controller notifications from lower layer library.
 */
static void i3c_controller_notification_callback(uint8_t notification, void* context)
{
    UNUSED(context);

    DEBUG_PRINT(MOD_NAME "%s - %d", __func__, (int)notification);
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
    DEBUG_PRINT(MOD_NAME "%s Start, die_num: [%u]\n", __func__, die_num);

    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();
    if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP))
    {
        DEBUG_PRINT(MOD_NAME "I3C Controller init for FPGA platform not supported yet\n");
        goto exit;
    }
    else if (platform_id == PLATFORM_SVP_SIM)
    {
        uint8_t index_i3c0 = 0x0;
        uint8_t index_i3c1 = 0x0;
        uint32_t intr_status = 0;
        const dat_entry_t* i3c_dev_table = NULL;
        i3c_isr_params_t params = {.die_id = die_num};

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
        FPFW_RUNTIME_ASSERT(i3c_dev_table != NULL);

        // GPIO AFM Init

        // Initialize I3C Config Struct to Pass into I3C Library
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
        i3c_config_t i3c_configs[NUM_I3C_CONFIGS] = {i3c_config0, i3c_config1};

        uint32_t i3c_irqnums[NUM_I3C_IRQNUMS] = {HW_INT_I3C_CTRL_0_INT, HW_INT_I3C_CTRL_1_INT};

        FPFwCoreInterruptHandler i3c_irq_handlers[NUM_I3C_IRQ_HANDLERS] = {(FPFwCoreInterruptHandler)i3c0_isr,
                                                                           (FPFwCoreInterruptHandler)i3c1_isr};

        // Master init
        for (uint8_t i = 0; i < NUM_I3C_CONFIGS; i++)
        {
            status = i3c_initialize(i3c_instance[i], &i3c_configs[i]);
            if (status != SILIBS_SUCCESS)
            {
                DEBUG_PRINT(MOD_NAME "Error initializing I3C%d Master Controller\n", i);
                // Error or BUGCHECK
                goto exit;
            }

            // Register Notification Callback
            i3c_register_notification_callback(i3c_instance[i], i3c_controller_notification_callback, NULL);

            // Register ISRs
            intr_status = FPFwCoreInterruptRegisterCallback(i3c_irqnums[i],
                                                            (FPFwCoreInterruptHandler)i3c_irq_handlers[i],
                                                            (void*)&params);
            intr_status |= FPFwCoreInterruptEnableVector(i3c_irqnums[i]);
            FPFW_RUNTIME_ASSERT(intr_status == 0);

            // Configure the Device Address Table (DAT)
            status = i3c_master_dat_config(i3c_instance[i], i3c_dev_table, NUM_OF_SLAVE_DEVICES);
            if (status != SILIBS_SUCCESS)
            {
                // Error in configuring DAT
                DEBUG_PRINT(MOD_NAME "Error initializing I3C%d Device Address Table\n", i);
                // Error or BUGCHECK
                goto exit;
            }

            // Send PMIC ON

            // Set all addresses to static addresses
            status = i3c_master_set_aasa(i3c_instance[i], I3C_SPEED_I2C_FM, NULL, NULL);
            if (status != SILIBS_SUCCESS)
            {
                // Error in setting all addresses to static addresses
                DEBUG_PRINT(MOD_NAME "Error in setting all addresses to static addresses\n");
                // Error or BUGCHECK
                goto exit;
            }

            // Register IBI handler
            i3c_master_register_ibi_handler(i3c_instance[i], i3c_controller_ibi_callback, NULL);
        }
    }
    else
    {
        DEBUG_PRINT(MOD_NAME "I3C Controller init is not supported yet on the platform\n");
    }

exit:
    DEBUG_PRINT(MOD_NAME "%s End, die_num: [%u], status 0x%x\n", __func__, die_num, status);
    return status;
}