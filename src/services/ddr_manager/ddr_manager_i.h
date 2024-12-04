//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_i.h
 * DDR Manager private API
 */

#pragma once

/*----------- Nested includes ------------*/
#include <kingsgate_hsp_boot_metadata.h>
#include <kingsgate_hsp_mailbox_commands.h>
#include <fpfw_icc_base.h>
#include <stdint.h>
#include <tx_api.h>
#include "ddr_manager_i3c.h"

/*-- Symbolic Constant Macros (defines) --*/
#define DDR_WORK_QUEUE_NAME ("ddr-work-queue")
#define DDR_WORK_THREAD_NAME ("ddr-work_thread")
#define DDR_TIMER_NAME ("ddr-timer")
#define NUM_DIMM  (6) // Each die will address 6 DIMMs
#define NUM_DIMM_TEMP_SENSORS (2) // Each DIMM will have 2 temperature sensors

#define MODULE_NAME "[DDR] "
#define NEWLINE     "\n"
#define DDR_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define DDR_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define DDR_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

/*-------------- Typedefs ----------------*/
typedef union _kng_hsp_cmd_load_fw_mailbox_msg
{
    struct kng_hsp_mailbox_cmd_load_fw_req load_fw_req; /**< incoming mailbox message from protocol to handler. */
    struct kng_hsp_mailbox_msg_rsp rsp; /**< outgoing mailbox message from handler to protocol. */
    uint32_t as_uint32[4];
} kng_hsp_cmd_load_fw_mailbox_msg;
/*--------- Function Prototypes ----------*/
void ddr_worker_thread_func(ULONG pddr_service_ctx);
void ddr_timer_cb(ULONG pddr_service_ctx);

/**
 * @brief Initializes DDR telemetry.
 * 
 * This function initializes the DDR telemetry module.
 */
void ddr_init_telemetry(void);

/**
 * @brief Polls the DIMMs for telemetry data.
 * 
 * This function polls the DIMMs to retrieve telemetry data.
 */
void ddr_poll_dimms(void);

/**
 * @brief Updates the telemetry snapshot for DIMM temps
 * 
 * @param dimm_idx The index of the DIMM.
 * @param ts_idx The index of the temperature sensor.
 * @param temp The temperature value to update.
 */
void ddr_telemetry_update_dimm_temp(uint8_t dimm_idx, uint8_t ts_idx, ddr_manager_i3c_temperature_t temp);

/**
 * @brief Retrieves the temperature of a DIMM from the telemetry snapshot.
 * 
 * @param dimm_idx The index of the DIMM.
 * @param ts_idx The index of the temperature sensor.
 * @return The temperature of the DIMM.
 */
ddr_manager_i3c_temperature_t ddr_telemetry_get_dimm_temp(uint8_t dimm_idx, uint8_t ts_idx);

void ddr_create_memory_map(void);
void ddr_process_i3c_data(void);
void ddr_create_bdat(void);
void ddr_create_smbios_tables(void);
void ddr_create_smbios_type_16(void);
void ddr_create_smbios_type_17(void);
void copy_type_16_to_ddr(void);
void copy_type_17_to_ddr(void);

uint16_t config_manager_get_ddr_dimm_temp_threshold_low();
uint16_t config_manager_get_ddr_dimm_temp_threshold_high();
uint16_t config_manager_get_ddr_dimm_temp_threshold_critical();

/**
 * Sets bit 2 in MSCP EXP THERMAL_IO Register to assert THERMAL_TRIP GPIO
 */
void ddr_manager_set_thermal_trip_gpio();

/**
 * Sets bit 1 in MSCP EXP THERMAL_IO Register to assert MEMORY_HOT GPIO
 */
void ddr_manager_set_memhot_gpio();

/**
 * Clears bit 1 in MSCP EXP THERMAL_IO Register to de-assert MEMORY_HOT GPIO
 */
void ddr_manager_clear_memhot_gpio();

/**
 * Checks if polling is supported on the platform for the DDR manager.
 *
 * @return true if polling is supported, false otherwise.
 */
bool ddr_manager_platform_is_polling_supported();