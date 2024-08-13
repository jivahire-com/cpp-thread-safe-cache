//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_i.h
 * Header containing internal definitions for power HW initialization
 */

#pragma once

/*----------- Nested includes ------------*/
#include <power_runconfig_i.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define CORES_PER_TILE (NUM_AP_CORES_PER_DIE / NUM_CPU_TILES)

/*-------------- Typedefs ----------------*/
/**
 * @brief Struct for passing telemetry config to power APIs
 */
typedef struct _power_telcfg_t
{
    uint32_t pstate_telem_wr_address;
    uint32_t scp_msg_telem_wr_address;
    uint32_t current_telem_start_addr;
    uint32_t temp_telem_start_addr;
    uint32_t volt_telem_start_addr;
    uint32_t vrtemp_telem_start_addr;
    uint32_t vrcurr_telem_start_addr;
    uint32_t socpvtvolt_telem_start_addr;
    uint32_t socpvttemp_telem_start_addr;

    uint32_t current_telem_buffer_size;
    uint32_t temp_telem_buffer_size;
    uint32_t volt_telem_buffer_size;

    uint8_t current_telem_entry_size;
    uint8_t temp_telem_entry_size;
    uint8_t volt_telem_entry_size;
    uint8_t vrtemp_telem_entry_size;
    uint8_t vrcurr_telem_entry_size;
    uint8_t socpvtvolt_telem_entry_size;
    uint8_t socpvttemp_telem_entry_size;
} power_telcfg_t;

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes core-specific power HW (dvfs, odcm, tile pvt, etc)
 *
 * @param[in] p_runconfig - power runtime configuration
 * @param[in] p_telemetry_config - telemetry addresses
 *
 * @return none
 *
 */
void power_init_core(const power_runconfig_t* p_runconfig, const power_telcfg_t* p_telemetry_config);

/**
 * @brief Initializes core-specific power telemetry HW (dvfs, odcm, tile pvt)
 *
 * @param[in] p_runconfig - power runtime configuration
 * @param[in] p_telemetry_config - telemetry addresses
 *
 * @return none
 *
 */
void power_init_ws_core(const power_runconfig_t* p_runconfig, const power_telcfg_t* p_telemetry_config);

/**
 * @brief Initializes soc/top-specific power HW (pvt)
 *
 * @param[in] p_runconfig - power runtime configuration
 *
 * @return none
 *
 */
void power_init_soc(const power_runconfig_t* p_runconfig);

/**
 * @brief Resets PVT after a warm reset
 *
 * \b Description:
 *      This function to reset PVT ahead of reprogramming telemetry settings on a warm reset; included here for test.
 *
 * @return none
 */
void power_warm_init_core_reset_pvt(const power_runconfig_t* p_runconfig);

/**
 * @brief Checks to see if HW supports PWR management
 *
 * @return true or false
 *
 */
bool power_hw_supported();

/**
 * @brief Checks to see if HW supports PWR management and additionally uses a model of PVT to produce sensor data
 *
 * @return true or false
 *
 */
bool power_hw_uses_pvt_model();

/**
 * @brief Checks to see if full init of HW is allowed (based on boot reason)
 *
 * @return true or false
 *
 */
bool power_hw_full_init_allowed();

/**
 * @brief Helper function to convert a raw DTS sample to a value in 1/10 degrees C
 *
 * @return none
 *
 */
uint16_t power_hw_dts_pvt_raw_to_temp_dC(uint16_t raw, dts_coeff_t fused_coeff);

/**
 * @brief Update a plimit
 *
 * \b Description:
 *      Used to post a plimits pending event to power module; used to create
 *      a polling loop that doesn't stall the rest of the firmware
 *
 */
void power_set_plimit(const power_runconfig_t* p_runconfig, unsigned int core, uint8_t plimit, bool rack_power_cap);

/**
 * @brief Checks if HW has GPIO connected meaningfully
 *
 * @return true or false
 *
 */
bool power_hw_gpio_connected();

/**
 * @brief Checks to see if rack limit gpio is asserted
 *
 * @return true or false
 *
 */
bool power_hw_gpio_rack_limit_asserted();

/**
 * @brief Finds pstate associated with given frequency
 *
 * @param[in] freq_MHz - The frequency in MHz to find associated pstate for
 *
 * @return pstate number
 *
 */
uint8_t power_hw_pstate_from_freq(uint16_t freq_MHz);

/**
 * @brief Get core adclk droop count
 *
 * \b Description:
 *      Used to get the adaptive clocking droop count for a core.
 *
 * @return droop count
 *
 */
uint32_t power_hw_get_adclk_count(const power_runconfig_t* p_runconfig, unsigned int core);

/**
 * @brief Acquires telemetry config details
 *
 */
void power_telemetry_init_config(power_telcfg_t* telemetry_config);



// TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1811925/
// remove temporary for test without system info
extern bool s_pw_supported;
extern unsigned s_core_count;

#ifdef __cplusplus
}
#endif