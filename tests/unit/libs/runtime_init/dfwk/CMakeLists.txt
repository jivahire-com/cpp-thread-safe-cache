# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET dfwk_init.test)

set(SOURCES
    test_dfwk_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/dfwk_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.utils
        ms::lib::threadx
        ms::cmocka::testrunner
)

# Generic threadx lock port provided by fwlibs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_LOCK "fwlib.lib.platform.impl.lock.threadx")

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkThreadxHostInitialize
)
