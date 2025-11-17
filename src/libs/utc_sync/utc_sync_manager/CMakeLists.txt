#
# Copyright (c) Microsoft Corporation. All rights reserved.
#
set (TARGET kng.lib.utc_sync_manager)

add_library(${TARGET} STATIC
        utc_sync_manager_mctp.c
        utc_sync_manager_mctp_events.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        ms::lib::gtimer
        ms::lib::event_trace::trace
        fwlib.lib.interface.platform.threadx
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.status
        fwlib.lib.assert
        fwlib.lib.utils
        fwlib.service.mts
        fwlib.service.utc_sync.manager
        fwlib.lib.mctp
    PUBLIC
        fwlib.lib.interface.utc_sync.common
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::utc_sync_manager ALIAS ${TARGET})