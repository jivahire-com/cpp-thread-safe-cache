set(TARGET service.telemetry.test)

set(SOURCES
    test_telemetry.cpp
    telemetry_mock.c
)

add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
    kng.service.telemetry
    fwlib.lib.status	
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::lib::arm_intrinsic
    ms::lib::register_headers
    ms::service::sensor_fifo	
    silibs::dvfs
    silibs::kng_silibs_platform
    silibs::sensor_ram_bridge
    silibs::fuse
    silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.telemetry,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    sensor_fifo_svc_poll_tile_temperature
    sensor_fifo_svc_poll_tile_voltage	
    sensor_fifo_svc_poll_core_current
    sensor_fifo_svc_poll_core_pstate
	sensor_fifo_svc_poll_vr_temperature
	sensor_fifo_svc_poll_vr_current
)