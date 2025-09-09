//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file out_of_band_tlm_cmpnt.c
 * Out of band telemetry processing
 */

/*------------- Includes -----------------*/
#include "out_of_band_tlm_cmpnt.h"

#include "out_of_band_tlm_cmpnt_i.h"

#include <DbgPrint.h>
#include <FpFwAssert.h>
#include <data_proc_tlm_cmpnt.h>
#include <exec_tlm_cmpnt.h>
#include <fpfw_pldm_service.h>
#include <telemetry_events_i.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

// Number of iterations before printing non-critical sensor data

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static bool is_cache_valid(void);

/*-- Declarations (Statics and globals) --*/
static uint8_t this_die_id;
pldm_numeric_sensor_context_t* active_sensor = NULL;
pwr_tlm_numeric_sensor_get_handler_t active_handler = NULL;
pldm_numeric_sensor_context_t pwr_tlm_numeric_sensor_ctxts[NUM_PWR_TLM_PLDM_SENSORS];
pwr_tlm_numeric_sensor_get_handler_t pwr_tlm_numeric_sensor_get_handlers[NUM_PWR_TLM_PLDM_SENSORS];
uint32_t pwr_tlm_oob_sensor_cache[NUM_PWR_TLM_PLDM_SENSORS];
static TX_EVENT_FLAGS_GROUP pldm_sync;

// Cache validity tracking
bool cache_is_valid = false;
uint64_t cache_timestamp_uS = 0;
uint32_t oob_print_non_critical_sensors_iter = 0;
#define CACHE_VALIDITY_TIMEOUT_US (10000000ULL) // 10 seconds in microseconds

/*------------- Functions ----------------*/
static bool is_cache_valid(void)
{
    if (!cache_is_valid)
    {
        return false;
    }

    uint64_t current_time_us = exec_tlm_cmpnt_get_timestamp_microseconds();
    uint64_t cache_age_us = current_time_us - cache_timestamp_uS;

    if (cache_age_us > CACHE_VALIDITY_TIMEOUT_US)
    {
        cache_is_valid = false;
        return false;
    }

    return true;
}

void out_of_band_tlm_cmpnt_init(uint8_t die_id)
{
    this_die_id = die_id;

    if (this_die_id != PRIMARY_DIE_ID)
    {
        return;
    }

    UINT txStatus = tx_event_flags_create(&pldm_sync, "PwrTlmPldmSync");
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    //-------------------------------------------------------------------------------------------
    // config data is copied internally so this structure can be reused for all sensors
    pldm_numeric_sensor_config_t config = {.sensor_id = PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS,
                                           .notifications.on_sensor_get = on_pwr_tlm_numeric_sensor_get_ext_entry,
                                           .notifications.context = pwr_tlm_oob_get_max_soc_temp};

    // out of band sensors are critical so will assert if not registered
    fpfw_status_t status = fpfw_pldm_service_register_numeric_sensor(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)],
        &config);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

    //-------------------------------------------------------------------------------------------
    config.sensor_id = PLDM_SENSOR_ID_POWER_TLM_SOC_PWR_NUM_SENS;
    config.notifications.context = pwr_tlm_oob_get_soc_pwr;
    status = fpfw_pldm_service_register_numeric_sensor(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_PWR_NUM_SENS)],
        &config);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

    //-------------------------------------------------------------------------------------------
    config.sensor_id = PLDM_SENSOR_ID_POWER_TLM_DIMM_TMP_MAX_NUM_SENS;
    config.notifications.context = pwr_tlm_oob_get_max_dimm_temp;
    status = fpfw_pldm_service_register_numeric_sensor(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_DIMM_TMP_MAX_NUM_SENS)],
        &config);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

    //-------------------------------------------------------------------------------------------
    config.sensor_id = PLDM_SENSOR_ID_POWER_TLM_DIMM_TOTAL_PWR_NUM_SENS;
    config.notifications.context = pwr_tlm_oob_get_dimm_total_pwr;
    status = fpfw_pldm_service_register_numeric_sensor(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_DIMM_TOTAL_PWR_NUM_SENS)],
        &config);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

    //-------------------------------------------------------------------------------------------
    config.sensor_id = PLDM_SENSOR_ID_POWER_TLM_VR_TMP_MAX_NUM_SENS;
    config.notifications.context = pwr_tlm_oob_get_max_vr_temp;
    status = fpfw_pldm_service_register_numeric_sensor(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_VR_TMP_MAX_NUM_SENS)],
        &config);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

    //-------------------------------------------------------------------------------------------
    config.sensor_id = PLDM_SENSOR_ID_POWER_TLM_SOC_AVG_FREQ_NUM_SENS;
    config.notifications.context = pwr_tlm_oob_get_soc_avg_freq;
    status = fpfw_pldm_service_register_numeric_sensor(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_AVG_FREQ_NUM_SENS)],
        &config);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

    //-------------------------------------------------------------------------------------------
    for (uint16_t sensor_id = PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_TMP_00_NUM_SENS;
         sensor_id <= PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_TMP_11_NUM_SENS;
         sensor_id++)
    {
        config.sensor_id = sensor_id;
        config.notifications.context = pwr_tlm_oob_get_dimm_avg_temp;
        status = fpfw_pldm_service_register_numeric_sensor(&pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)],
                                                           &config);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
    }

    //-------------------------------------------------------------------------------------------
    for (uint16_t sensor_id = PLDM_SENSOR_ID_POWER_TLM_DIMM_MAX_TMP_00_NUM_SENS;
         sensor_id <= PLDM_SENSOR_ID_POWER_TLM_DIMM_MAX_TMP_11_NUM_SENS;
         sensor_id++)
    {
        config.sensor_id = sensor_id;
        config.notifications.context = pwr_tlm_oob_get_dimm_max_temp;
        status = fpfw_pldm_service_register_numeric_sensor(&pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)],
                                                           &config);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
    }

    //-------------------------------------------------------------------------------------------
    for (uint16_t sensor_id = PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_PWR_00_NUM_SENS;
         sensor_id <= PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_PWR_11_NUM_SENS;
         sensor_id++)
    {
        config.sensor_id = sensor_id;
        config.notifications.context = pwr_tlm_oob_get_dimm_avg_pwr;
        status = fpfw_pldm_service_register_numeric_sensor(&pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)],
                                                           &config);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
    }
}

void on_pwr_tlm_numeric_sensor_get_ext_entry(pldm_numeric_sensor_context_t* p_sensor, void* p_context)
{
    if (!exec_tlm_cmpnt_is_oob_data_valid())
    {
        fpfw_pldm_service_numeric_sensor_not_ready(p_sensor);
        return;
    }

    // current PDLM implementation only handles 1 sensor get in flight at a time.
    // it relies on calling fpfw_pldm_service_numeric_sensor_set() before this api returns.
    // this is called from the PLDM service thread, and it is not desirable to access data
    // from power telemetry as it is updated on the telemetry service thread.
    // save the active sensor,  notify power tlm thread and block here until tlm thread calls
    // fpfw_pldm_service_numeric_sensor_set() to complete the request.  the handler on the pwr tlm
    // thread will then unblock this thread
    if ((active_sensor != NULL) || (active_handler != NULL))
    {
        FPFW_ET_LOG(OobSensorAlreadyActive, (uint32_t)active_sensor, (uint32_t)active_handler);
        return; // already processing a sensor get request
    }
    active_sensor = p_sensor;
    active_handler = p_context;

    // this will result in pwr tlm thread calling out_of_band_tlm_cmpnt_handle_new_pldm_requests()
    exec_tlm_cmpnt_notify_new_out_of_band_pldm_request();

    ULONG current_bits;
    UINT txStatus = tx_event_flags_get(&pldm_sync, RELEASE_PLDM_SERVICE, TX_OR_CLEAR, &current_bits, TX_WAIT_FOREVER);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void out_of_band_tlm_cmpnt_handle_new_pldm_requests(void)
{
    fpfw_pldm_composite_value_t composite_value = {0};

    if ((active_sensor != NULL) && (active_handler != NULL))
    {
        uint16_t sensor_id = active_sensor->sensor_state.config.sensor_id;

        // Call the handler to get the sensor value
        active_handler(sensor_id, &composite_value);
        fpfw_status_t status = fpfw_pldm_service_numeric_sensor_set(active_sensor, composite_value);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0); // low probability of failure

        // Mark cache as valid and update timestamp when sensor handlers are called
        cache_is_valid = true;
        cache_timestamp_uS = exec_tlm_cmpnt_get_timestamp_microseconds();

        active_sensor = NULL;  // Clear the active sensor after use
        active_handler = NULL; // Clear the handler after use
    }
    else
    {
        FPFW_ET_LOG(OobNoActiveHandler);
    }

    UINT txStatus = tx_event_flags_set(&pldm_sync, RELEASE_PLDM_SERVICE, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void pwr_tlm_oob_get_max_soc_temp(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if (sensor_id == PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)
    {
        uint16_t temp_value = data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC();
        sensor_value->numeric.u16 = temp_value;
        pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)] = (uint32_t)temp_value;
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS, sensor_id);
    }
}

void pwr_tlm_oob_get_soc_pwr(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if (sensor_id == PLDM_SENSOR_ID_POWER_TLM_SOC_PWR_NUM_SENS)
    {
        uint32_t power_value = data_proc_tlm_cmpnt_get_oob_soc_pwr_mW();
        sensor_value->numeric.u32 = power_value;
        pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)] = power_value;
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_SOC_PWR_NUM_SENS, sensor_id);
    }
}

void pwr_tlm_oob_get_max_dimm_temp(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if (sensor_id == PLDM_SENSOR_ID_POWER_TLM_DIMM_TMP_MAX_NUM_SENS)
    {
        uint16_t temp_value = data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC();
        sensor_value->numeric.u16 = temp_value;
        pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)] = (uint32_t)temp_value;
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_DIMM_TMP_MAX_NUM_SENS, sensor_id);
    }
}

void pwr_tlm_oob_get_dimm_total_pwr(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if (sensor_id == PLDM_SENSOR_ID_POWER_TLM_DIMM_TOTAL_PWR_NUM_SENS)
    {
        uint32_t power_value = data_proc_tlm_cmpnt_get_oob_dimm_total_pwr_mW();
        sensor_value->numeric.u32 = power_value;
        pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)] = power_value;
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_DIMM_TOTAL_PWR_NUM_SENS, sensor_id);
    }
}

void pwr_tlm_oob_get_max_vr_temp(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if (sensor_id == PLDM_SENSOR_ID_POWER_TLM_VR_TMP_MAX_NUM_SENS)
    {
        uint16_t temp_value = data_proc_tlm_cmpnt_get_oob_crit_max_vr_temp_dC();
        sensor_value->numeric.u16 = temp_value;
        pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)] = (uint32_t)temp_value;
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_VR_TMP_MAX_NUM_SENS, sensor_id);
    }
}

void pwr_tlm_oob_get_soc_avg_freq(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if (sensor_id == PLDM_SENSOR_ID_POWER_TLM_SOC_AVG_FREQ_NUM_SENS)
    {
        uint16_t freq_value = data_proc_tlm_cmpnt_get_oob_soc_avg_freq_MHz();
        sensor_value->numeric.u16 = freq_value;
        pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)] = (uint32_t)freq_value;
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_SOC_AVG_FREQ_NUM_SENS, sensor_id);
    }
}

void pwr_tlm_oob_get_dimm_avg_temp(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if ((sensor_id >= PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_TMP_00_NUM_SENS) &&
        (sensor_id <= PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_TMP_11_NUM_SENS))
    {
        uint8_t dimm_idx = sensor_id - PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_TMP_00_NUM_SENS;
        uint16_t temp_value = data_proc_tlm_cmpnt_get_oob_dimm_avg_temp_dC(dimm_idx);
        sensor_value->numeric.u16 = temp_value;
        pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)] = (uint32_t)temp_value;
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_TMP_00_NUM_SENS, sensor_id);
    }
}

void pwr_tlm_oob_get_dimm_max_temp(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if ((sensor_id >= PLDM_SENSOR_ID_POWER_TLM_DIMM_MAX_TMP_00_NUM_SENS) &&
        (sensor_id <= PLDM_SENSOR_ID_POWER_TLM_DIMM_MAX_TMP_11_NUM_SENS))
    {
        uint8_t dimm_idx = sensor_id - PLDM_SENSOR_ID_POWER_TLM_DIMM_MAX_TMP_00_NUM_SENS;
        uint16_t temp_value = data_proc_tlm_cmpnt_get_oob_dimm_max_temp_dC(dimm_idx);
        sensor_value->numeric.u16 = temp_value;
        pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)] = (uint32_t)temp_value;
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_DIMM_MAX_TMP_00_NUM_SENS, sensor_id);
    }
}

void pwr_tlm_oob_get_dimm_avg_pwr(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if ((sensor_id >= PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_PWR_00_NUM_SENS) &&
        (sensor_id <= PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_PWR_11_NUM_SENS))
    {
        uint8_t dimm_idx = sensor_id - PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_PWR_00_NUM_SENS;
        uint32_t power_value = data_proc_tlm_cmpnt_get_oob_dimm_avg_pwr_mW(dimm_idx);
        sensor_value->numeric.u32 = power_value;
        pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(sensor_id)] = power_value;
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_PWR_00_NUM_SENS, sensor_id);
    }
}

void out_of_band_tlm_cmpnt_print_sensors(void)
{
    // Check if cache is valid, otherwise use direct API calls
    bool use_cache = is_cache_valid();

    uint16_t max_soc_temp_dC;
    uint32_t soc_pwr_mW;
    uint16_t max_dimm_tmp_dC;
    uint32_t dimm_total_pwr_mW;
    uint16_t soc_avg_freq_Mhz;

    if (use_cache)
    {
        max_soc_temp_dC =
            pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)];
        soc_pwr_mW = pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_PWR_NUM_SENS)];
        max_dimm_tmp_dC =
            pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_DIMM_TMP_MAX_NUM_SENS)];
        dimm_total_pwr_mW =
            pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_DIMM_TOTAL_PWR_NUM_SENS)];
        soc_avg_freq_Mhz =
            pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_AVG_FREQ_NUM_SENS)];
    }
    else
    {
        max_soc_temp_dC = data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC();
        soc_pwr_mW = data_proc_tlm_cmpnt_get_oob_soc_pwr_mW();
        max_dimm_tmp_dC = data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC();
        dimm_total_pwr_mW = data_proc_tlm_cmpnt_get_oob_dimm_total_pwr_mW();
        soc_avg_freq_Mhz = data_proc_tlm_cmpnt_get_oob_soc_avg_freq_MHz();
    }

    FPFW_DBGPRINT_ALWAYS("OOB CRITICAL SENSORS - START\n");
    FPFW_DBGPRINT_ALWAYS("SOC_TMP_MAX: %d dC\n", max_soc_temp_dC);
    FPFW_DBGPRINT_ALWAYS("SOC_PWR: %d mW\n", soc_pwr_mW);
    FPFW_DBGPRINT_ALWAYS("DIMM_TMP_MAX: %d dC\n", max_dimm_tmp_dC);
    FPFW_DBGPRINT_ALWAYS("DIMM_TOTAL_PWR: %d mW\n", dimm_total_pwr_mW);
    FPFW_DBGPRINT_ALWAYS("SOC_AVG_FREQ: %d MHz\n", soc_avg_freq_Mhz);
    FPFW_DBGPRINT_ALWAYS("OOB CRITICAL SENSORS - END\n");

    if (oob_print_non_critical_sensors_iter % OOB_PRINT_NON_CRITICAL_SENSORS_INTERVAL == 0)
    {
        FPFW_DBGPRINT_ALWAYS("OOB NON-CRITICAL SENSORS - START\n\n");
        // Print non-critical sensor data here
        for (uint8_t dimm_idx = 0; dimm_idx < NUMBER_OF_DIMMS_PER_SOC; dimm_idx++)
        {
            uint16_t dimm_avg_temp_dC;
            uint16_t dimm_max_temp_dC;
            uint32_t dimm_avg_pwr_mW;

            if (use_cache)
            {
                dimm_avg_temp_dC =
                    pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_TMP_00_NUM_SENS + dimm_idx)];
                dimm_max_temp_dC =
                    pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_DIMM_MAX_TMP_00_NUM_SENS + dimm_idx)];
                dimm_avg_pwr_mW =
                    pwr_tlm_oob_sensor_cache[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_DIMM_AVG_PWR_00_NUM_SENS + dimm_idx)];
            }
            else
            {
                dimm_avg_temp_dC = data_proc_tlm_cmpnt_get_oob_dimm_avg_temp_dC(dimm_idx);
                dimm_max_temp_dC = data_proc_tlm_cmpnt_get_oob_dimm_max_temp_dC(dimm_idx);
                dimm_avg_pwr_mW = data_proc_tlm_cmpnt_get_oob_dimm_avg_pwr_mW(dimm_idx);
            }

            FPFW_DBGPRINT_ALWAYS("DIMM[%d] AVG_TEMP: %d dC\n", dimm_idx, dimm_avg_temp_dC);
            FPFW_DBGPRINT_ALWAYS("DIMM[%d] MAX_TEMP: %d dC\n", dimm_idx, dimm_max_temp_dC);
            FPFW_DBGPRINT_ALWAYS("DIMM[%d] AVG_PWR: %d mW\n\n", dimm_idx, dimm_avg_pwr_mW);
        }

        FPFW_DBGPRINT_ALWAYS("OOB NON-CRITICAL SENSORS - END\n");
    }

    oob_print_non_critical_sensors_iter++;
}