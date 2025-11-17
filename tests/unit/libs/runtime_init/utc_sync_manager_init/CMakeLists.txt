#
# Copyright (c) Microsoft Corporation. All rights reserved.
#

set(TARGET utc_sync_manager_init.test)

set(SOURCES
    test_utc_sync_manager_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/utc_sync_manager_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.mctp
        fwlib.lib.status
        fwlib.lib.utils
        fwlib.service.mts
        fwlib.service.utc_sync.manager
        ms::cmocka::testrunner
        ms::lib::gtimer
        ms::lib::utc_sync_manager
        silibs::idsw
        silibs::idsw_kng
        silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    fpfw_init_get_handle
    gtimer_prodfw_get_frequency
    idsw_get_die_id
    utc_sync_manager_init
    utc_sync_manager_mctp_init
)
