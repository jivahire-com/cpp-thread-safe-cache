//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_health_monitor_init.cpp
 * Tests the init of the health monitor
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <accelip_id.h> // for ACCEL_ID_SDM, ACCEL_ID_CDED
#include <fpfw_init.h>  // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_hm_svc;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_hm_pre_ddr_init(hm_config_t* hm_config)
{
    assert_non_null(hm_config->mscp_ghes_base);
    assert_non_null(hm_config->mscp_ghes_error_record_addr_table_base);
    assert_non_null(hm_config->mscp_ghes_ack_addr_table_base);
    assert_non_null(hm_config->mscp_ghes_error_record_addr_base);
    function_called();
}
}

//
// Tests
//
TEST_FUNCTION(hm_svc, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, (KNG_DIE_ID)1);
    expect_function_call_any(__wrap_hm_pre_ddr_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}
