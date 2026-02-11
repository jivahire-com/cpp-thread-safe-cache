# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET warm_start_test.test)

set(SOURCES
    warm_start_mocks.cpp
    warm_start_test.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

# Get private includes for module under test
target_include_directories(${TARGET}
PRIVATE
    ${WARM_START_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.interface.platform.interrupts
        ms::cmocka::testrunner
        ms::lib::warm_start
        silibs::kng_silibs_platform
    PUBLIC
        fwlib.lib.assert
        fwlib.lib.utils
 )

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    FpFwLockInitialize
    FpFwLockAcquire
    FpFwLockRelease
)