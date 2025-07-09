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

#include <FpFwAssert.h>
#include <data_proc_tlm_cmpnt.h>
#include <exec_tlm_cmpnt.h>
#include <fpfw_pldm_service.h>
#include <telemetry_events_i.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t this_die_id;
pldm_numeric_sensor_context_t* active_sensor = NULL;
pwr_tlm_numeric_sensor_get_handler_t active_handler = NULL;
pldm_numeric_sensor_context_t pwr_tlm_numeric_sensor_ctxts[NUM_PWR_TLM_PLDM_SENSORS];
pwr_tlm_numeric_sensor_get_handler_t pwr_tlm_numeric_sensor_get_handlers[NUM_PWR_TLM_PLDM_SENSORS];
static TX_EVENT_FLAGS_GROUP pldm_sync;

/*------------- Functions ----------------*/
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
    pldm_numeric_sensor_config_t config = {.sensor_id = PLDM_SENSOR_ID_POWER_TLM_SOC_TEMP_MAX_NUM_SENS,
                                           .notifications.on_sensor_get = on_pwr_tlm_numeric_sensor_get_ext_entry,
                                           .notifications.context = pwr_tlm_oob_get_max_soc_temp};

    // out of band sensors are critical so will assert if not registered
    fpfw_status_t status = fpfw_pldm_service_register_numeric_sensor(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TEMP_MAX_NUM_SENS)],
        &config);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

    //-------------------------------------------------------------------------------------------
    config.sensor_id = PLDM_SENSOR_ID_POWER_TLM_DIMM_TEMP_MAX_NUM_SENS;
    config.notifications.context = pwr_tlm_oob_get_max_dimm_temp;
    status = fpfw_pldm_service_register_numeric_sensor(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_DIMM_TEMP_MAX_NUM_SENS)],
        &config);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);
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
    if (sensor_id == PLDM_SENSOR_ID_POWER_TLM_SOC_TEMP_MAX_NUM_SENS)
    {
        sensor_value->numeric.u16 = data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC();
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_SOC_TEMP_MAX_NUM_SENS, sensor_id);
    }
}

void pwr_tlm_oob_get_max_dimm_temp(uint16_t sensor_id, fpfw_pldm_composite_value_t* sensor_value)
{
    if (sensor_id == PLDM_SENSOR_ID_POWER_TLM_DIMM_TEMP_MAX_NUM_SENS)
    {
        sensor_value->numeric.u16 = data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC();
    }
    else
    {
        FPFW_ET_LOG(UnexpectedSensorId, PLDM_SENSOR_ID_POWER_TLM_DIMM_TEMP_MAX_NUM_SENS, sensor_id);
    }
}