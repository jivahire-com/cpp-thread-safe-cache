# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.platform.impl.mts.test)

set(SOURCES
    mts_platform_mock.c
    test_mts_platform.cpp
    test_mts_manifest.cpp
)


set(LIBS
    PRIVATE
        ms::service::mts
        ms::cmocka::mocks
        ms::cmocka::testrunner
        in_band_telemetry_ddr
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
        ${LIBS}
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::service::mts,INCLUDE_DIRECTORIES>)


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
mock_target(${TARGET}
    transfer_rly_get_this_die_id
    transfer_rly_should_send_dcp_msg
    transfer_rly_should_send_trp_msg
    fpfw_icc_base_send_sync
    FpFwAssert
    FpFwAssertWithArgs
    _tx_thread_sleep
)
