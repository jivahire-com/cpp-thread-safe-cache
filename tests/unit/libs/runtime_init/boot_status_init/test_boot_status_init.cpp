//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_boot_status_init.cpp
 * boot status init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <boot_status.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <idsw_kng.h> // for KNG_CPU_TYPE

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_boot_stat;
/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_boot_status_init(boot_status_icc_ctx_t* boot_status_ctx)
{
    assert_non_null(boot_status_ctx);
    assert_non_null(boot_status_ctx->hsp_mbx_ctx);
    assert_non_null(boot_status_ctx->accel_icc_ctx[0].lfifo_mbx_ctx);
    assert_non_null(boot_status_ctx->accel_icc_ctx[0].send_params);
    assert_non_null(boot_status_ctx->accel_icc_ctx[0].recv_params);
    assert_non_null(boot_status_ctx->accel_icc_ctx[1].lfifo_mbx_ctx);
    assert_non_null(boot_status_ctx->accel_icc_ctx[1].send_params);
    assert_non_null(boot_status_ctx->accel_icc_ctx[1].recv_params);
    function_called();
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    function_called();
    return mock_type(void*);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    function_called();
    return mock_type(idsw_cpu_type_t);
}

//
// Tests
//
TEST_FUNCTION(test_boot_status_init, nullptr, nullptr)
{
    uint32_t dummy = 0;
    fpfw_icc_base_ctx_t* dummy_icc_ctx = (fpfw_icc_base_ctx_t*)&dummy;

    // Set up expectations
    will_return(__wrap_fpfw_init_get_handle, dummy_icc_ctx);
    expect_function_call(__wrap_fpfw_init_get_handle);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    expect_function_call(__wrap_idsw_get_cpu_type);

    // Additional expectations for SCP
    will_return_count(__wrap_fpfw_init_get_handle, dummy_icc_ctx, 2);
    expect_function_calls(__wrap_fpfw_init_get_handle, 2);
    expect_function_call(__wrap_boot_status_init);

    // Call API under test
    _fpfw_component_boot_stat.init_fn();
}
}
