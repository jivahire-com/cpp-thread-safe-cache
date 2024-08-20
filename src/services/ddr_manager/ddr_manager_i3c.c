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
#include <i3c_controller.h>
#include <idhw.h>

/*-- Symbolic Constant Macros (defines) --*/
// 5 I3C Devices Per DIMM
// See here: https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.MSCP?path=/src/libs/i3c_controller/inc/i3c_controller.h&version=GBmain&line=39&lineEnd=40&lineStartColumn=1&lineEndColumn=1&lineStyle=plain&_a=contents
#define I3C_DEVICE_STRIDE 5

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static ddr_manager_i3c_temperature_t ts_convert_temperature(uint8_t ts_low, uint8_t ts_high)
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
    // Initialize I3C Interface
    ddr_i3c_interface_set_instance(get_i3c0(), get_i3c1());
}

int ddr_manager_temperature_sensor_read(int dimm_idx, int channel_idx, ddr_manager_i3c_temperature_t* ts_scaled_celsius)
{
    BUG_ASSERT_PARAM(dimm_idx >= 0 && dimm_idx <= 11, dimm_idx, 0);
    BUG_ASSERT_PARAM(channel_idx == 0 || channel_idx == 1, channel_idx, 0);

    i3c_instance_t* i3c_instance_0;
    i3c_instance_t* i3c_instance_1;
    ddr_i3c_interface_get_instance(&i3c_instance_0, &i3c_instance_1);
    BUG_ASSERT(i3c_instance_0 != NULL && i3c_instance_1 != NULL);

    i3c_instance_t* i3c_instance;
    KNG_DIE_ID die_num = idhw_get_die_id();
    // DIE0.I3C0: DDRSS 1/3/5
    // DIE0.I3C1: DDRSS 0/2/4
    // DIE1.I3C0: DDRSS 6/8/10
    // DIE1.I3C1: DDRSS 7/9/11
    if (die_num == DIE_0)
    {
        i3c_instance = (dimm_idx % 2 ? i3c_instance_0 : i3c_instance_1);
    }
    else
    {
        i3c_instance = (dimm_idx % 2 ? i3c_instance_1 : i3c_instance_0);
    }

    // Convert from DDRSS_EN to I3C Slave Device Enum
    uint8_t i3c_dev_idx = i3c_get_dev_id(dimm_idx);

    // Determine which register to read based on DDR_DIMM_INDEX_I3C0/1
    uint8_t ts_idx = get_ts_idx(i3c_dev_idx, channel_idx);

    // Read TSx low
    uint8_t ts_low = 0;
    i3c_cmd_t i3c_cmd;
    uint8_t data_len = 1; // Size of MR Read in Temp Sensor
    int status = ddr_i3c_interface_read_temp_sensor_mr_reg(i3c_instance, &i3c_cmd, ts_idx, TS_MR49, &ts_low, &data_len);
    if (status != DDR_I3C_INTERFACE_SUCCESS)
    {
        printf("Temperature Sensor MR49 Read Failed\n");
        return DDR_MANAGER_I3C_TRANSACTION_ERROR;
    }

    // Read TSx high
    uint8_t ts_high = 0;
    status = ddr_i3c_interface_read_temp_sensor_mr_reg(i3c_instance, &i3c_cmd, ts_idx, TS_MR50, &ts_high, &data_len);
    if (status != DDR_I3C_INTERFACE_SUCCESS)
    {
        printf("Temperature Sensor MR50 Read Failed\n");
        return DDR_MANAGER_I3C_TRANSACTION_ERROR;
    }

    *ts_scaled_celsius = ts_convert_temperature(ts_low, ts_high);

    return DDR_MANAGER_I3C_SUCCESS;
}