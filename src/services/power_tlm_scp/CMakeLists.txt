#
#  Copyright (c) Microsoft Corporation. All rights reserved.
#
set(TARGET kng.service.pwr_tlm_scp)

add_library(${TARGET}
    STATIC
        pwr_tlm_service_scp_events.c
        pwr_tlm_service_scp.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PUBLIC
        IFPFW.EventTracing
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.status
        fwlib.lib.utils
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
        kng_silibs_platform
        ms::lib::event_trace::trace
        ms::lib::pwr_tlm_core_ex
        ms::service::mts
        ms::service::power
)

add_library(ms::service::pwr_tlm_scp ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
