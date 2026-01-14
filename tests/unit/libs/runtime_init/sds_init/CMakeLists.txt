# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET sds_init.test)

set(SOURCES
    test_sds_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/sds_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.linkedlist
        fwlib.lib.utils
        fwlib.lib.status
        fwlib.lib.spinlock
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::service::sds_service
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    sds_init
    sds_interface_init
)
