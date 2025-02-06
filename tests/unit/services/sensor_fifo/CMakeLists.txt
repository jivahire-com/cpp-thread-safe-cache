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

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_link_libraries(${TARGET}
        ${LIBS}
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.sensor_fifo,INCLUDE_DIRECTORIES>)
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.driver.sensor_fifo,INCLUDE_DIRECTORIES>)


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
mock_target(${TARGET}
    DfwkClientInterfaceOpen
    FpFwAssert
    DfwkDeviceInitialize
    DfwkInterfaceInitialize
    DfwkInterfaceSendSync
    sensor_fifo_driver_read_entry
    sensor_fifo_driver_write_entries
    fpfw_icc_base_send_sync
    fpfw_icc_base_recv
    FpFwLockAcquire
    FpFwLockRelease
    sensor_fifo_driver_inf_sync_fifo_enables
)
