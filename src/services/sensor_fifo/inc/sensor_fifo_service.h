//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Sensor Fifo Service Interface
 */

#pragma once

/*----------- Nested includes ------------*/
#include <sensor_fifo_driver_interface.h>
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
    TEMPERATURE_TELEMETRY_HW_FIFO = 0,
    VOLTAGE_TELEMETRY_HW_FIFO = 1,
    CURRENT_TELEMETRY_HW_FIFO = 2,
    PSTATE_TELEMETRY_HW_FIFO = 3,
    SCP_MESSAGING_HW_FIFO = 4,
    SOC_PVT_TEMP_FW_FIFO = 5,
    SOC_PVT_VOLTAGE_FW_FIFO = 6,
    DIMM_TEMP_FW_FIFO = 7,
    VR_TEMP_BUFFER_FW_FIFO = 8,
    VR_CURRENT_BUFFER_FW_FIFO = 9,
    MAX_FIFO_ID = 10
} SENSOR_FIFO_ID;

#define LAST_CORE_TILE_SENSOR_FIFO CURRENT_TELEMETRY_FIFO

/**
 * @brief Information describing a single FIFO
 *
 */
typedef struct {
    uint32_t start_address;
    uint32_t end_address;
    uint32_t buffersize;
    uint8_t entry_size;
    uint8_t stride_size;
    SENSOR_FIFO_ID fifo_id;
    bool enabled;
} sensor_fifo_properties_t;

/**
 * @brief Single entry for  SOC_PVT_TEMP_FW_FIFO
 *
 */
typedef struct __attribute__((packed)) {
    uint64_t time_stamp;
    uint16_t sensor_temp[NUMBER_OF_SOC_TEMP_SENSORS];
    uint8_t  padding[2]; // needs to be multiple of QUADWORD_SIZE
} soc_pvt_temp_t;

/**
 * @brief Single entry for  SOC_PVT_VOLTAGE_FW_FIFO
 *
 */
typedef struct __attribute__((packed)) {
    uint64_t time_stamp;
    uint16_t sensor_voltage[NUMBER_OF_SOC_VOLT_MON_SENSORS];
    uint8_t  padding[4]; // needs to be multiple of QUADWORD_SIZE
} soc_pvt_voltage_t;

/**
 * @brief Single entry for  VR_TEMP_BUFFER_FW_FIFO
 *
 */
typedef struct __attribute__((packed)) {
    uint64_t time_stamp;
    uint16_t vr_temp[MAX_NUM_OF_VR_RAILS];
} vr_temp_t;

/**
 * @brief Single entry for VR_CURRENT_BUFFER_FW_FIFO
 *
 */
typedef struct __attribute__((packed)) {
    uint64_t time_stamp;
    uint16_t vr_current[MAX_NUM_OF_VR_RAILS];
    uint16_t vr_voltage[MAX_NUM_OF_VR_RAILS];
} vr_current_t;

/**
 * @brief Single entry for  DIMM_TEMP_FW_FIFO
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
 * @brief Initialize the service.
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] driver_interface - Pointer to a platform driver interface
 */
void sensor_fifo_svc_initialize(sensor_fifo_driver_interface_t* driver_interface);

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
void sensor_fifo_svc_enable(SENSOR_FIFO_ID fifo);

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
void sensor_fifo_svc_disable(SENSOR_FIFO_ID fifo);

/**
 * @brief Empties and discards all of the entries
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] fifo - Select the specific fifo
 */
void sensor_fifo_svc_discard(SENSOR_FIFO_ID fifo);

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
void sensor_fifo_svc_get_properties(SENSOR_FIFO_ID fifo, sensor_fifo_properties_t* properties);

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
 * @brief - Add an entry to SOC_PVT_TEMP_FW_FIFO
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
 * @brief - Add an entry to SOC_PVT_VOLTAGE_FW_FIFO
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
 * @brief - Add an entry to DIMM_TEMP_FW_FIFO
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
 * @brief - Add an entry to VR_TEMP_BUFFER_FW_FIFO
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
 * @brief - Add an entry to VR_CURRENT_BUFFER_FW_FIFO
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
 * @brief Poll TEMPERATURE_TELEMETRY_HW_FIFO and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] temperature_data - destination pointer for entry read from fifo
 * @param[out] tile_index - destination pointer for entry's tile index
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_tile_temperature(sensor_telem_t* temperature_data, uint8_t* tile_index);

/**
 * @brief Poll VOLTAGE_TELEMETRY_HW_FIFO and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] voltage_data - destination pointer for entry read from fifo
 * @param[out] tile_index - destination pointer for entry's tile index
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_tile_voltage(volt_data_t* voltage_data, uint8_t* tile_index);

/**
 * @brief Poll CURRENT_TELEMETRY_HW_FIFO and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] time_stamp - destination pointer for timestamp of entry
 * @param[out] current_data - destination pointer for entry read from fifo
 * @param[out] tile_index - destination pointer for entry's tile index
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_tile_current(uint64_t* time_stamp, current_data_t* current_data, uint8_t* tile_index);

/**
 * @brief Poll PSTATE_TELEMETRY_HW_FIFO and read out an entry if available
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
 * @brief Poll SCP_MESSAGING_HW_FIFO and read out an entry if available
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[out] plimit_msg - destination pointer for entry read from fifo
 * @retval sensor_ram_poll_status_t - See documentation for return values
 */
sensor_ram_poll_status_t sensor_fifo_svc_poll_scp_message(plimit_msg_telem_t* plimit_msg);

/**
 * @brief Poll SOC_PVT_TEMP_FW_FIFO and read out an entry if available
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
 * @brief Poll SOC_PVT_VOLTAGE_FW_FIFO and read out an entry if available
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
 * @brief Poll DIMM_TEMP_FW_FIFO and read out an entry if available
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
 * @brief Poll VR_TEMP_BUFFER_FW_FIFO and read out an entry if available
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
 * @brief Poll VR_CURRENT_BUFFER_FW_FIFO and read out an entry if available
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
