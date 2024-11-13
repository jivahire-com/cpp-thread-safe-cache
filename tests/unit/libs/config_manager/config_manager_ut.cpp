//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_sds_init.cpp
 * Tests the init of the sds service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <FpFwUtils.h>
#include <config_manager.h>
#include <config_manager_i.h>
#include <kng_error.h>
#include <mscp_exp_rmss_memory_map.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t rmss_shared_ram_region[128] = {0};
static uint32_t test_data = 1234;

fpfw_cfg_mgr_config_t config_manager_setting = {.mission_mode = false,
                                                .profile_id = 0,
                                                .write_knob_fn = update_knob_in_cached_db_cb,
                                                .read_knob_fn = read_knob_from_default_db_cb,
                                                .db_ctx = (void*)1};

var_service_shared_mem_t shared_mem = {
    .payload_base = (uintptr_t)rmss_shared_ram_region,
    .max_payload_size = sizeof(rmss_shared_ram_region),
};

static uint32_t hsp_variable_svc_invoke_count = 0;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_FPFwSpinLockInitialize(size_t* pLock)
{
    FPFW_UNUSED(pLock);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

int32_t __wrap_variable_service_initialize_ctx(var_service_req_ctx_t* var_serv_ctx, var_service_shared_mem_t* mem_ctx)
{
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(mem_ctx);
    return 0;
}

void __wrap_variable_service_unlock_get_var_ctx(var_service_req_ctx_t* var_serv_ctx)
{
    FPFW_UNUSED(var_serv_ctx);
}

int32_t __wrap_variable_service_sync_get_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(req_params);
    hsp_variable_svc_invoke_count++;

    return mock_type(int32_t);
}

void __wrap_variable_service_async_set_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(req_params);
    function_called();
}

bool __wrap_update_knob_in_cached_db_cb(const fpfw_cfg_mgr_guid_t* knob_namespace,
                                        const char* knob_name,
                                        const uint8_t* data,
                                        size_t data_size,
                                        void* ctx)
{
    FPFW_UNUSED(knob_namespace);
    FPFW_UNUSED(knob_name);
    FPFW_UNUSED(data_size);
    FPFW_UNUSED(ctx);

    assert_true(test_data == *((uint32_t*)data));
    return true;
}
}

//
// Tests
//
TEST_FUNCTION(test_cfg_mgr_init_no_hsp, nullptr, nullptr)
{
    will_return(__wrap_system_info_is_hsp_present, false);

    cfg_mgr_init(&config_manager_setting, NULL);

    assert_true(cached_knob_data_size() == KNOB_MAX);
    assert_true(get_cached_knob_data() != NULL);
}

TEST_FUNCTION(test_cfg_mgr_init_hsp_override, nullptr, nullptr)
{
    hsp_variable_svc_invoke_count = 0;
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_variable_service_sync_get_variable, KNG_SUCCESS);

    cfg_mgr_init(&config_manager_setting, &shared_mem);

    assert_true(cached_knob_data_size() == KNOB_MAX);
    assert_true(get_cached_knob_data() != NULL);
    assert_true(hsp_variable_svc_invoke_count == cached_knob_data_size());

    for (uint32_t idx = 0; idx < cached_knob_data_size(); idx++)
    {
        assert_true(get_cached_knob_data()[idx].overridden == true);
    }
}

TEST_FUNCTION(test_cfg_mgr_init_hsp_no_override, nullptr, nullptr)
{
    hsp_variable_svc_invoke_count = 0;
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);

    cfg_mgr_init(&config_manager_setting, &shared_mem);

    assert_true(cached_knob_data_size() == KNOB_MAX);
    assert_true(get_cached_knob_data() != NULL);
    assert_true(hsp_variable_svc_invoke_count == cached_knob_data_size());

    for (uint32_t idx = 0; idx < cached_knob_data_size(); idx++)
    {
        assert_true(get_cached_knob_data()[idx].overridden != true);
    }
}

TEST_FUNCTION(test_update_knob_in_cached_db_cb, nullptr, nullptr)
{
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);

    cfg_mgr_init(&config_manager_setting, &shared_mem);

    config_set_k_uint32_t(test_data);
}

TEST_FUNCTION(test_update_knob_data, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);
    expect_function_call(__wrap_variable_service_async_set_variable);

    cfg_mgr_init(&config_manager_setting, &shared_mem);

    update_knob_data(&get_cached_knob_data()[0],
                     (uint8_t*)get_cached_knob_data()[0].data,
                     get_cached_knob_data()[0].size,
                     true);
}