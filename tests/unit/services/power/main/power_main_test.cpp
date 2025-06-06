//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_main_test.cpp
 * Power service main tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t
#include <silibs_common.h>

extern "C" {

#include <CMockaWrapper.h>    // for expect_value, check_expected_ptr, Cmo...
#include <DfwkAsyncRequest.h> // for PDFWK_ASYNC_REQUEST_HEADER
#include <DfwkCommon.h>       // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUE...
#include <modules/CdDumpDescriptor.h>
#include <odcm_struct.h> // for odcm_telem_config_t
#include <power_dfwk.h>  // for power_service_t, power_service_interf...
#include <power_hw_int_i.h>
#include <power_hw_int_i.h>       // for power_telcfg_t
#include <power_i.h>              // for power_init, power_interface_init
#include <power_init.h>           // for power_init, power_interface_init
#include <power_runconfig.h>      // for power_service_config_t
#include <power_runconfig_i.h>    // for power_runconfig_t
#include <power_warmstart_i.h>    // for power_ws_fuse_t
#include <startup_shutdown_ssi.h> // for pssi_startup_notification_request_t
#include <warm_start_id.h>        // for mod_ws_data_id_t

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void __real_power_runconfig_init(const power_service_config_t* p_config);
/*-- Declarations (Statics and globals) --*/

DFWK_ASYNC_REQUEST_DISPATCH s_dispatch_routine = NULL;
DFWK_REQUEST_DISPATCH_SYNC s_dispatch_routine_sync = NULL;
const power_telcfg_t* sp_telemetry_config;
dvfs_config_t dvfs_cfg = DVFS_DEFAULT_CONFIG;

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {

void __wrap_power_hw_clear_force_pmin(power_pmin_type_t type)
{
    function_called();
    check_expected(type);
}

// wrapper function that checks expected values for the incoming parameters
void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{

    check_expected_ptr(Interface);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchQueue);
    check_expected_ptr(DispatchSync);
    // save the dispatch routine for later use
    s_dispatch_routine_sync = DispatchSync;
}

// wrapper function that checks expected values for the two incoming parameters
void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    check_expected_ptr(Device);
    check_expected_ptr(Schedule);
}

// wrapper function that checks expected values for the incoming parameters
void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    check_expected_ptr(Queue);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchRoutine);
    check_expected_ptr(DispatchContext);
    check_expected(QueueType);
    // save the dispatch routine for later use
    s_dispatch_routine = DispatchRoutine;
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

bool __wrap_power_hw_full_init_allowed()
{
    return mock_type(bool);
}

// wrap for power_runconfig_init
void __wrap_power_runconfig_init(const power_service_config_t* p_config)
{
    check_expected_ptr(p_config);
}

void __wrap_power_loops_init()
{
    // nothing to do
    function_called();
}

void __wrap_power_timer_start_loop_timers()
{
    // nothing to do
    function_called();
}

void __wrap_power_loops_control_post_core_init()
{
    // nothing to do
    function_called();
}

void __wrap_power_telemetry_enable()
{
    // nothing to do
    function_called();
}

// wrap for power_telemetry_init_config
void __wrap_power_telemetry_init_config(const power_telcfg_t* p_telemetry_config)
{
    assert_non_null(p_telemetry_config);
    sp_telemetry_config = p_telemetry_config;
}

// wrap for power_init_soc
void __wrap_power_init_soc(const power_runconfig_t* p_runconfig)
{
    check_expected_ptr(p_runconfig);
}

void __wrap_power_vcpu_precalculate_vf_currents(power_runconfig_t* p_runconfig, dvfs_config_t* p_dvfs_cfg)
{
    check_expected_ptr(p_runconfig);
    assert_int_equal((uintptr_t)p_dvfs_cfg, (uintptr_t)&dvfs_cfg);
}

// wrap for power_init_core
void __wrap_power_init_core(const power_runconfig_t* p_runconfig, const power_telcfg_t* p_telemetry_config)
{
    check_expected_ptr(p_runconfig);
    assert_int_equal((uintptr_t)p_telemetry_config, (uintptr_t)sp_telemetry_config);
}

void* __wrap_ws_data_get(mod_ws_data_id_t id, uint32_t* p_size)
{
    check_expected(id);
    *p_size = mock_type(uint32_t);

    function_called();

    return mock_ptr_type(void*);
}

void* __wrap_ws_data_put(mod_ws_data_id_t id, void* p_data, uint32_t size)
{
    check_expected(id);
    check_expected_ptr(p_data);
    check_expected(size);

    function_called();

    return mock_ptr_type(void*);
}

void __wrap_reset_tile_pvt_dts_vm(uintptr_t cluster_pex_base_addr)
{
    FPFW_UNUSED(cluster_pex_base_addr);

    function_called();
}

int __wrap_tile_pvt_sda_reconfig(uintptr_t cluster_pex_base_addr, bool fw_sda_ip_control)
{
    FPFW_UNUSED(cluster_pex_base_addr);
    FPFW_UNUSED(fw_sda_ip_control);

    function_called();

    return 0;
}

void __wrap_odcm_telemetry_config(const uintptr_t cluster_pex_base_addr, const odcm_telem_config_t* telem_cfg)
{
    FPFW_UNUSED(cluster_pex_base_addr);
    FPFW_UNUSED(telem_cfg);

    function_called();
}

void __wrap_dvfs_telemetry_config(const uintptr_t cluster_pex_base_addr, const uint32_t pstate_telemetry_addr, const uint32_t scp_msg_addr)
{
    FPFW_UNUSED(cluster_pex_base_addr);
    FPFW_UNUSED(pstate_telemetry_addr);
    FPFW_UNUSED(scp_msg_addr);

    function_called();
}

void __wrap_dvfs_set_plimit(const uintptr_t cluster_pex_base_addr, uint8_t plimit_index, bool rack_power_cap)
{
    FPFW_UNUSED(cluster_pex_base_addr);
    FPFW_UNUSED(plimit_index);
    FPFW_UNUSED(rack_power_cap);

    function_called();
}

void __wrap_tile_pvt_dma_config(uintptr_t cluster_pex_base_addr, const tile_pvt_telem_setting_config_t* pvt_telem_settings)
{
    FPFW_UNUSED(cluster_pex_base_addr);
    FPFW_UNUSED(pvt_telem_settings);

    function_called();
}

// wrap for power_runconfig_get
power_runconfig_t* __wrap_power_runconfig_get()
{
    return mock_type(power_runconfig_t*);
}
// wrap for power_get_dvfs_config
dvfs_config_t* __wrap_power_get_dvfs_config()
{
    return mock_type(dvfs_config_t*);
}

void __wrap_power_loops_control_init()
{
    function_called();
}

void __wrap_power_loops_telemetry_init()
{
    function_called();
}

void __wrap_crash_dump_register_address32(void* address_exp, uint32_t size_exp, FPFwCdDumpPriority priority_exp)
{
    FPFW_UNUSED(address_exp);
    FPFW_UNUSED(size_exp);
    FPFW_UNUSED(priority_exp);
    function_called();
}

void __wrap_crash_dump_register_pre_dump_callback(void cb(void*), void* ctx)
{
    FPFW_UNUSED(ctx);
    FPFW_UNUSED(cb);
    function_called();
}
void __wrap_power_hw_force_pmin(power_pmin_type_t type)
{
    check_expected(type);
    function_called();
}

} // extern "C"

//
// Tests
//
POWER_TEST(init, NULL, NULL)
{
    power_service_t test_device;
    power_service_config_t test_config;

    DFWK_SCHEDULE test_schedule;

    expect_value(__wrap_DfwkDeviceInitialize, Device, &test_device.header);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &test_schedule);
    expect_value(__wrap_DfwkQueueInitialize, Queue, &test_device.default_queue);
    expect_value(__wrap_DfwkQueueInitialize, Device, &test_device.header);
    expect_any(__wrap_DfwkQueueInitialize, DispatchRoutine);
    expect_value(__wrap_DfwkQueueInitialize, DispatchContext, &test_device.header);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_SerializedDispatch);

    // add the expected/check values for power internal functions
    will_return(__wrap_power_hw_full_init_allowed, true);
    expect_value(__wrap_power_runconfig_init, p_config, &test_config);

    expect_function_call(__wrap_power_loops_init);
    expect_function_call(__wrap_power_loops_control_init);
    expect_function_call(__wrap_power_loops_telemetry_init);
    expect_function_call(__wrap_crash_dump_register_pre_dump_callback);
    power_init(&test_device, &test_schedule, &test_config);
}

POWER_TEST(init_ws, NULL, NULL)
{
    power_service_t test_device;
    power_service_config_t test_config;
    power_runconfig_t test_runconfig = {.fuses = {.ldodac_to_volt = {.slope_uvolt = 2000, .offset_uvolt = 2000}}};
    power_ws_fuse_t test_ws_stored = {.version = 1};

    DFWK_SCHEDULE test_schedule;

    expect_value(__wrap_DfwkDeviceInitialize, Device, &test_device.header);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &test_schedule);
    expect_value(__wrap_DfwkQueueInitialize, Queue, &test_device.default_queue);
    expect_value(__wrap_DfwkQueueInitialize, Device, &test_device.header);
    expect_any(__wrap_DfwkQueueInitialize, DispatchRoutine);
    expect_value(__wrap_DfwkQueueInitialize, DispatchContext, &test_device.header);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_SerializedDispatch);

    // add the expected/check values for power internal functions
    will_return(__wrap_power_hw_full_init_allowed, false);
    expect_value(__wrap_power_runconfig_init, p_config, &test_config);
    will_return(__wrap_power_runconfig_get, &test_runconfig);
    expect_value_count(__wrap_FpFwAssert, expression, true, 4); // power_ws_recover_fuse_init checks runconfig.
    expect_value(__wrap_ws_data_get, id, WARM_START_ID_POWER_FUSE);
    will_return(__wrap_ws_data_get, sizeof(test_ws_stored));
    will_return(__wrap_ws_data_get, &test_ws_stored);
    expect_function_call(__wrap_ws_data_get);
    expect_function_call(__wrap_power_loops_init);
    expect_function_call(__wrap_power_loops_control_init);
    expect_function_call(__wrap_power_loops_telemetry_init);
    expect_function_call(__wrap_crash_dump_register_pre_dump_callback);

    power_init(&test_device, &test_schedule, &test_config);
}

POWER_TEST(init_ap_soc, NULL, NULL)
{
    power_runconfig_t test_runconfig;
    power_ws_fuse_t test_ws_stored;

    will_return(__wrap_power_hw_full_init_allowed, true);
    will_return(__wrap_power_runconfig_get, &test_runconfig);
    will_return(__wrap_power_get_dvfs_config, &dvfs_cfg);
    expect_value(__wrap_power_init_soc, p_runconfig, &test_runconfig);
    expect_value(__wrap_power_init_core, p_runconfig, &test_runconfig);
    expect_value(__wrap_FpFwAssert, expression, true); // power_ws_save_fuse_init checks runconfig.
    expect_value(__wrap_FpFwAssert, expression, true); // power_ws_generate_fuse_data checks runconfig.
    expect_value(__wrap_FpFwAssert, expression, true); // power_ws_generate_fuse_data checks ws_fuse_data.
    expect_value(__wrap_ws_data_put, id, WARM_START_ID_POWER_FUSE);
    expect_any(__wrap_ws_data_put, p_data);
    expect_value(__wrap_ws_data_put, size, sizeof(test_ws_stored));
    will_return(__wrap_ws_data_put, &test_ws_stored);
    expect_function_call(__wrap_ws_data_put);
    expect_value(__wrap_power_vcpu_precalculate_vf_currents, p_runconfig, &test_runconfig);
    expect_function_call(__wrap_power_loops_control_post_core_init);
    power_ap_soc_init();
}

POWER_TEST(init_ap_soc_post_remote_sync, NULL, NULL)
{
    expect_function_call(__wrap_power_telemetry_enable);
    expect_function_call(__wrap_power_timer_start_loop_timers);
    expect_function_call(__wrap_power_hw_clear_force_pmin);
    expect_value(__wrap_power_hw_clear_force_pmin, type, PM_PMIN_ALL);

    power_ap_soc_init_post_remote_sync();
}

POWER_TEST(init_ap_soc_ws, NULL, NULL)
{
    const corebits_t default_cores = (const corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFF, 0xFFFFFFFF, 0xF);
    power_service_config_t test_config = {
        .platform_cores_in_die = &default_cores,
        .platform_die_core_count = 1,
        .platform_core_power_support = true,
    };
    power_runconfig_t test_runconfig = {.p_sconfig = &test_config};

    will_return(__wrap_power_hw_full_init_allowed, false);
    will_return(__wrap_power_runconfig_get, &test_runconfig);
    will_return(__wrap_power_get_dvfs_config, &dvfs_cfg);
    expect_value_count(__wrap_FpFwAssert, expression, true, 7);
    expect_function_call(__wrap_reset_tile_pvt_dts_vm);
    expect_function_call(__wrap_tile_pvt_sda_reconfig);
    expect_function_call(__wrap_tile_pvt_dma_config);
    expect_value(__wrap_power_vcpu_precalculate_vf_currents, p_runconfig, &test_runconfig);
    expect_function_call(__wrap_power_loops_control_post_core_init);

    power_ap_soc_init();
}

POWER_TEST(interface_init, NULL, NULL)
{
    power_service_t test_device;
    power_service_interface_t test_interface;

    expect_value(__wrap_DfwkInterfaceInitialize, Interface, &test_interface.header);
    expect_value(__wrap_DfwkInterfaceInitialize, Device, &test_device.header);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchQueue, &test_device.default_queue);
    expect_any(__wrap_DfwkInterfaceInitialize, DispatchSync);

    power_interface_init(&test_device, &test_interface);
}

POWER_TEST(dispatch_sync_default, NULL, NULL)
{
    DFWK_SYNC_REQUEST_HEADER test_request;
    test_request.RequestType = -1; // invalid request type

    expect_value(__wrap_FpFwAssert, expression, false);

    if (s_dispatch_routine_sync)
    {
        s_dispatch_routine_sync(&test_request);
    }
}

POWER_TEST(dispatch_default, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER test_request;
    power_service_t test_device;
    test_request.RequestType = -1; // invalid request type

    expect_value(__wrap_FpFwAssert, expression, false);

    if (s_dispatch_routine)
    {
        s_dispatch_routine(&test_request, &test_device.header);
    }
}

POWER_TEST(power_service_dispatch_async_quiesce, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER mock_request;
    power_runconfig_t test_runconfig;
    mock_request.RequestType = SSI_QUIESCE_ASYNC;

    pssi_shutdown_notification_request_t ssi_request = (pssi_shutdown_notification_request_t)&mock_request;
    ssi_request->shutdown_type = MSCP_SUBSYS_RESET;

    will_return(__wrap_power_runconfig_get, &test_runconfig);
    expect_value(__wrap_power_hw_force_pmin, type, PM_FW_PMIN_CONTROL);
    expect_function_call(__wrap_power_hw_force_pmin);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &mock_request);

    power_service_dispatch_async(&mock_request, NULL);
    assert_true(test_runconfig.knobs.loops_disable == power_loops_disable_t_ALL);
}

POWER_TEST(power_service_dispatch_async_shutdown, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER mock_request;
    mock_request.RequestType = SSI_SHUTDOWN_ASYNC;

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &mock_request);

    power_service_dispatch_async(&mock_request, NULL);
}

POWER_TEST(power_service_dispatch_async_complete_async, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER mock_request;
    mock_request.RequestType = SSI_STARTUP_STAGE_COMPLETE_ASYNC;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &mock_request);

    power_service_dispatch_async(&mock_request, NULL);
}