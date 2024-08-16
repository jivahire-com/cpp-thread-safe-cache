//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Sensor Fifo Service Interface
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdbool.h>
#include <stdint.h>

// hardware telemetry entry definitions
// NOTE:  details for these structures can be found in the RMSS HAS document
#define BEGIN_EXTERN_C
#define END_EXTERN_C
#include <telemetry_data_struct.h> // IWYU pragma: keep

/*-- Symbolic Constant Macros (defines) --*/
#define NUMBER_OF_SOC_TEMP_SENSORS  (15)
#define NUMBER_OF_SOC_VOLT_MON_SENSORS  (18)
// One die has 8 VR's and the other has 2.  Reserve space for the max number.
#define MAX_NUM_OF_VR_RAILS        (8)
#define QUADWORD_SIZE              (8)

/*-------------- Typedefs ----------------*/
/**
 * @brief FIFO identifier
 *
 */
typedef enum
{
    SENSOR_FIFO_PSTATE_TELEMETRY_HW = 0,
    SENSOR_FIFO_SCP_MSG_TELEMETRY_HW = 1,
    SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW = 2,
    SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW = 3,
    SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW = 4,
    SENSOR_FIFO_PVT_TEMP_FW = 5,
    SENSOR_FIFO_PVT_VOLTAGE_FW = 6,
    SENSOR_FIFO_DIMM_TEMP_FW = 7,
    SENSOR_FIFO_VR_TEMP_FW = 8,
    SENSOR_FIFO_VR_CURRENT_FW = 9,
    SENSOR_FIFO_MAX_ID = 10
} SENSOR_FIFO_ID;

/**
 * @brief Information describing a single FIFO
 *
 */
typedef struct {
    uint16_t entry_size_bytes;
    uint16_t stride_size_bytes;
    uint32_t start_address; ///< inclusive
    uint32_t end_address; ///< inclusive
    uint16_t epoch_count; ///< number of strides in the fifo, 1 indexed
    char*    name;
} sensor_fifo_properties_t, *psensor_fifo_properties_t;

/**
 * @brief Single entry for  SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW
 *
 */
typedef struct
{
    uint64_t timestamp;
    temp_full_data0_t temp0;
    temp_full_data1_t temp1;
    temp_full_data2_t temp2;
} tile_temp_t;

/**
 * @brief Single entry for  SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW
 *
 */
typedef struct
{
    uint64_t timestamp;
    volt_data_t data;
} tile_voltage_t;

/**
 * @brief Single entry for  SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW
 *
 */
typedef struct
{
    uint64_t timestamp;
    current_data_t data;
} core_current_t;


/**
 * @brief Single entry for  SENSOR_FIFO_PVT_TEMP_FW
 *
 */
typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    uint16_t sensor_temp[NUMBER_OF_SOC_TEMP_SENSORS];
    uint8_t  padding[2]; // needs to be multiple of QUADWORD_SIZE
} soc_pvt_temp_t;

/**
 * @brief Single entry for  SENSOR_FIFO_PVT_VOLTAGE_FW
 *
 */
typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    uint16_t sensor_voltage[NUMBER_OF_SOC_VOLT_MON_SENSORS];
    uint8_t  padding[4]; // needs to be multiple of QUADWORD_SIZE
} soc_pvt_voltage_t;

/**
 * @brief Single entry for  SENSOR_FIFO_VR_TEMP_FW
 *
 */
typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    uint16_t vr_temp[MAX_NUM_OF_VR_RAILS];
} vr_temp_t;

/**
 * @brief Single entry for SENSOR_FIFO_VR_CURRENT_FW
 *
 */
typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    uint16_t vr_current[MAX_NUM_OF_VR_RAILS];
    uint16_t vr_voltage[MAX_NUM_OF_VR_RAILS];
} vr_current_t;

/**
 * @brief Single entry for  SENSOR_FIFO_DIMM_TEMP_FW
 *
 */
typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    uint8_t dimm_id;
    uint8_t dimm_throttling;
    uint8_t dimm_memory_frequency_id;
    uint8_t dimm_info_type;
    uint16_t dimm_temp_s0;
    uint16_t dimm_temp_s1;
    uint16_t dimm_power;
    uint16_t temp_threshold_low;
    uint16_t temp_threshold_high;
    uint16_t temp_threshold_critical;
} sensor_ram_dimm_info_t;

/**
 * @brief Controls flow when reading multiple entries from a FIFO
 *
 */
typedef struct {
    bool curr_data_is_valid; ///< True if there was entry available to process, false if not
    bool more_entries;        ///< True if more data in the fifo, false if empty
} sensor_ram_poll_status_t;


typedef struct {
    uint32_t tbd;
} sensor_fifo_health_status_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Global Enable/Disable for all hardware fifos
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] enable - true - hardware fifo's enabled via sensor_fifo_svc_enable_fifo will collect data
 *                     false - all hardware fifo's are disabled
 */
void sensor_fifo_svc_set_global_hw_enable(bool enable);

/**
 * @brief Enables Data Collection
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] fifo - Select the specific fifo
 */
void sensor_fifo_svc_enable_fifo(SENSOR_FIFO_ID fifo);

/**
 * @brief Disables Data Collection
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] fifo - Select the specific fifo
 */
void sensor_fifo_svc_disable_fifo(SENSOR_FIFO_ID fifo);

/**
 * @brief Retrieve fifo properties
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] fifo - Select the specific fifo
 * @param[out] properties - Information about an individual fifo
 */
void sensor_fifo_svc_get_properties(SENSOR_FIFO_ID fifo, psensor_fifo_properties_t properties);

/**
 * @brief Retrieve operational health of the sensor fifo service for telemetry reporting. This will clear the data.
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] health_status - Current Status of the fifos
 */
void sensor_fifo_svc_poll_health(sensor_fifo_health_status_t* health_status);

/**
 * @brief - Add an entry to SENSOR_FIFO_PVT_TEMP_FW
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] pvt_temperature - pointer to the data
 */
void sensor_fifo_svc_add_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature);

/**
 * @brief - Add an entry to SENSOR_FIFO_PVT_VOLTAGE_FW
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] pvt_voltage - pointer to the data
 */
void sensor_fifo_svc_add_soc_pvt_voltage(soc_pvt_voltage_t* pvt_voltage);

/**
 * @brief - Add an entry to SENSOR_FIFO_DIMM_TEMP_FW
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] dimm_info - pointer to the data
 */
void sensor_fifo_svc_add_dimm_info(sensor_ram_dimm_info_t* dimm_info);

/**
 * @brief - Add an entry to SENSOR_FIFO_VR_TEMP_FW
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] vr_temperature - pointer to the data
 */
void sensor_fifo_svc_add_vr_temperature(vr_temp_t* vr_temperature);

/**
 * @brief - Add an entry to SENSOR_FIFO_VR_CURRENT_FW
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] vr_current - pointer to the data
 */
void sensor_fifo_svc_add_vr_current(vr_current_t* vr_current);

/**
 * @brief Poll SENSOR_FIFO_TEMPERATURE_TELEMETRY_HW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] temperature_data - destination pointer for entry read from fifo
 * @param[out] tile_index - destination pointer for entry's tile index. 0 indexed
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_tile_temperature(tile_temp_t* temperature_data, uint16_t* tile_index);

/**
 * @brief Poll SENSOR_FIFO_VOLTAGE_TELEMETRY_HW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] voltage_data - destination pointer for entry read from fifo
 * @param[out] tile_index - destination pointer for entry's tile index. 0 indexed
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_tile_voltage(tile_voltage_t* voltage_data, uint16_t* tile_index);

/**
 * @brief Poll SENSOR_FIFO_CURRENT_TELEMETRY_HW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] current_data - destination pointer for entry read from fifo
 * @param[out] core_index - destination pointer for entry's core index. 0 indexed
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_core_current(core_current_t* current_data, uint16_t* core_index);

/**
 * @brief Poll SENSOR_FIFO_PSTATE_TELEMETRY_HW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] state_data - destination pointer for entry read from fifo
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_core_pstate(pstate_telem_t* state_data);

/**
 * @brief Poll SENSOR_FIFO_SCP_MSG_TELEMETRY_HW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] plimit_msg - destination pointer for entry read from fifo
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_scp_message(plimit_telem_msg_t* plimit_msg);

/**
 * @brief Poll SENSOR_FIFO_PVT_TEMP_FW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] pvt_temperature - destination pointer for entry read from fifo
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature);

/**
 * @brief Poll SENSOR_FIFO_PVT_VOLTAGE_FW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] pvt_voltage - destination pointer for entry read from fifo
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_soc_pvt_voltage(soc_pvt_voltage_t* pvt_voltage);

/**
 * @brief Poll SENSOR_FIFO_DIMM_TEMP_FW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] dimm_info - destination pointer for entry read from fifo
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_dimm_info(sensor_ram_dimm_info_t* dimm_info);

/**
 * @brief Poll SENSOR_FIFO_VR_TEMP_FW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] vr_temperature - destination pointer for entry read from fifo
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_vr_temperature(vr_temp_t* vr_temperature);

/**
 * @brief Poll SENSOR_FIFO_VR_CURRENT_FW and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] vr_current - destination pointer for entry read from fifo
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_vr_current(vr_current_t* vr_current);
