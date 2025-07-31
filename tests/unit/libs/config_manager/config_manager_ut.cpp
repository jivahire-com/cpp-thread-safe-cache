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
#include <idsw_kng.h>
#include <kng_error.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <silibs_common.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern knob_data_t g_knob_data[];
static uint8_t rmss_shared_ram_region[2048] = {0};
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

void __real_write_override_knob_to_shared_rmss(void* rmss_base_addr, size_t rmss_base_addr_size);
void __real_apply_override_knob_from_primary_die(uint32_t rmss_base_addr);
bool __real_update_knob_in_cached_db_cb(const fpfw_cfg_mgr_guid_t* knob_namespace,
                                        const char* knob_name,
                                        const uint8_t* data,
                                        size_t data_size,
                                        void* ctx);

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

    if (mock_type(int32_t) == KNG_SUCCESS)
    {
        if (req_params->data_size == sizeof(int))
        {
            memcpy(req_params->data, &test_data, sizeof(int));
        }
    }

    return mock_type(int32_t);
}

int32_t __wrap_variable_service_async_set_variable(var_service_req_ctx_t* var_serv_ctx,
                                                   var_service_req_params_t* req_params,
                                                   variable_service_req_complete_notify callback,
                                                   void* context)
{
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(req_params);
    FPFW_UNUSED(callback);
    FPFW_UNUSED(context);
    function_called();

    var_serv_ctx->async_req_result = 0;

    callback(context, var_serv_ctx, NULL, 0);
    return 0;
}

int32_t __wrap_variable_service_async_get_variable(var_service_req_ctx_t* var_serv_ctx,
                                                   var_service_req_params_t* req_params,
                                                   variable_service_req_complete_notify callback,
                                                   void* context)
{
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(req_params);
    FPFW_UNUSED(callback);
    FPFW_UNUSED(context);
    function_called();

    cached_knob_data_t* current_entry = (cached_knob_data_t*)context;

    if (current_entry->index == 0)
    {
        callback(context, var_serv_ctx, (uint8_t*)current_entry->data, current_entry->size);
    }
    else
    {
        callback(context, var_serv_ctx, (uint8_t*)current_entry->data, current_entry->size - 1);
    }

    return 0;
}

bool __wrap_update_knob_in_cached_db_cb(const fpfw_cfg_mgr_guid_t* knob_namespace,
                                        const char* knob_name,
                                        const uint8_t* data,
                                        size_t data_size,
                                        void* ctx)
{
    function_called();
    FPFW_UNUSED(knob_namespace);
    FPFW_UNUSED(knob_name);
    FPFW_UNUSED(data_size);
    FPFW_UNUSED(ctx);

    assert_true(test_data == *((uint32_t*)data));
    __real_update_knob_in_cached_db_cb(knob_namespace, knob_name, data, data_size, ctx);

    return true;
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type()
{
    return mock_type(idsw_cpu_type_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

bool __wrap_idhw_is_single_die_boot_en()
{
    return mock_type(bool);
}

void __wrap_apply_override_knob_from_primary_die(uint32_t rmss_base_addr)
{
    FPFW_UNUSED(rmss_base_addr);

    __real_apply_override_knob_from_primary_die((uint32_t)rmss_shared_ram_region);
}

void __wrap_write_override_knob_to_shared_rmss(void* rmss_base_addr, size_t rmss_base_addr_size)
{
    FPFW_UNUSED(rmss_base_addr);
    FPFW_UNUSED(rmss_base_addr_size);
    __real_write_override_knob_to_shared_rmss((void*)rmss_shared_ram_region, sizeof(rmss_shared_ram_region));
}

int __wrap_mscp_exp_spi_synchronize_dies(mscp_exp_spi_sync_point_t sync_point, int die_id)
{
    FPFW_UNUSED(sync_point);
    FPFW_UNUSED(die_id);
    return 0;
}

int __wrap_spi_controller_read_direct_instruction(uintptr_t spi_master_reg, uint32_t ahbAddr32, uint32_t actualWaitCycles, uint32_t* readData)
{
    FPFW_UNUSED(spi_master_reg);
    FPFW_UNUSED(ahbAddr32);
    FPFW_UNUSED(actualWaitCycles);
    FPFW_UNUSED(readData);

    memcpy(readData, (void*)ahbAddr32, sizeof(uint32_t));
    return 0;
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

static int rmss_memory_map_setup(void** state)
{
    FPFW_UNUSED(state);

    //
    // Reset global knob data that a previous test case may have overriden
    //
    for (uint32_t idx = 0; idx < KNOB_MAX; idx++)
    {
        g_knob_data[idx].statistics = (knob_statistics_t){0, 0};
        memcpy(g_knob_data[idx].cache_value_address, g_knob_data[idx].default_value_address, g_knob_data[idx].value_size);
    }

    knob_payload_header_t header = {.signature = CFG_MGR_RELAY_PAYLOAD_SIGNATURE, .total_override_knob_count = 0};
    memset(rmss_shared_ram_region, 0, sizeof(rmss_shared_ram_region));
    memcpy(rmss_shared_ram_region, &header, sizeof(header));

    return 0;
}

size_t __wrap_fpfw_cfg_mgr_get_cached_knob_values_size()
{
    return mock_type(size_t);
}

fpfw_status_t __wrap_fpfw_cfg_mgr_get_cached_knob_values(void* dest_addr, size_t dest_size)
{
    check_expected(dest_addr);
    check_expected(dest_size);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_cfg_mgr_set_cached_knob_values(void* src_addr, size_t src_size)
{
    check_expected(src_addr);
    check_expected(src_size);
    return mock_type(fpfw_status_t);
}

void update_knob_cb(cached_knob_data_t* requested_knob, uint8_t* updated_data, size_t data_size)
{
    FPFW_UNUSED(requested_knob);
    FPFW_UNUSED(updated_data);
    FPFW_UNUSED(data_size);
}

bool __wrap_system_info_get_mission_mode()
{
    return mock_type(bool);
}
}

//
// Tests
//
TEST_FUNCTION(test_cfg_mgr_init_no_hsp, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_system_info_is_hsp_present, false);

    cfg_mgr_init(&config_manager_setting, NULL);

    assert_true(cached_knob_data_size() == KNOB_MAX);
    assert_true(get_cached_knob_data() != NULL);
}

TEST_FUNCTION(test_cfg_mgr_init_mcp, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values_size, KNOB_MAX);
    will_return(__wrap_fpfw_cfg_mgr_set_cached_knob_values, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_cfg_mgr_set_cached_knob_values, src_addr, SCP_EXP_CONFIG_KNOB_CACHE_BASE);
    expect_value(__wrap_fpfw_cfg_mgr_set_cached_knob_values, src_size, KNOB_MAX);

    cfg_mgr_init(&config_manager_setting, &shared_mem);
}

TEST_FUNCTION(test_cfg_mgr_init_no_override, nullptr, nullptr)
{
    hsp_variable_svc_invoke_count = 0;

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);

    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values_size, KNOB_MAX);
    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_addr, SCP_EXP_CONFIG_KNOB_CACHE_BASE);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_size, KNOB_MAX);

    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, dsize, shared_mem.max_payload_size, -1);

    cfg_mgr_init(&config_manager_setting, &shared_mem);

    assert_true(cached_knob_data_size() == KNOB_MAX);
    assert_true(get_cached_knob_data() != NULL);
    assert_true(hsp_variable_svc_invoke_count == cached_knob_data_size());

    for (uint32_t idx = 0; idx < cached_knob_data_size(); idx++)
    {
        assert_true(get_cached_knob_data()[idx].overridden != true);
    }
}

TEST_FUNCTION(test_cfg_mgr_init_override_die0, rmss_memory_map_setup, nullptr)
{
    hsp_variable_svc_invoke_count = 0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_variable_service_sync_get_variable, KNG_SUCCESS);

    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values_size, KNOB_MAX);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_addr, SCP_EXP_CONFIG_KNOB_CACHE_BASE);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_size, KNOB_MAX);

    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, dsize, shared_mem.max_payload_size, -1);

    cfg_mgr_init(&config_manager_setting, &shared_mem);
}

TEST_FUNCTION(test_cfg_mgr_init_override_die1, rmss_memory_map_setup, nullptr)
{
    hsp_variable_svc_invoke_count = 0;

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);

    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values_size, KNOB_MAX);
    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_addr, SCP_EXP_CONFIG_KNOB_CACHE_BASE);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_size, KNOB_MAX);

    cfg_mgr_init(&config_manager_setting, &shared_mem);
}

TEST_FUNCTION(test_update_knob_in_cached_db_cb, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, dsize, shared_mem.max_payload_size, -1);

    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);
    expect_function_call(__wrap_update_knob_in_cached_db_cb);

    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values_size, KNOB_MAX);
    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_addr, SCP_EXP_CONFIG_KNOB_CACHE_BASE);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_size, KNOB_MAX);

    cfg_mgr_init(&config_manager_setting, &shared_mem);

    config_set_k_uint32_t(test_data);
}

TEST_FUNCTION(test_update_knob_data, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, dsize, shared_mem.max_payload_size, -1);

    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);
    expect_function_call(__wrap_variable_service_async_set_variable);
    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values_size, KNOB_MAX);
    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_addr, SCP_EXP_CONFIG_KNOB_CACHE_BASE);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_size, KNOB_MAX);

    cfg_mgr_init(&config_manager_setting, &shared_mem);

    update_knob_data(&get_cached_knob_data()[0],
                     (uint8_t*)get_cached_knob_data()[1].data,
                     get_cached_knob_data()[0].size,
                     update_knob_cb,
                     true);
}

TEST_FUNCTION(test_read_fails_on_max_knob, rmss_memory_map_setup, nullptr)
{
    //
    // Initialize the config manager, which will increment the knob cindex used
    // to read the knob from the default database.
    //
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_system_info_is_hsp_present, false);

    cfg_mgr_init(&config_manager_setting, NULL);

    assert_true(cached_knob_data_size() == KNOB_MAX);
    assert_true(get_cached_knob_data() != NULL);

    //
    // Issue another read from the default db, which should fail as this is
    // only done once on init.
    //
    fpfw_status_t status = read_knob_from_default_db_cb(NULL, NULL, NULL, 0, NULL);
    assert_int_equal(status, FPFW_STATUS_NOT_FOUND);
}

TEST_FUNCTION(test_check_var_store_knob_data_async, rmss_memory_map_setup, nullptr)
{
    expect_function_call_any(SCB_CleanInvalidateDCache_by_Addr);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, addr, (void*)ALIGN_DOWN(shared_mem.payload_base, 32), -1);
    expect_value_count(SCB_CleanInvalidateDCache_by_Addr, dsize, shared_mem.max_payload_size, -1);

    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);
    expect_function_call_any(__wrap_variable_service_async_get_variable);

    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values_size, KNOB_MAX);
    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_addr, SCP_EXP_CONFIG_KNOB_CACHE_BASE);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_size, KNOB_MAX);

    cfg_mgr_init(&config_manager_setting, &shared_mem);

    check_var_store_knob_data_async(&get_cached_knob_data()[0]);
    check_var_store_knob_data_async(&get_cached_knob_data()[1]);
}

TEST_FUNCTION(test_check_var_store_mission_mode, rmss_memory_map_setup, nullptr)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_system_info_get_mission_mode, true);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idhw_is_single_die_boot_en, true);

    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values_size, KNOB_MAX);
    will_return(__wrap_fpfw_cfg_mgr_get_cached_knob_values, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_addr, SCP_EXP_CONFIG_KNOB_CACHE_BASE);
    expect_value(__wrap_fpfw_cfg_mgr_get_cached_knob_values, dest_size, KNOB_MAX);

    cfg_mgr_init(&config_manager_setting, &shared_mem);
}