set(TARGET power.test.main)

# Compile test
add_executable(${TARGET}
    power_cap_test.cpp
    power_distribution_test.cpp
    power_fuses_test.cpp
    power_knobs_test.cpp
    power_main_test.cpp
    power_pid_test.cpp
    power_vcpu_cal_test.cpp
    power_log_test.cpp
    ../mocks/mock_bug_check.c
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::scp_avs
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::power
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.linkedlist
        fwlib.lib.utils
 )

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::service::power,INCLUDE_DIRECTORIES>
        ../
        ../mocks/

)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    FpFwAssert
    
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkInterfaceInitialize
    platform_read_fuse
    power_hw_full_init_allowed
    power_init_core
    power_init_soc
    power_fuses_is_power_hw_supported
    power_loops_init
    power_loops_control_init
    power_loops_control_post_core_init
    power_loops_telemetry_init
    power_runconfig_get  
    power_runconfig_init
    power_telemetry_init_config
    power_telemetry_enable
    power_timer_start_loop_timers
    power_timer_get_counter
    crash_dump_bug_check
    ws_data_get
    ws_data_put
    reset_tile_pvt_dts_vm
    tile_pvt_sda_reconfig
    odcm_telemetry_config
    dvfs_telemetry_config
    dvfs_set_plimit
    tile_pvt_dma_config
    crash_dump_register_address32
    crash_dump_register_pre_dump_callback
    power_hw_clear_force_pmin
    config_get_power_enable_survivability_mode
    mmio_write64
    mmio_write32
    mmio_write16
    mmio_write8
    atu_map
    atu_unmap
    idsw_get_die_id
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)