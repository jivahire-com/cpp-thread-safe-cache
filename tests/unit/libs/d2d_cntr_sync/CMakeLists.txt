# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET d2d_cntr_sync.test)

set(SOURCES
    test_d2d_cntr_sync.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.cfg_mgr
        fwlib.lib.status
        fwlib.lib.utils
        ms::cmocka::testrunner
        silibs::d2d_counter_sync
        ms::lib::d2d_cntr_sync
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    d2d_counter_sync_configure_d1_params
    d2d_counter_sync_configure_d0_and_enable_sync
    d2d_counter_sync_enable
    idsw_get_platform_sdv
    idhw_is_single_die_boot_en
    crash_dump_bug_check
    config_get_d2d_counter_sync_knobs
    config_get_d2d_counter_sync_enable_die1
)