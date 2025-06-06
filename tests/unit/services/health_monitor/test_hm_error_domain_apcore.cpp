//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_error_domain_apcore.cpp
 * Tests health monitor error injection
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <hm_test.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
extern acpi_error_domain_t test_error_domain;
extern char* TEST_ERROR_DOMAIN_NAME;
extern ras_einj_info_t einj_payload_local;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// Mocks
//
}

//
// Tests
//
TEST_FUNCTION(hm_apcore_error_injection_listener, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);

    hm_register_error_domain((uint16_t)test_error_domain, NULL, TEST_ERROR_DOMAIN_NAME, hm_error_injection_cb, nullptr);

    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(hm_error_injection_cb);
    will_return_always(__wrap_idsw_get_die_id, 0);
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);

    hm_apcore_error_injection_listener((fpfw_icc_base_ctx_t*)ICC_HM_ERROR_INJECTION_AP2SCP);
}