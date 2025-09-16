//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// TODO: Rename the Header file to be ddr_manager_temp_conversion.h
/**
 * @file ddr_manager_i3c.h
 * DDR Manager I3C APIs
 */

#pragma once

/*----------- Nested includes ------------*/
#include <dw_i3c.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef enum {
    DDR_MANAGER_I3C_SUCCESS = 0,
    DDR_MANAGER_I3C_TRANSACTION_ERROR = -1,
} ddr_manager_i3c_status_t;

typedef struct {
    union {
        struct {
            uint8_t temp_frac; // 0.01 degree C
            uint8_t temp_int;  // degree C
        };
        uint16_t as_uint16;
    };
    bool is_positive;  // true if positive, false if negative
} ddr_manager_i3c_temperature_t;

// Create a struct for reading each DDR I3C device type
typedef struct {
    i3c_instance_t* instance; // I3C instance
    i3c_cmd_t cmd;       // I3C command
    uint8_t ts0_dev_id;
    uint8_t ts1_dev_id;
    uint8_t pmic_dev_id;
    uint8_t ts_mr_reg_low;
    uint8_t ts_mr_reg_high;
    uint8_t pmic_mr_reg;
    uint8_t ts_data[2];
} ddr_i3c_descriptor_t;

typedef struct {
    bool is_initialized;
    ddr_i3c_descriptor_t dimm[6]; //todo - Why doesn't the NUM_DIMM_PER_DIE macro work here?
} ddr_i3c_details_t;

typedef enum
{
    TS_MR0 = 0,    // Device Type MSB
    TS_MR1,        // Device Type LSB
    TS_MR2,        // Device Revision
    TS_MR3,        // Vendor ID, Byte 0
    TS_MR4,        // Vendor ID, Byte 1
    TS_MR7  = 7,   // Device Configuration - HID
    TS_MR18 = 18,  // Device Configuration
    TS_MR19,       // Clear Register MR51 Temp Status Cmd
    TS_MR20,       // Clear Register MR52 Error Status Cmd
    TS_MR26 = 26,  // TS Config
    TS_MR27,       // Interrupt Config
    TS_MR28,       // TS Temp High Limit Config - Low Byte
    TS_MR29,       // TS Temp High Limit Config - High Byte
    TS_MR30,       // TS Temp Low Limit Config - Low Byte
    TS_MR31,       // TS Temp Low Limit Config - High Byte
    TS_MR32,       // TS Critical Temp High Limit Config - Low Byte
    TS_MR33,       // TS Critical Temp High Limit Config - High Byte
    TS_MR34,       // TS Critical Temp Low Limit Config - Low Byte
    TS_MR35,       // TS Critical Temp Low Limit Config - High Byte
    TS_MR48 = 48,  // Device Status
    TS_MR49,       // TS Current Sensed Temp - Low Byte
    TS_MR50,       // TS Current Sensed Temp - High Byte
    TS_MR51,       // TS Temp Status
    TS_MR52,       // Misc Error Status
    TS_MR255 = 255
} ts_mr_cmds_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  API to read one of a DDR5 DIMM's two temperature sensors
 *
 *  @param
 *      dimm_idx    - DIMM index (0-11)
 *      channel_idx - Channel 0 or 1
 *      ts_scaled_celsius - Pointer to scaled TSx data packed as upper byte(integer degree C) lower byte(fractional degree C, in 100ths of a degree)
 *
 *  @return
 *      status - Return status
 */
// TODO: Move below API to a Private API section
int ddr_manager_temperature_sensor_read(int dimm_idx, int channel_idx, ddr_manager_i3c_temperature_t* ts_scaled_celsius);
int ddr_manager_power_mw_read(int dimm_idx, uint16_t* power_mW);
/**
 *  API to initialize the DDR I3C interface
 */
void ddr_manager_i3c_init();

ddr_manager_i3c_temperature_t ts_convert_temperature(uint8_t ts_low, uint8_t ts_high);
