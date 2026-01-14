# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.pwr_tlm_mcp.cmpnt.exec)

add_library(${TARGET}
    STATIC
        exec_tlm_cmpnt.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PUBLIC

    PRIVATE
        pwr_tlm_mcp.service.cmpnt.interfaces
        fwlib.lib.assert
        fwlib.lib.status
        fwlib.lib.tmr_queue
        fwlib.lib.utils
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
        ms::lib::error_codes
        ms::lib::gtimer
        
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
