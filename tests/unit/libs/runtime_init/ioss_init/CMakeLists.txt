# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET ioss_init.test)

set(SOURCES
    test_ioss_ini.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/ioss_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        silibs::idsw
        silibs::idsw_kng
        ms::cmocka::testrunner
        ms::lib::boot_status
        ms::lib::ioss_ini
        silibs_platform
        kng_silibs_platform
        fwlib.lib.assert
        pcr_ioss
        fwlib.lib.cfg_mgr
        silibs::atu
        silibs::ioss_init
        silibs::usb_init
        ms::lib::variable_services
        ms::lib::error_codes
        ms::service::atu_service
        ms::lib::crash_dump
        ms::lib::utils
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    ioss_ini
    idsw_get_die_id
    boot_status_notify_extd
    idsw_get_cpu_type
    idsw_get_die_id
)
