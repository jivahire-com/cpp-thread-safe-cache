//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_telemetry.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <DfwkClient.h> // for PDFWK_DEVICE_HEADER, PDFWK_INTERFACE_HEADER
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <pwr_telemetry_data.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <telmain_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}

/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
extern core_runtime_info_t core[];
extern tile_runtime_info_t tile[];
extern soc_runtime_info_t soc_info;
}

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

//
// Tests
//


// Test for telmain_runtime_info_mgr
TEST_FUNCTION(test_telmain_runtime_info_mgr, test_setup, test_teardown)
{
    // runtime information manager test
    sensor_ram_poll_status_t status_expected = { .curr_data_is_valid = false, .more_entries = false };
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);		
    telmain_runtime_info_mgr();
}

// Test for telmain_log_tile_temperature
TEST_FUNCTION(test_telmain_log_tile_temperature, test_setup, test_teardown)
{
    // runtime information manager test
    tile_temp_t temperature_data = {
        .timestamp = 0,
        .temp0 = {
            .temp_valid = 1,
            .max_id = 7,
            .max_temp = 80,
            .core0 = 40,
            .core1 = 40,
        },
        .temp1 = {
            .temp0 = 20,
            .temp1 = 30,
            .temp2 = 40,
            .temp3 = 40,
        },
        .temp2 = {
            .temp4 = 30,
            .temp5 = 20,
            .temp6 = 40,
            .temp7 = 80,
        },
    };

    uint8_t index = 0;
    telmain_log_tile_temperature(&temperature_data, index);

    index = 1;
    temperature_data.temp1.temp1 = 60;
    telmain_log_tile_temperature(&temperature_data, index);
}

// Test for telmain_log_tile_voltage
TEST_FUNCTION(test_telmain_log_tile_voltage, test_setup, test_teardown)
{
    // runtime information manager test
    tile_voltage_t voltage_data = {
        .timestamp = 0,
        .data = {
            .vcore0 = 5,
            .vcore1 = 6,
            .vcpu = 10,
            .vsys = 80,
        },
    };

    uint8_t index = 0;
    telmain_log_tile_voltage(&voltage_data, index);
	
	// Check core 0 and core 1 voltage
    assert_int_equal(core[index].voltage.instantaneous, (uint16_t)(voltage_data.data.vcore0 * 1000));	
    assert_int_equal(core[index+1].voltage.instantaneous, (uint16_t)(voltage_data.data.vcore1 * 1000));	
    assert_int_equal(tile[index].vcpu.instantaneous , (uint16_t)(voltage_data.data.vcpu * 1000));		
    assert_int_equal(tile[index].vsys.instantaneous , (uint16_t)(voltage_data.data.vsys * 1000));		
}

// Test for telmain_log_core_current
TEST_FUNCTION(test_telmain_log_core_current, test_setup, test_teardown)
{
    // runtime information manager test
    core_current_t current_data = {
        .timestamp = 0,
        .data = {
            .avg = 100,
            .min = 10,
            .max = 150,
            .volt = 100,
            .pwr = 50,
            .pstate = 12,
            .change = 1,
            .mpam_id_low = 0,
            .mpam_id_high = 0,
            .cstate = 0,
        },
    };

    uint8_t index = 0;
    telmain_log_core_current(&current_data, index);

    current_data.timestamp = 100;
    current_data.data.pstate = 13;
    current_data.data.change = 0;
    telmain_log_core_current(&current_data, index);
	
	// Check core 0 current
    assert_int_equal(core[index].current_pkt_timestamp, (current_data.timestamp ));	
    assert_int_equal(core[index].current.instantaneous, (uint16_t)(current_data.data.avg * CORE_CURRENT_CONVERSION_FACTOR ));	
    assert_int_equal(core[index].ldo_voltage, (uint16_t)(current_data.data.volt ));
    assert_int_equal(core[index].current_tel_pstate, (current_data.data.pstate ));
    assert_int_equal(core[index].current_mpam_id, (current_data.data.mpam_id_low ));		
}

// Test for telmain_log_core_pstate
TEST_FUNCTION(test_telmain_log_core_pstate, test_setup, test_teardown)
{
    // runtime information manager test
    pstate_telem_t pstate_data = {
        .timestamp = 0,
        .data = {
            .pstate = 12,
            .throttle_status = 0,
            .vm_throttle_pri = 0,			
            .max_pstate = 0,			
            .cstate = 0,
            .plimit = 5,
            .core = 0,
            .mpam_low = 0,
            .mpam_high = 0,
            .boost_priority = 0,
        },
    };

    // Baseline log
    telmain_log_core_pstate(&pstate_data);

    // Do Pstate change
    pstate_data.timestamp = 100;
    pstate_data.data.pstate = 5;
    telmain_log_core_pstate(&pstate_data);


    // Do Throttling start
    pstate_data.timestamp = 120;
    pstate_data.data.throttle_status = RACK_LIMIT_THROTTLING_START;
    telmain_log_core_pstate(&pstate_data);


    // Do Throttling start
    pstate_data.timestamp = 500;
    pstate_data.data.throttle_status = RACK_LIMIT_THROTTLING_END;
    telmain_log_core_pstate(&pstate_data);

    // Do Cstate change
    pstate_data.timestamp = 110;
    pstate_data.data.pstate = 31;
    pstate_data.data.cstate = 3;
    telmain_log_core_pstate(&pstate_data);
}

// Test for telmain_log_vr_temp
TEST_FUNCTION(test_telmain_log_vr_temp, test_setup, test_teardown)
{
    // runtime information manager test
    vr_temp_t vr_temperature = {
        .timestamp = 0,
        .vr_temp = {15, 25, 35, 45, 55, 65, 75, 85},
    };

    // Baseline log
    telmain_log_vr_temp(&vr_temperature);
	
	// Check VR Temp
	for(uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
	{
		assert_int_equal(soc_info.rail[index].temperature.instantaneous, (vr_temperature.vr_temp[index]));	
	}	
}

// Test for telmain_log_vr_current
TEST_FUNCTION(test_telmain_log_vr_current, test_setup, test_teardown)
{
    // runtime information manager test
    vr_current_t data = {
        .timestamp = 0,
        .vr_current = {10, 20, 30, 40, 50, 60, 70, 80},
        .vr_voltage = {1000, 900, 800, 700, 600, 700, 800, 900},		
    };

    // Baseline log
    telmain_log_vr_current(&data);
	
	// Check VR Current and voltage
	for(uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
	{
		assert_int_equal(soc_info.rail[index].current.instantaneous, (data.vr_current[index]));
		assert_int_equal(soc_info.rail[index].voltage.instantaneous, (data.vr_voltage[index]));			
	}		
}



