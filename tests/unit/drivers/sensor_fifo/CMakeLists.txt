# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.driver.sensor_fifo.test)

set(SOURCES
    test_sensor_fifo_driver.cpp
    test_hw_fifo.cpp
    sensor_fifo_mock.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    kng.driver.sensor_fifo
    kng.device.scf_mhu

    fwlib.dfwk
    fwlib.lib.assert
    fwlib.platform.dfwkhost.threadx
    ms::client_interface::startup_shutdown
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::mocks::threadx
    silibs::kng_silibs_platform
    silibs_platform
    silibs::sensor_ram_bridge
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.driver.sensor_fifo,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    crash_dump_bug_check
    FpFwAssert
    DfwkAsyncRequestComplete
    DfwkDeviceInitialize
    DfwkInterfaceInitialize
    DfwkInterfaceSendSync
    DfwkQueueInitialize
    mmio_read32
    mmio_write32
    mmio_update32
    mmio_read64
    mmio_write64
    scf_trigger_start
    scf_trigger_stop
    init_scf_mhu
)
