# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET  kng.service.pwr_tlm_mcp.cmpnt.data_process.test)

set(SOURCES
    data_proc_mock.c
    test_compute_metrics.cpp
    test_data_proc_cmpnt.cpp
    test_data_sampling.cpp
    test_data_utilities.cpp
    test_die_2_die_exch.cpp
    test_oob_interface.cpp
    test_package_interface.cpp
    test_aging_counters.cpp
)

add_subdirectory(functional)
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    kng.service.pwr_tlm_mcp.cmpnt.data_process
    fwlib.lib.status
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::lib::telemetry::fuses
    ms::service::sensor_fifo
    silibs::dvfs
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp,INCLUDE_DIRECTORIES>)
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp.cmpnt.data_process,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    sensor_fifo_svc_is_empty
    sensor_fifo_svc_poll_tile_temperature
    sensor_fifo_svc_poll_tile_voltage
    sensor_fifo_svc_poll_core_current
    sensor_fifo_svc_poll_core_pstate
    sensor_fifo_svc_poll_vr_temperature
    sensor_fifo_svc_poll_vr_current
    sensor_fifo_svc_poll_soc_pvt_temperature
    sensor_fifo_svc_poll_dimm_info
    tlm_fuses_get_dts_coeff_tile
    exec_tlm_cmpnt_get_timestamp_microseconds
    gtimer_prodfw_get_frequency
    in_band_tlm_cmpnt_is_inst_record_enabled
    in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg
    in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg
    wait_for_semaphore
    release_semaphore
    pwr_tlm_core_exch_mcp_read_droop_counts
    core_info_get_enable_cores_result
    dvfs_c2_pcm_enable_aging_sensor_measurement
    dvfs_c2_pcm_aging_get_sensor_status
    dvfs_c2_get_pcm_bank_sensor_data
    mesh_clock_telemetry
    mesh_get_m1_entry_count
    mesh_get_m2_entry_count
    mesh_get_m0_residency
    mesh_get_m1_residency
    mesh_get_m2_residency
    mesh_get_telemetry_delivered_perf_count
    d2dss_pmu_init
    d2dss_pmu_read
    in_band_tlm_cmpnt_is_power_record_enabled
    power_telemetry_loadline_loss_die0
    power_telemetry_loadline_loss_die1
)