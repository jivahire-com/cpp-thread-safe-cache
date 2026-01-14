# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET ioss_lib.test)

set(SOURCES
    ioss_mocks.c
    ioss_unit_tests.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::cmocka::mocks
        ms::lib::ioss_ini
        ms::lib::error_handler
        silibs_platform
        silibs::kng_silibs_platform
        silibs::ioss_init
        fwlib.lib.assert
        fwlib.lib.utils
        fwlib.lib.cfg_mgr
        silibs::atu
        silibs::ioss_init
        silibs::usb_init
        ms::lib::variable_services
        ms::lib::error_codes
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    ioss_init
    crash_dump_bug_check
    variable_service_initialize_ctx
    variable_service_sync_set_variable
)