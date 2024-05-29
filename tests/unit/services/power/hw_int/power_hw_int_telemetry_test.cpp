//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_telemetry_test.cpp
 * Power service telemetry interface tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {

#include <CMockaWrapper.h>  // for CmockaWrapperTest, assert_int_equal
#include <power_hw_int_i.h> // for power_init_core, power_init_soc

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

} // extern "C"

/*------------- Functions ----------------*/
//
// Mocks
//

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

POWER_TEST(power_telemetry_init_config, NULL, NULL)
{
    // TODO:  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1811872
    // update tests with telemetry / sensor ram driver integration

    // for now, do basic sanity checks on the telemetry config

    power_telcfg_t telemetry_config;

    power_telemetry_init_config(&telemetry_config);

    // no buffer sizes should be zero
    assert_true(telemetry_config.current_telem_buffer_size > 0);
    assert_true(telemetry_config.temp_telem_buffer_size > 0);
    assert_true(telemetry_config.volt_telem_buffer_size > 0);
    // no entry sizes should be zero
    assert_true(telemetry_config.current_telem_entry_size > 0);
    assert_true(telemetry_config.temp_telem_entry_size > 0);
    assert_true(telemetry_config.volt_telem_entry_size > 0);
    assert_true(telemetry_config.vrtemp_telem_entry_size > 0);
    assert_true(telemetry_config.vrcurr_telem_entry_size > 0);
    assert_true(telemetry_config.socpvtvolt_telem_entry_size > 0);
    assert_true(telemetry_config.socpvttemp_telem_entry_size > 0);
}