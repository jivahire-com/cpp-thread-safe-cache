set(TARGET service.sensor_fifo.test)

set(SOURCES
    test_sensor_fifo_service.cpp
    sensor_fifo_service_mock.c
)

set(LIBS
    PRIVATE
        kng.service.sensor_fifo
        ms::cmocka::mocks
        ms::cmocka::testrunner

)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
        ${LIBS}
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.sensor_fifo,INCLUDE_DIRECTORIES>)

# Defining platform APIs
# set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
# set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.interface.win32")
# set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
mock_target(${TARGET}
    DfwkClientInterfaceOpen
    FpFwAssert
)
