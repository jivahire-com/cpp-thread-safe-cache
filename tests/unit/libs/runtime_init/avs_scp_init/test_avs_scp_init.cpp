//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_avs_scp_init.cpp
 * AVS Init test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include "debug.h" // for UNUSED

#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <scp_avs_driver.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_avs0;
extern fpfw_init_component_t _fpfw_component_avs1;
extern fpfw_init_component_t _fpfw_component_avs2;
extern fpfw_init_component_t _fpfw_component_avs3;
extern fpfw_init_component_t _fpfw_component_avs0_int;
extern fpfw_init_component_t _fpfw_component_avs1_int;
extern fpfw_init_component_t _fpfw_component_avs2_int;
extern fpfw_init_component_t _fpfw_component_avs3_int;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_scp_avs_init(pscp_avs_device avsDevice, DFWK_SCHEDULE* scheduler)
{
    assert_non_null(avsDevice);
    check_expected(scheduler);
}

void __wrap_scp_avs_interface_initialize(pscp_avs_device Device, pscp_avs_interface_t Interface)
{
    check_expected(Device);
    UNUSED(Interface);
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    check_expected_ptr(id);
    return mock_type(void*);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

//
// Tests
//
TEST_FUNCTION(test_avs0_scp, nullptr, nullptr)
{
    DFWK_THREADX_HOST test_host = {};
    fpfw_init_component_id_t dfwk_id = "dfwk";

    // Set up expectations
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_value(__wrap_scp_avs_init, scheduler, &(test_host.Schedule));
    expect_memory(__wrap_fpfw_init_get_handle, id, dfwk_id, sizeof(fpfw_init_component_id_t));

    // Call API under test
    _fpfw_component_avs0.init_fn();
}

TEST_FUNCTION(test_avs1_scp, nullptr, nullptr)
{
    DFWK_THREADX_HOST test_host = {};
    fpfw_init_component_id_t dfwk_id = "dfwk";

    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_value(__wrap_scp_avs_init, scheduler, &(test_host.Schedule));
    expect_memory(__wrap_fpfw_init_get_handle, id, dfwk_id, sizeof(fpfw_init_component_id_t));

    // Call API under test
    _fpfw_component_avs1.init_fn();
}

TEST_FUNCTION(test_avs2_scp, nullptr, nullptr)
{
    DFWK_THREADX_HOST test_host = {};
    fpfw_init_component_id_t dfwk_id = "dfwk";

    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_value(__wrap_scp_avs_init, scheduler, &(test_host.Schedule));
    expect_memory(__wrap_fpfw_init_get_handle, id, dfwk_id, sizeof(fpfw_init_component_id_t));

    // Call API under test
    _fpfw_component_avs2.init_fn();
}

TEST_FUNCTION(test_avs3_scp, nullptr, nullptr)
{
    DFWK_THREADX_HOST test_host = {};
    fpfw_init_component_id_t dfwk_id = "dfwk";

    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die); 
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_value(__wrap_scp_avs_init, scheduler, &(test_host.Schedule));
    expect_memory(__wrap_fpfw_init_get_handle, id, dfwk_id, sizeof(fpfw_init_component_id_t));

    // Call API under test
    _fpfw_component_avs3.init_fn();
}

TEST_FUNCTION(test_avs0_scp_init, nullptr, nullptr)
{
    // Set up expectations
    scp_avs_device_t test_avs_device = {
        .config = {},
    };
    fpfw_init_component_id_t avs_id = "avs0";

    will_return(__wrap_fpfw_init_get_handle, &test_avs_device);
    expect_value(__wrap_scp_avs_interface_initialize, Device, &test_avs_device);
    expect_memory(__wrap_fpfw_init_get_handle, id, avs_id, sizeof(fpfw_init_component_id_t));

    // Call API under test
    _fpfw_component_avs0_int.init_fn();
}

TEST_FUNCTION(test_avs1_scp_init, nullptr, nullptr)
{
   // Set up expectations
    scp_avs_device_t test_avs_device = {
        .config = {},
    };
    fpfw_init_component_id_t avs_id = "avs1";
    const auto test_die = (KNG_DIE_ID)0;

    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_fpfw_init_get_handle, &test_avs_device);
    expect_value(__wrap_scp_avs_interface_initialize, Device, &test_avs_device);
    expect_memory(__wrap_fpfw_init_get_handle, id, avs_id, sizeof(fpfw_init_component_id_t));

    // Call API under test
    _fpfw_component_avs1_int.init_fn();
}

TEST_FUNCTION(test_avs2_scp_init, nullptr, nullptr)
{
    // Set up expectations
    scp_avs_device_t test_avs_device = {
        .config = {},
    };
    fpfw_init_component_id_t avs_id = "avs2";
    const auto test_die = (KNG_DIE_ID)0;

    will_return(__wrap_idsw_get_die_id, test_die);

    will_return(__wrap_fpfw_init_get_handle, &test_avs_device);
    expect_value(__wrap_scp_avs_interface_initialize, Device, &test_avs_device);
    expect_memory(__wrap_fpfw_init_get_handle, id, avs_id, sizeof(fpfw_init_component_id_t));

    // Call API under test
    _fpfw_component_avs2_int.init_fn();
}

TEST_FUNCTION(test_avs3_scp_init, nullptr, nullptr)
{
    // Set up expectations
    scp_avs_device_t test_avs_device = {
        .config = {},
    };
    fpfw_init_component_id_t avs_id = "avs3";
    const auto test_die = (KNG_DIE_ID)0;

    will_return(__wrap_idsw_get_die_id, test_die);

    will_return(__wrap_fpfw_init_get_handle, &test_avs_device);
    expect_value(__wrap_scp_avs_interface_initialize, Device, &test_avs_device);
    expect_memory(__wrap_fpfw_init_get_handle, id, avs_id, sizeof(fpfw_init_component_id_t));

    // Call API under test
    _fpfw_component_avs3_int.init_fn();
}
}