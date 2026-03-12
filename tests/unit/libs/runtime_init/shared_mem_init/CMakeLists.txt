# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET arsm_init.test)

set(SOURCES
    test_shared_mem_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/shared_mem_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::boot_status
        ms::lib::system_info
        ms::service::atu_service
        silibs_platform
        silibs::idsw
        silibs::atu
        silibs::idsw_kng
        ms::lib::utils
)

# Platform ports
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    system_info_is_warm_start
    idsw_get_die_id
    idsw_get_platform_sdv
    memset
    atu_map
    atu_unmap
    boot_status_notify_extd
    idsw_get_cpu_type
)
