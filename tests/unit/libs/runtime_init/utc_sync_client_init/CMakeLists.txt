#
# Copyright (c) Microsoft Corporation. All rights reserved.
#

set(TARGET utc_sync_client_init.test)

set(SOURCES
    test_utc_sync_client_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/utc_sync_client_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.status
        fwlib.lib.utils
        fwlib.service.iccBase
        fwlib.service.mts
        fwlib.service.utc.cli
        fwlib.service.utc_sync.client
        ms::cmocka::testrunner
        ms::lib::boot_status
        ms::lib::crash_dump
        ms::lib::gtimer
        ms::lib::hsp_firmware_headers
        silibs::idhw
        ms::lib::utils
)

# Setup the MCP Runtime Init Flag as it covers all code paths.
target_compile_definitions(${TARGET}
    PRIVATE
       MCP_RUNTIME_INIT
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
    fpfw_icc_base_send_sync
    fpfw_init_get_handle
    gtimer_prodfw_get_frequency
    utc_cli_svc_initialize
    utc_sync_client_init
    boot_status_notify_extd
    idsw_get_cpu_type
    idsw_get_die_id
)
