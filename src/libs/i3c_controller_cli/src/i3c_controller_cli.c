//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file i3c_controller_cli.c
 * This file contains the implementation of the i3c_controller CLI interface
 * and related functionality.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h> // for DfwkAsyncRequestInitialize, PDFWK_INTER...
#include <FpFwCli.h>
#include <FpFwLinkedList.h>
#include <FpFwUtils.h>
#include <MboxPrimitives.h> // for FPFW_MBX_PAYLOAD, FpFwMailbox...
#include <ddr_i3c.h>
#include <ddr_manager_i3c.h>
#include <i3c_controller.h>
#include <i3c_controller_cli.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
#define TX_ARR_LENGTH 16
#define RX_ARR_LENGTH 16
static uint8_t tx_reg_data[TX_ARR_LENGTH] = {0x6E, 0x6C, 0x00, 0x08};
static uint8_t rx_reg_data[RX_ARR_LENGTH] = {0};

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS i3c_controller_echo_cli(int argc, const char** argv);
static FPFW_CLI_STATUS i3c_rstdaa(int argc, const char** argv);
static FPFW_CLI_STATUS i3c_setaasa(int argc, const char** argv);
static FPFW_CLI_STATUS i3c_read_temp_sensor(int argc, const char** argv);
static FPFW_CLI_STATUS i3c_read_pmic_power(int argc, const char** argv);
static FPFW_CLI_STATUS i3c_writeread(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND i3c_controller_cli_list[] = {
    {NULL_LIST_ENTRY, "i3c", "i3c_echo", i3c_controller_echo_cli, "i3c echo", "i3c_echo <addr(Hex)> <data(Hex)>"},
    {NULL_LIST_ENTRY, "i3c", "i3c_rstdaa", i3c_rstdaa, "i3c RSTDAA ", "i3c_rstdaa <I3C_Bus 0/1>"},
    {NULL_LIST_ENTRY, "i3c", "i3c_setaasa", i3c_setaasa, "i3c SETAASA", "i3c_setaasa <I3C_Bus 0/1>"},
    {NULL_LIST_ENTRY, "i3c", "i3c_rd_ts", i3c_read_temp_sensor, "i3c Read TS", "i3c_rd_ts <I3C_Bus 0/1> <dev_id 0x3/0x4, 0x8/0x9, 0xD/0xE>"},
    {NULL_LIST_ENTRY, "i3c", "i3c_rd_pwr", i3c_read_pmic_power, "i3c Read PMIC", "i3c_rd_pwr <I3C_Bus 0/1> <dev_id 0x3/0x4, 0x8/0x9, 0xD/0xE>"},
    {NULL_LIST_ENTRY, "i3c", "i3c_writeread", i3c_writeread, "i3c WriteRead", "i3c_writeread <bus_idx> <device_idx> <rx_byte_count> <reg_addr> <w_byte0> - <w_byte n>"},
};

/*------------- Functions ----------------*/
static PLACED_CODE FPFW_CLI_STATUS i3c_rstdaa(int argc, const char** argv)
{
    if (argc == 2)
    {
        char* endptr;
        uint8_t i3c_bus = strtoul(argv[1], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("1st arg %s is Invalid Hex value\n", argv[1]);
            return CLI_ERROR;
        }
        if (i3c_bus != 0 && i3c_bus != 1)
        {
            FpFwCliPrint("Invalid I3C Bus number\n");
            FpFwCliPrint("Cmds: 1, <I3C_Bus 0/1>\n");
            return CLI_ERROR;
        }
        FpFwCliPrint("i3c_rstdaa\n");
        FpFwCliPrint("i3c_bus: 0x%x\n", i3c_bus);
        i3c_instance_t* instance = (i3c_bus == 0) ? get_i3c0() : get_i3c1();
        i3c_master_reset_daa(instance, i3c_master_resetdaa_callback, NULL);
    }
    else
    {
        FpFwCliPrint("i3c_rstdaa CLI Help\n");
        FpFwCliPrint("Cmds: 1, <I3C_Bus 0/1>\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS i3c_setaasa(int argc, const char** argv)
{
    if (argc == 2)
    {
        char* endptr;
        uint8_t i3c_bus = strtoul(argv[1], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("Invalid Hex value\n");
            return CLI_ERROR;
        }
        if (i3c_bus != 0 && i3c_bus != 1)
        {
            FpFwCliPrint("Invalid I3C Bus number\n");
            return CLI_ERROR;
        }
        FpFwCliPrint("i3c_setaasa\n");
        FpFwCliPrint("i3c_bus: 0x%x\n", i3c_bus);
        i3c_instance_t* instance = (i3c_bus == 0) ? get_i3c0() : get_i3c1();
        // Set static slave address using setaasa CCC
        int32_t status = i3c_master_set_aasa(instance, I3C_SPEED_I2C_FM, i3c_master_setaasa_callback, NULL);
        if (status != CLI_SUCCESS)
        {
            DEBUG_PRINT("ERR setting I3C0 slave's dynamic addr\n");
            return status;
        }
    }
    else
    {
        FpFwCliPrint("i3c_setaasa CLI Help\n");
        FpFwCliPrint("Cmds: 1, <I3C_Bus 0/1>\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS i3c_read_temp_sensor(int argc, const char** argv)
{
    if (argc == 3)
    {
        char* endptr;
        uint8_t i3c_bus = strtoul(argv[1], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("Invalid Hex value\n");
            return CLI_ERROR;
        }
        if (i3c_bus != 0 && i3c_bus != 1)
        {
            FpFwCliPrint("Invalid I3C Bus number\n");
            return CLI_ERROR;
        }
        uint8_t dev_id = strtoul(argv[2], &endptr, 16);
        if ((dev_id != DDR0_TS0) && (dev_id != DDR4_TS0) && (dev_id != DDR2_TS0) && (dev_id != DDR0_TS1) &&
            (dev_id != DDR4_TS1) && (dev_id != DDR2_TS1))
        {
            FpFwCliPrint("Invalid Device ID\n");
            return CLI_ERROR;
        }
        FpFwCliPrint("i3c_rd_ts\n");
        FpFwCliPrint("i3c_bus: 0x%x, dev_id 0x%x\n", i3c_bus, dev_id);
        i3c_instance_t* instance = (i3c_bus == 0) ? get_i3c0() : get_i3c1();
        // TSx_LOW
        uint8_t temp_mr_reg = 49;
        uint8_t temp_data_len = 0x2;
        i3c_cmd_t s_i3c_cmd_test = {0};
        int32_t status = 0;
        uint8_t temp_data[2] = {0};

        status = ddr_i3c_interface_read_temp_sensor_mr_reg(instance, &s_i3c_cmd_test, dev_id, temp_mr_reg, temp_data, &temp_data_len);
        if (status != SILIBS_SUCCESS)
        {
            FpFwCliPrint("Error in reading DDR Temp TS%d, MR49 status 0x%x\n", dev_id, status);
        }
        FpFwCliPrint("DEV_ID: %d, temp_low 0x%x, temp_high 0x%x\n", dev_id, temp_data[0], temp_data[1]);
        ddr_manager_i3c_temperature_t ts_scaled_celsius = ts_convert_temperature(temp_data[0], temp_data[1]);
        if (ts_scaled_celsius.is_positive)
        {
            FpFwCliPrint("DDR Temp +%d.%d C\n", ts_scaled_celsius.temp_int, ts_scaled_celsius.temp_frac);
        }
        else
        {
            FpFwCliPrint("DDR Temp -%d.%d C\n", ts_scaled_celsius.temp_int, ts_scaled_celsius.temp_frac);
        }
    }
    else
    {
        FpFwCliPrint("i3c_rd_ts CLI Err\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS i3c_read_pmic_power(int argc, const char** argv)
{
    if (argc == 3)
    {
        char* endptr;
        uint8_t i3c_bus = strtoul(argv[1], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("Invalid Hex value\n");
            return CLI_ERROR;
        }
        if (i3c_bus != 0 && i3c_bus != 1)
        {
            FpFwCliPrint("Invalid I3C Bus number\n");
            return CLI_ERROR;
        }
        uint8_t dev_id = strtoul(argv[2], &endptr, 16);
        if ((dev_id != DDR0_PMIC) && (dev_id != DDR4_PMIC) && (dev_id != DDR2_PMIC))
        {
            FpFwCliPrint("Invalid Device ID\n");
            return CLI_ERROR;
        }
        FpFwCliPrint("i3c_rd_pwr\n");
        FpFwCliPrint("i3c_bus: 0x%x, dev_id 0x%x\n", i3c_bus, dev_id);
        i3c_instance_t* instance = (i3c_bus == 0) ? get_i3c0() : get_i3c1();

        uint8_t readbuff8 = 0;
        uint8_t mr_reg = PMIC_REGISTER_0C;
        uint8_t data_len = 1;
        const int POWER_SCALING_FACTOR = 125;
        uint16_t power_mw = 0;
        i3c_cmd_t s_i3c_cmd_test = {0};
        int32_t status = 0;

        status = ddr_i3c_interface_read_pmic_power(instance, &s_i3c_cmd_test, DDR0_PMIC, mr_reg, &readbuff8, &data_len);
        if (status != DDR_I3C_INTERFACE_SUCCESS)
        {
            FpFwCliPrint("PMIC power Read Failed\n");
            return CLI_ERROR;
        }
        power_mw = (readbuff8 * POWER_SCALING_FACTOR);
        FpFwCliPrint("PMIC power Read: %d mW\n", power_mw);
    }
    else
    {
        FpFwCliPrint("i3c_rd_pwr CLI Err\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

/**
 *  Write reg_address followed by write bytes, then read <rx_byte_count> bytes from the target device.
 *
 *  @param argc
 *      Number of arguments provided
 *
 *  @param argv
 *      pointer array for the argument strings
 *
 *  @return
 *      FWK_SUCCESS (always)
 */
static PLACED_CODE FPFW_CLI_STATUS i3c_writeread(int argc, const char** argv)
{
    char* endptr;

    if (argc < 5)
    {
        FpFwCliPrint("Invalid Args, Check usage\n");
        return CLI_ERROR;
    }
    if ((argc - 5) > TX_ARR_LENGTH)
    {
        FpFwCliPrint("Exceeded max write bytes(16)\n");
        return CLI_ERROR;
    }
    if (strtoul(argv[3], &endptr, 16) > RX_ARR_LENGTH)
    {
        FpFwCliPrint("Exceeded max read bytes(16)\n");
        return CLI_ERROR;
    }
    uint8_t i3c_bus = strtoul(argv[1], &endptr, 16);
    if (*endptr != '\0')
    {
        FpFwCliPrint("Invalid Hex value\n");
        return CLI_ERROR;
    }
    if (i3c_bus != 0 && i3c_bus != 1)
    {
        FpFwCliPrint("Invalid I3C Bus number\n");
        return CLI_ERROR;
    }
    uint8_t dev_id = strtoul(argv[2], &endptr, 16);
    if (dev_id > DDR2_TS1)
    {
        FpFwCliPrint("Invalid Device ID 0x%x > 0x%x\n", dev_id, DDR2_TS1);
        return CLI_ERROR;
    }
    i3c_cmd_t s_i3c_cmd = {0};
    i3c_instance_t* instance = (i3c_bus == 0) ? get_i3c0() : get_i3c1();
    s_i3c_cmd.i3c_dev_idx = dev_id;
    ddr_i3c_interface_get_i3c_speed(&s_i3c_cmd.i3c_speed);

    s_i3c_cmd.rx_data = rx_reg_data;
    s_i3c_cmd.rx_data_length = strtoul(argv[3], &endptr, 16);
    s_i3c_cmd.reg_addr = strtoul(argv[4], &endptr, 16);

    uint8_t tx_byte_count = 0;
    for (uint8_t i = 5; i < argc; i++)
    {
        tx_reg_data[tx_byte_count++] = strtoul(argv[i], &endptr, 16);
    }
    s_i3c_cmd.tx_data = tx_reg_data;
    s_i3c_cmd.tx_data_length = tx_byte_count;
    s_i3c_cmd.is_last_command = false;

    int status = i3c_master_write_read_xfer(instance, &s_i3c_cmd, i3c_master_read_mr_callback, NULL);
    if (status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("i3c_master_write_read_xfer failed, status 0x%x\n", status);
        return CLI_ERROR;
    }

    SLEEP_US(DELAY_100_MS);
    for (uint8_t i = 0; i < s_i3c_cmd.rx_data_length; i++)
    {
        FpFwCliPrint("reg_addr 0x%x\n", s_i3c_cmd.reg_addr);
        FpFwCliPrint("rx_data[%d]: 0x%x\n", i, s_i3c_cmd.rx_data[i]);
    }
    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS i3c_controller_echo_cli(int argc, const char** argv)
{
    if (argc == 3)
    {
        char* endptr;
        unsigned long addr32 = strtoul(argv[1], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("1st arg %s is Invalid Hex value\n", argv[1]);
            return CLI_ERROR;
        }
        unsigned long data32 = strtoul(argv[2], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("2nd arg %s is Invalid Hex value\n", argv[2]);
            return CLI_ERROR;
        }
        FpFwCliPrint("i3c echo\n");
        FpFwCliPrint("addr32: 0x%x, data32: 0x%x\n", addr32, data32);
    }
    else
    {
        FpFwCliPrint("i3c Echo CLI Err\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

void i3c_controller_cli_initialize(void)
{
    FpFwCliRegisterTable(i3c_controller_cli_list, FPFW_ARRAY_SIZE(i3c_controller_cli_list));

    FpFwCliPrint("i3c_controller CLI init done\n");
}
