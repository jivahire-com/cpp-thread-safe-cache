//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ioss_mocks.c
 * IOSS mock functions
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <atu_lib.h>    // for atu_id_t, atu_map_entry_t
#include <cmocka.h>     // for mock_type
#include <ioss_init.h>
#include <stdint.h> // for uint64_t, uintptr_t, uint32_t
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//

int __wrap_variable_service_initialize_ctx(var_service_req_ctx_t* var_serv_ctx, var_service_shared_mem_t* mem_ctx)
{
    check_expected(var_serv_ctx);
    check_expected(mem_ctx);
    return mock_type(int);
}

void __wrap_variable_service_sync_set_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    check_expected(var_serv_ctx);
    check_expected(req_params);
    function_called();
}

int __wrap_ioss_init(IOSS_INSTANCE ioss_id, ioss_init_t* init)
{
    FPFW_UNUSED(ioss_id);
    FPFW_UNUSED(init);
    return mock_type(int);
}
