//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file i3c_controller.h
 *  Calls the I3C Controller Library for KNG SOC
 */

#pragma once

#include <DbgPrint.h>
#include <dw_i3c.h>
#include <silibs_scp_top_regs.h>
#include <silibs_scp_exp_top_regs.h>

#define MASTER_DYNAMIC_ADDRESS_0    0x30
#define MASTER_DYNAMIC_ADDRESS_1    0x31

#define I3C_CORE_CLOCK              200
#define NUM_OF_TARGET_DEVICES        15

#define NUM_I3C_CONFIGS             2
#define NUM_I3C_INSTANCES           2
#define NUM_I3C_IRQNUMS             2
#define NUM_I3C_IRQ_HANDLERS        2

typedef enum {
    KNG_SOC_DIE_0_I3C0 = 0x00,
    KNG_SOC_DIE_0_I3C1 = 0x01,
    KNG_SOC_DIE_1_I3C0 = 0x10,
    KNG_SOC_DIE_1_I3C1 = 0x11
} die_i3c_id_t;

// I3C DDR DIMM Addresses
// KNG Schematic Folder - https://microsoft.sharepoint.com/teams/C4143PlatformRFQ/Shared%20Documents/Forms/AllItems.aspx?csf=1&web=1&e=e8hXGn&cid=a8fde410%2D4810%2D4383%2Db639%2D3be78a6b0cf5&FolderCTID=0x012000B43A5631B7F59D4B9CE0281A1D773DCA&OR=Teams%2DHL&CT=1722618328889&clickparams=eyJBcHBOYW1lIjoiVGVhbXMtRGVza3RvcCIsIkFwcFZlcnNpb24iOiI0OS8yNDA3MTEyODgxNyIsIkhhc0ZlZGVyYXRlZFVzZXIiOmZhbHNlfQ%3D%3D&id=%2Fteams%2FC4143PlatformRFQ%2FShared%20Documents%2FGeneral%2FEE%2F3%5FRVP%5FBoard%2F02%5FPre%2DEV&viewid=7411ce1d%2Dae0e%2D454c%2D80c5%2D215fa6aebd44
// The Schematic folder shows the addresses of the SPD I3C Targets in the DDR DIMM and the layout connections
// The PMIC, RCD, TS0, and TS1 I3C Targets are derived from the JESD Spec
// JESD Spec - https://www.jedec.org/standards-documents/docs/jesd251
static const dat_entry_t i3c_dev_table_0_2[NUM_OF_TARGET_DEVICES] = {
    {0x50, 0x50},  //  DDR DIMM-A OR DIMM-G I3C SPD Hub
    {0x48, 0x48},  //  DDR DIMM-A OR DIMM-G I3C PMIC
    {0x58, 0x58},  //  DDR DIMM-A OR DIMM-G I3C RCD
    {0x10, 0x10},  //  DDR DIMM-A OR DIMM-G I3C TS0
    {0x30, 0x30},  //  DDR DIMM-A OR DIMM-G I3C TS1

    {0x51, 0x51},  //  DDR DIMM-B OR DIMM-H I3C SPD Hub
    {0x49, 0x49},  //  DDR DIMM-B OR DIMM-H I3C PMIC
    {0x59, 0x59},  //  DDR DIMM-B OR DIMM-H I3C RCD
    {0x11, 0x11},  //  DDR DIMM-B OR DIMM-H I3C TS0
    {0x31, 0x31},  //  DDR DIMM-B OR DIMM-H I3C TS1

    {0x52, 0x52},  //  DDR DIMM-C OR DIMM-J I3C SPD Hub
    {0x4A, 0x4A},  //  DDR DIMM-C OR DIMM-J I3C PMIC
    {0x5A, 0x5A},  //  DDR DIMM-C OR DIMM-J I3C RCD
    {0x12, 0x12},  //  DDR DIMM-C OR DIMM-J I3C TS0
    {0x32, 0x32},  //  DDR DIMM-C OR DIMM-J I3C TS1
};

static const dat_entry_t i3c_dev_table_3_5[NUM_OF_TARGET_DEVICES] = {
    {0x53, 0x53},  //  DDR DIMM-D OR DIMM-K I3C SPD Hub
    {0x4B, 0x4B},  //  DDR DIMM-D OR DIMM-K I3C PMIC
    {0x5B, 0x5B},  //  DDR DIMM-D OR DIMM-K I3C RCD
    {0x13, 0x13},  //  DDR DIMM-D OR DIMM-K I3C TS0
    {0x33, 0x33},  //  DDR DIMM-D OR DIMM-K I3C TS1

    {0x54, 0x54},  //  DDR DIMM-E OR DIMM-L I3C SPD Hub
    {0x4C, 0x4C},  //  DDR DIMM-E OR DIMM-L I3C PMIC
    {0x5C, 0x5C},  //  DDR DIMM-E OR DIMM-L I3C RCD
    {0x14, 0x14},  //  DDR DIMM-E OR DIMM-L I3C TS0
    {0x34, 0x34},  //  DDR DIMM-E OR DIMM-L I3C TS1

    {0x55, 0x55},  //  DDR DIMM-F OR DIMM-M I3C SPD Hub
    {0x4D, 0x4D},  //  DDR DIMM-F OR DIMM-M I3C PMIC
    {0x5D, 0x5D},  //  DDR DIMM-F OR DIMM-M I3C RCD
    {0x15, 0x15},  //  DDR DIMM-F OR DIMM-M I3C TS0
    {0x35, 0x35},  //  DDR DIMM-F OR DIMM-M I3C TS1
};

/*--------------- Includes ---------------*/
/**
 * @brief I3C0 ISR handler
 *
 *      This function is called when I3C0 interrupt is triggered.
 *      It calls the I3C ISR handler in the lower level library
 * @param context
 */
void i3c0_isr(void *context);

/**
 * @brief I3C1 ISR handler
 *
 *      This function is called when I3C1 interrupt is triggered.
 *      It calls the I3C ISR handler in the lower level library
 * @param context
 */
void i3c1_isr(void *context);

/**
 * @brief Get I3C0 instance
 *
 * @return i3c_instance_t*
 */
i3c_instance_t* get_i3c0();

/**
 * @brief Get I3C1 instance
 *
 * @return i3c_instance_t*
 */
i3c_instance_t* get_i3c1();

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
int i3c_controller(uint8_t die_num);

void ts_display_temperature(uint8_t ts_low, uint8_t ts_high, uint8_t* ts_integer, uint8_t* ts_fraction);

i3c_instance_t* get_i3c0();
i3c_instance_t* get_i3c1();

uint32_t get_i3c_dimm_detected(void);
uint32_t get_i3c_dimm_cap_in_gb(void);
uint8_t get_i3c_dimm_cap_per_ch(void);
uint8_t get_i3c_dimm_sku(void);
bool is_i3c_supported();
