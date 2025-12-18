set(TARGET kng.svc.pwr_tlm_mcp.dp.func.test)

set(SOURCES
    test_tile_temperature.cpp
    test_tile_voltage.cpp
    test_log_soc_dimm_info.cpp
    telemetry_functional_mocks.c
    test_core_current.cpp
    test_soc_rails_vr_current_and_voltage.cpp
    test_soc_rails_vr_temperature.cpp
    test_soc_pvt_temperature.cpp
    test_core_pstate_data.cpp
    test_cstate_data.cpp
    log_DIMM_power_integration_test.cpp
    test_core_power.cpp
    test_soc_hnf_temperature.cpp
    test_droop_count.cpp
    test_core_throttling_data.cpp
    log_core_instantaneous_data_integration_test.cpp
    test_log_dimm_instantaneous_telemetry.cpp
    test_soc_max_temperature.cpp
    test_log_inst_die_max_temp.cpp
    test_core_vm_throttling_priorities.cpp
    test_vm_core_power_integration.cpp
    test_inst_die_temp_sensors_integration.cpp
    test_core_aging_counter_data.cpp
    log_vm_throttling_integration_test.cpp
    log_memory_throttling_integration_test.cpp
    test_core_histogram_integration.cpp
    test_inst_soc_voltage_rails.cpp
    )

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    kng.service.pwr_tlm_mcp.cmpnt.data_process
    kng.service.pwr_tlm_mcp
    kng.service.pwr_tlm_mcp.cmpnt.in_band_tlm
    ms::lib::event_trace::trace
    fwlib.lib.status
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::lib::telemetry::fuses
    ms::service::sensor_fifo
    ms::lib::interrupts
    ms::lib::atomics
)

# platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_include_directories(${TARGET}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/libs/event_trace/trace/inc
    $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp.cmpnt.data_process,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp.cmpnt.in_band_tlm,INCLUDE_DIRECTORIES>
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock functions - only mock hardware/platform interfaces
mock_target(${TARGET}
    # Sensor mocks (hardware interface)
    sensor_fifo_svc_is_empty
    sensor_fifo_svc_poll_tile_temperature
    sensor_fifo_svc_poll_tile_voltage
    sensor_fifo_svc_poll_core_current
    sensor_fifo_svc_poll_core_pstate
    sensor_fifo_svc_poll_vr_temperature
    sensor_fifo_svc_poll_vr_current
    sensor_fifo_svc_poll_soc_pvt_temperature
    sensor_fifo_svc_poll_dimm_info
    pwr_tlm_core_exch_mcp_read_droop_counts
    exec_tlm_cmpnt_get_timestamp_microseconds
    gtimer_prodfw_get_frequency
    # Aging counter mocks (DVFS register-level hardware interface)
    dvfs_c2_pcm_aging_get_sensor_status
    dvfs_c2_pcm_enable_aging_sensor_measurement
    dvfs_c2_get_pcm_bank_sensor_data
    # Multi-die communication mocks
    die_2_die_exch_get_this_die_id
    in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg
    in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg
    # Semaphore mocks
    initialize_semaphore
    read_semaphore
    wait_for_semaphore
    release_semaphore
    die_2_die_exch_ib_write_inst_max_die_temp
    die_2_die_exch_ib_read_inst_max_die_temp_dC
    die_2_die_exch_get_this_die_id
    )