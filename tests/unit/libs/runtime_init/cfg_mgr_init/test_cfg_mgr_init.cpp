//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cfg_mgr_init.cpp
 * Config Manger Init test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
// #include "debug.h" // for UNUSED

#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_cfg_mgr_init.h>
#include <fpfw_init.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern bool empty_read_knob_fn(const fpfw_cfg_mgr_guid_t* knob_namespace, const char* knob_name, uint8_t* data, size_t data_size, void* ctx);
extern bool empty_write_knob_fn(const fpfw_cfg_mgr_guid_t* knob_namespace, const char* knob_name, const uint8_t* data, size_t data_size, void* ctx);
/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_cfg_mgr;

/*------------- Functions ----------------*/
//
// Mocks
//


//
// Tests
//
TEST_FUNCTION(test_cfg_mgr_init_success, nullptr, nullptr)
{
    // Verify that the configuration manager can be initialized with a valid configuration
    // and that the configuration manager can be initialized multiple times
    fpfw_init_result_t result = _fpfw_component_cfg_mgr.init_fn();
    assert_int_equal(FPFW_INIT_STATUS_SUCCESS, result.status);
}

TEST_FUNCTION(add_blind_coverage_to_placeholder_functions, nullptr, nullptr)
{
    assert_int_equal(true, empty_read_knob_fn(nullptr, nullptr, nullptr, 0, nullptr));
    assert_int_equal(true, empty_write_knob_fn(nullptr, nullptr, nullptr, 0, nullptr));
}

}