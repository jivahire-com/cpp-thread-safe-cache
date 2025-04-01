//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ib_telm_hsp_init.cpp
 * Implement unit tests for in band telemetry hsp initialization
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h>     // for FPFW_UNUSED
#include <fpfw_icc_base.h> // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_ib_telm_hsp;
static uint32_t dummy_icc_ctx = 0;

/*------------- Functions ----------------*/

fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(payload_buffer);
    FPFW_UNUSED(buffer_size);

    return mock_type(fpfw_status_t);
}

void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);
    return &dummy_icc_ctx;
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

TEST_FUNCTION(test_ib_telm_hsp_init_success_die_0, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_die_id, (KNG_DIE_ID)DIE_0);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    fpfw_init_result_t result = _fpfw_component_ib_telm_hsp.init_fn();
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
    assert_int_equal(result.associated_handle, NULL);
}

TEST_FUNCTION(test_ib_telm_hsp_init_success_die_1, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_die_id, (KNG_DIE_ID)DIE_1);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    fpfw_init_result_t result = _fpfw_component_ib_telm_hsp.init_fn();
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
    assert_int_equal(result.associated_handle, NULL);
}

TEST_FUNCTION(test_ib_telm_hsp_init_fail, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_die_id, (KNG_DIE_ID)DIE_0);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_FAIL);

    fpfw_init_result_t result = _fpfw_component_ib_telm_hsp.init_fn();
    assert_int_equal(result.status, (uint32_t)FPFW_STATUS_FAIL);
    assert_int_equal(result.associated_handle, NULL);
}
}
