set(TARGET kng.service.pwr_tlm_mcp.cmpnt.data_proc.functional.test)

set(SOURCES
    test_tile_temperature.cpp
    test_tile_voltage.cpp
    test_log_soc_dimm_info.cpp
    telemetry_functional_mocks.c
    test_core_current.cpp
    test_soc_rails_vr_current_and_voltage.cpp
    test_soc_rails_vr_temperature.cpp
    test_soc_pvt_temperature.cpp
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
    ms::lib::power_fuse
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
    sensor_fifo_svc_poll_tile_temperature
    sensor_fifo_svc_poll_tile_voltage
    sensor_fifo_svc_poll_core_current
    sensor_fifo_svc_poll_core_pstate
    sensor_fifo_svc_poll_vr_temperature
    sensor_fifo_svc_poll_vr_current
    sensor_fifo_svc_poll_soc_pvt_temperature
    sensor_fifo_svc_poll_dimm_info
    exec_tlm_cmpnt_get_timestamp_microseconds
    gtimer_prodfw_get_frequency
)