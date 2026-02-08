# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET d2d_cntr_sync_init.test)

set(SOURCES
    test_d2d_cntr_sync_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/d2d_cntr_sync_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.utils
        fwlib.lib.init
        idhw
        ms::cmocka::testrunner
        ms::lib::boot_status
        ms::lib::d2d_cntr_sync
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

#mock the functions needed
mock_target(${TARGET}
    d2d_cntr_sync_init
    idsw_get_die_id
    idsw_get_platform_sdv
    idsw_get_cpu_type
    boot_status_notify_extd
)
