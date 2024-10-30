set(TARGET  kng.service.telemetry.cmpnt.in_band_tlm.test)

set(SOURCES
    in_band_mocks.c
    test_create_package.cpp
    test_dcs_manager.cpp
    test_ddr_manager.cpp
    test_in_band_cmpnt.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    kng.service.telemetry.cmpnt.in_band_tlm
    fwlib.lib.status
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::service::sensor_fifo

)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.telemetry,INCLUDE_DIRECTORIES>)
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.telemetry.cmpnt.in_band_tlm,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    _tx_time_get
    _txe_queue_create
    _txe_queue_send
    _txe_queue_receive
    _txe_queue_flush
    FpFwAssertWithArgs
)