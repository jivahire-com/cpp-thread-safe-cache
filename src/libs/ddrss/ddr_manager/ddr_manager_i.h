//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_i.h
 * DDR Manager private API
 */

#pragma once

/*----------- Nested includes ------------*/
#include <ddrss_config.h>
#include <ddrss_runtime_api.h>
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
#define NUM_DIMM_PER_DIE  (6) // Each die will address 6 DIMMs
#define NUM_DIMM_TEMP_SENSORS (2) // Each DIMM will have 2 temperature sensors

// SVP Reserved regions
// Bug #2538992 - Limit 0x0000020290000000 - 0x0000026000000000 by setting Uefi Concealed attribute`
#define SVP_DDRSS_RESERVED_REGION_START (0x0000020290000000)
#define SVP_DDRSS_RESERVED_REGION_END   (0x0000026000000000)
#define SVP_DDRSS_RESERVED_REGION_ATTRIBUTES (DRAM_ACCESS_ANY | UEFI_CONCEALED_REGION)

#define MODULE_NAME "[DDR] "
#define NEWLINE     "\n"

#define DDR_LOG_INFO(fmt, ...) DbgPrint(FPFW_DEBUG_PRINT_LEVEL_INFO,    "%-*s " fmt, FPFW_DBGPRINT_LEVEL_LEN, "[INFO]", ##__VA_ARGS__)
#define DDR_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define DDR_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

// #define DEBUG_PRINT_ENABLED (1) // Enable debug print statements
#ifdef DEBUG_PRINT_ENABLED
#define DDR_LOG_DEBUG(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#else
#define DDR_LOG_DEBUG(fmt, ...) (void)0
#endif

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
bool dimm_is_present(uint32_t dimm_local_idx);
ddrss_memory_region_t* ddr_manager_get_outgoing_memory_map();

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
 * @brief Checks the DIMM temperature thresholds.
 *
 * This function evaluates the current temperature data for each DIMM and 
 * initiates appropriate corrective actions if any DIMM exceeds its thermal limits.
 */
void check_dimm_temp_thresholds();

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

/**
 * @brief Reports the telemetry data for DIMMs.
 *
 * This function reports the telemetry data for the DIMMs to the telemetry service.
 */
void ddr_telemetry_report(void);

void ddr_create_memory_map(void);
void ddr_process_i3c_data(void);
void ddr_create_bdat(void);
void ddr_create_smbios_tables(void);

uint32_t get_single_type17_table_and_strings_size(uint8_t* spd_buff);
size_t get_deviceLocator_string(uint32_t dimm_global_idx, char* buffer, size_t bufferSize);
size_t get_dimm_bank_locator_string(uint32_t dimm_global_idx, char* buffer, size_t bufferSize);
size_t get_dimm_manufacturer_string(const uint8_t* spd_buff, char* buffer, size_t bufferSize);
size_t get_dimm_serial_number_string(const uint8_t *spd_buff, char *buffer, size_t bufferSize);
size_t get_dimm_asset_tag_string(char* buffer, size_t bufferSize);
size_t get_dimm_part_number_string(const uint8_t* spd_buff, char* buffer, size_t bufferSize);
size_t get_dimm_phy_fw_version_string(char* buffer, size_t bufferSize);

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
 * Check if platform supports PHY binary loading
 *
 * @return true if platform supports PHY binary loading, false otherwise
 */
bool platform_supports_phy_bin_loading();

/**
 * @brief  Publish the DDR translation configuration table to shared memory locatioin
 *
 * \b Description:
 * This function publishes the address translation configuration for DDRSS that OS will use.
 */
void ddr_publish_prm_addr_trans_cfg();