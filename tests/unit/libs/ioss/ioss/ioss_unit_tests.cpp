//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ioss_unit_tests.cpp
 * IOSS tests
 */

/*------------- Includes -----------------*/
#include "silibs_common.h"
#include "silibs_platform.h"

#include <CMockaWrapper.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <cstdint>
#include <ioss_top_regs.h> // for IOSS_TOP_IOSS_PCR_ADDRESS, IOSS_...
#include <kng_error.h>
#include <kng_soc_constants.h> // for ATU_PAGE_SIZE, NUM_IOSS_INSTANCES
#include <mscp_exp_rmss_memory_map.h>
#include <silibs_ap_top_regs.h>     // for AP_TOP_D0_VAB_CDED_IOSS_ADDRESS
#include <stddef.h>                 // for NULL
#include <vab_cded_ioss_top_regs.h> // for VAB_CDED_IOSS_TOP_IOSS_ADDRESS
#include <variable_services.h>

extern "C" {
#include <error_handler.h>
#include <ioss_ini.h> // for ioss_ini
}
/*-- Symbolic Constant Macros (defines) --*/
#define BUGCHECK_MOCK_RETURN   (setjmp(mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static var_service_req_ctx_t req_ctx = {};
static var_service_shared_mem_t mem_ctx = {0};
static jmp_buf mock_jump_buf;
static bool should_return;

/*------------- Functions ----------------*/
extern "C" {

void __wrap_crash_dump_bug_check(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(p0);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    function_called();

    /* Handle noreturn, allowing control to return to test */
    if (!should_return)
    {
        longjmp(mock_jump_buf, 1);
    }
}
}
//
// Tests
//
TEST_FUNCTION(test_valid_die_0, NULL, NULL)
{
    mem_ctx.payload_base = (uintptr_t)SCP_EXP_SCP_USB_VARIABLE_SERVICE_PAYLOAD_BASE;
    mem_ctx.max_payload_size = SCP_EXP_SCP_USB_VARIABLE_SERVICE_PAYLOAD_SIZE;
    expect_memory(__wrap_variable_service_initialize_ctx, var_serv_ctx, &req_ctx, sizeof(req_ctx));
    expect_memory(__wrap_variable_service_initialize_ctx, mem_ctx, &mem_ctx, sizeof(mem_ctx));
    will_return(__wrap_variable_service_initialize_ctx, KNG_SUCCESS);

    expect_any(__wrap_variable_service_sync_set_variable, var_serv_ctx);
    expect_any(__wrap_variable_service_sync_set_variable, req_params);
    expect_function_call(__wrap_variable_service_sync_set_variable);

    will_return(__wrap_ioss_init, SILIBS_SUCCESS);
    ioss_ini();
}

TEST_FUNCTION(test_variable_service_initialize_ctx, NULL, NULL)
{
    mem_ctx.payload_base = (uintptr_t)SCP_EXP_SCP_USB_VARIABLE_SERVICE_PAYLOAD_BASE;
    mem_ctx.max_payload_size = SCP_EXP_SCP_USB_VARIABLE_SERVICE_PAYLOAD_SIZE;
    expect_memory(__wrap_variable_service_initialize_ctx, var_serv_ctx, &req_ctx, sizeof(req_ctx));
    expect_memory(__wrap_variable_service_initialize_ctx, mem_ctx, &mem_ctx, sizeof(mem_ctx));
    will_return(__wrap_variable_service_initialize_ctx, 1);
    expect_function_call(__wrap_crash_dump_bug_check);
    if (!bugcheck_mock_return())
    {
        ioss_ini();
    }
}

TEST_FUNCTION(test_ioss_init_failed, NULL, NULL)
{
    mem_ctx.payload_base = (uintptr_t)SCP_EXP_SCP_USB_VARIABLE_SERVICE_PAYLOAD_BASE;
    mem_ctx.max_payload_size = SCP_EXP_SCP_USB_VARIABLE_SERVICE_PAYLOAD_SIZE;
    expect_memory(__wrap_variable_service_initialize_ctx, var_serv_ctx, &req_ctx, sizeof(req_ctx));
    expect_memory(__wrap_variable_service_initialize_ctx, mem_ctx, &mem_ctx, sizeof(mem_ctx));
    will_return(__wrap_variable_service_initialize_ctx, KNG_SUCCESS);

    expect_any(__wrap_variable_service_sync_set_variable, var_serv_ctx);
    expect_any(__wrap_variable_service_sync_set_variable, req_params);
    expect_function_call(__wrap_variable_service_sync_set_variable);

    will_return(__wrap_ioss_init, SILIBS_E_INIT);
    expect_function_call(__wrap_crash_dump_bug_check);
    should_return = false;
    if (!bugcheck_mock_return())
    {
        ioss_ini();
    }
}
