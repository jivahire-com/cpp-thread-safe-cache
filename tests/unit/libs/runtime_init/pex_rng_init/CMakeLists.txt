# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET pex_rng_init.test)

set(SOURCES
    test_pex_rng_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/pex_rng_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
PRIVATE
fwlib.dfwk
fwlib.lib.init
fwlib.lib.utils
fwlib.lib.interface.platform.dbgprint
ms::cmocka::testrunner
ms::lib::boot_status
ms::lib::pex_rng
ms::lib::core_info
ms::lib::corebits
ms::lib::crash_dump
ms::lib::ift
ms::lib::register_headers
ms::lib::system_info
ms::service::atu_service
silibs::atu
silibs::crypto::rng
silibs::idsw_kng
silibs_platform
ms::lib::utils
)


# # Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    core_info_get_enable_cores_result
    system_info_is_warm_start
    register_pex_error_domain
    init_pex_rng
    ift_is_enabled
    _tx_timer_create
    _txe_timer_create
    crash_dump_bug_check
    hm_register_error_domain
    boot_status_notify_extd
    idsw_get_cpu_type
    idsw_get_die_id
)
