//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_i3c.c
 * Implements DDR Manager I3C APIs
 *
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i3c.h"

#include <bug_check.h>
#include <ddr_i3c.h>
#include <ddr_manager_events.h>
#include <ddr_manager_i.h>
#include <i3c_controller.h>
#include <idhw.h>

/*-- Symbolic Constant Macros (defines) --*/
// 5 I3C Devices Per DIMM
// See here: https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.MSCP?path=/src/libs/i3c_controller/inc/i3c_controller.h&version=GBmain&line=39&lineEnd=40&lineStartColumn=1&lineEndColumn=1&lineStyle=plain&_a=contents
#define I3C_DEVICE_STRIDE 5

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static uint8_t get_pmic_reg(int dimm_idx);

/*-- Declarations (Statics and globals) --*/
ddr_i3c_details_t ddr_i3c_sensors = {0};

/*------------- Functions ----------------*/
ddr_manager_i3c_temperature_t ts_convert_temperature(uint8_t ts_low, uint8_t ts_high)
{
    uint8_t ts_integer = 0;
    uint8_t ts_fraction = 0;
    bool is_positive = true;

    if (ts_low & BIT2)
    {
        ts_fraction += 25;
    }
    if (ts_low & BIT3)
    {
        ts_fraction += 50;
    }
    if (ts_low & BIT4)
    {
        ts_integer += 1;
    }
    if (ts_low & BIT5)
    {
        ts_integer += 2;
    }
    if (ts_low & BIT6)
    {
        ts_integer += 4;
    }
    if (ts_low & BIT7)
    {
        ts_integer += 8;
    }
    if (ts_high & BIT0)
    {
        ts_integer += 16;
    }
    if (ts_high & BIT1)
    {
        ts_integer += 32;
    }
    if (ts_high & BIT2)
    {
        ts_integer += 64;
    }
    if (ts_high & BIT3)
    {
        ts_integer += 128;
    }
    if (ts_high & BIT4)
    {
        is_positive = false;
    }
    else
    {
        is_positive = true;
    }

    return (ddr_manager_i3c_temperature_t){{{ts_fraction, ts_integer}}, is_positive};
}

static int get_ts_idx(uint8_t i3c_dev_idx, int channel_idx)
{
    return (I3C_DEVICE_STRIDE * (i3c_dev_idx) + (channel_idx == 0 ? DDR0_TS0 : DDR0_TS1));
}

static uint8_t i3c_get_dev_id(uint32_t ddrss_index)
{
    // DDRSS Index || Device ID
    //     0-1             0
    //     2-3             1
    //     4-5             2
    if (ddrss_index <= DDR1_DIMM)
    {
        return DDR_I3C_DEV_ID_0;
    }
    if (ddrss_index <= DDR3_DIMM)
    {
        return DDR_I3C_DEV_ID_1;
    }
    if (ddrss_index <= DDR5_DIMM)
    {
        return DDR_I3C_DEV_ID_2;
    }
    return DDR_I3C_DEV_MAX;
}

void ddr_manager_i3c_init()
{
    i3c_instance_t* i3c_instance_0;
    i3c_instance_t* i3c_instance_1;
    i3c_instance_t* i3c_instance;

    // Initialize I3C Interface
    ddr_i3c_interface_set_instance(get_i3c0(), get_i3c1());
    ddr_i3c_interface_get_instance(&i3c_instance_0, &i3c_instance_1);

    KNG_DIE_ID die_id = idhw_get_die_id();

    i3c_cmd_t i3c_cmd[NUM_DIMM_PER_DIE] = {0};
    uint8_t i3c_dev_idx;

    BUG_ASSERT(i3c_instance_0 != NULL && i3c_instance_1 != NULL);

    for (int dimm = 0; dimm < NUM_DIMM_PER_DIE; dimm++)
    {
        if (die_id == DIE_0)
        {
            i3c_instance = (dimm % 2 ? i3c_instance_0 : i3c_instance_1);
        }
        else
        {
            i3c_instance = (dimm % 2 ? i3c_instance_1 : i3c_instance_0);
        }

        // Convert from DDRSS_EN to I3C Slave Device Enum
        i3c_dev_idx = i3c_get_dev_id(dimm);
        ddr_i3c_sensors.dimm[dimm].ts0_dev_id = get_ts_idx(i3c_dev_idx, 0);
        ddr_i3c_sensors.dimm[dimm].ts1_dev_id = get_ts_idx(i3c_dev_idx, 1);
        ddr_i3c_sensors.dimm[dimm].pmic_dev_id = get_pmic_reg(dimm);
        ddr_i3c_sensors.dimm[dimm].instance = i3c_instance;
        ddr_i3c_sensors.dimm[dimm].cmd = &i3c_cmd[dimm];
        ddr_i3c_sensors.dimm[dimm].ts_mr_reg_low = TS_MR49;
        ddr_i3c_sensors.dimm[dimm].ts_mr_reg_high = TS_MR50;
        ddr_i3c_sensors.dimm[dimm].pmic_mr_reg = PMIC_REGISTER_0C;
    }

    ddr_i3c_sensors.is_initialized = true;
}

static uint8_t get_pmic_reg(int dimm_idx)
{
    switch (dimm_idx)
    {
    case DDR0_DIMM:
        return DDR0_PMIC;
    case DDR1_DIMM:
        return DDR1_PMIC;
    case DDR2_DIMM:
        return DDR2_PMIC;
    case DDR3_DIMM:
        return DDR3_PMIC;
    case DDR4_DIMM:
        return DDR4_PMIC;
    case DDR5_DIMM:
        return DDR5_PMIC;
    case DDR6_DIMM:
        return DDR6_PMIC;
    case DDR7_DIMM:
        return DDR7_PMIC;
    case DDR8_DIMM:
        return DDR8_PMIC;
    case DDR9_DIMM:
        return DDR9_PMIC;
    case DDR10_DIMM:
        return DDR10_PMIC;
    case DDR11_DIMM:
        return DDR11_PMIC;
    default:
        return 0xFF;
    }
}

#include <stdio.h>
#define PRINT_VARS(var1, var2) printf("%s: %d != %s: %d\n", #var1, var1, #var2, var2)

int ddr_manager_temperature_sensor_read(int dimm_idx, int channel_idx, ddr_manager_i3c_temperature_t* ts_scaled_celsius)
{
    uint8_t ts_low_data = 0;
    uint8_t ts_high_data = 0;
    uint8_t ts_per_channel_idx = 0;
    uint8_t data_len = 1; // Size of MR Read in Temp Sensor

    BUG_ASSERT_PARAM(dimm_idx >= 0 && dimm_idx <= 11, dimm_idx, 0);
    BUG_ASSERT_PARAM(channel_idx == 0 || channel_idx == 1, channel_idx, 0);

    if (ddr_i3c_sensors.is_initialized == false)
    {
        ddr_manager_i3c_init();
    }

    ts_per_channel_idx =
        (channel_idx == 0 ? ddr_i3c_sensors.dimm[dimm_idx].ts0_dev_id : ddr_i3c_sensors.dimm[dimm_idx].ts1_dev_id);

    // Read TS[0/1] low
    int status = ddr_i3c_interface_read_temp_sensor_mr_reg(ddr_i3c_sensors.dimm[dimm_idx].instance,
                                                           ddr_i3c_sensors.dimm[dimm_idx].cmd,
                                                           ts_per_channel_idx,
                                                           ddr_i3c_sensors.dimm[dimm_idx].ts_mr_reg_low,
                                                           &ts_low_data,
                                                           &data_len);

    if (status != DDR_I3C_INTERFACE_SUCCESS)
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_TEMPERATURE_SENSOR_MR49_READ, status);
        return DDR_MANAGER_I3C_TRANSACTION_ERROR;
    }

    // Read TS[0/1] high
    status = ddr_i3c_interface_read_temp_sensor_mr_reg(ddr_i3c_sensors.dimm[dimm_idx].instance,
                                                       ddr_i3c_sensors.dimm[dimm_idx].cmd,
                                                       ts_per_channel_idx,
                                                       ddr_i3c_sensors.dimm[dimm_idx].ts_mr_reg_high,
                                                       &ts_high_data,
                                                       &data_len);

    if (status != DDR_I3C_INTERFACE_SUCCESS)
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_TEMPERATURE_SENSOR_MR50_READ, status);
        return DDR_MANAGER_I3C_TRANSACTION_ERROR;
    }

    *ts_scaled_celsius = ts_convert_temperature(ts_low_data, ts_high_data);
    return DDR_MANAGER_I3C_SUCCESS;
}

int ddr_manager_power_mw_read(int dimm_idx, uint16_t* power_mW)
{
    uint8_t data_len = 1;
    uint8_t power_byte = 0;

    BUG_ASSERT_PARAM(dimm_idx >= 0 && dimm_idx <= 11, dimm_idx, 0);

    if (ddr_i3c_sensors.is_initialized == false)
    {
        ddr_manager_i3c_init();
    }

    int32_t status = ddr_i3c_interface_read_pmic_power(ddr_i3c_sensors.dimm[dimm_idx].instance,
                                                       ddr_i3c_sensors.dimm[dimm_idx].cmd,
                                                       ddr_i3c_sensors.dimm[dimm_idx].pmic_dev_id,
                                                       ddr_i3c_sensors.dimm[dimm_idx].pmic_mr_reg,
                                                       &power_byte,
                                                       &data_len);

    if (status != SILIBS_SUCCESS)
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_PMIC_POWER_READ, status);
        return DDR_MANAGER_I3C_TRANSACTION_ERROR;
    }

    const int POWER_SCALING_FACTOR = 125;
    *power_mW = (uint16_t)(power_byte * POWER_SCALING_FACTOR);

    return DDR_MANAGER_I3C_SUCCESS;
}