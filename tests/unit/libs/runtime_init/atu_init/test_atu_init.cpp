//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_atu_init.cpp
 * ATU init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <FpFwUtils.h>
#include <atu_init.h>
#include <atu_lib.h>
#include <fpfw_init.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_atu_svc;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}
void __wrap_atu_svc_init(atu_service_t* atu_service, PDFWK_SCHEDULE schedule)
{
    assert_non_null(atu_service->atu_device);
    assert_non_null(schedule);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

int __wrap_atu_init(atu_id_t atu_id, const atu_map_entry_t* atu_map_config, uint32_t num)
{
    function_called();
    assert_true(atu_id < ATU_ID_MAX);
    assert_non_null(atu_map_config);
    FPFW_UNUSED(num);

    return SILIBS_SUCCESS;
}
}

//
// Tests
//
TEST_FUNCTION(test_atu_svc, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_fpfw_init_get_handle, &test_host);
    will_return(__wrap_idsw_get_die_id, 1);
    expect_function_call_any(__wrap_atu_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_atu_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}