set(TARGET kng.lib.boot_status)

add_library(${TARGET}
    STATIC
        boot_status.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.interface.platform.dbgprint
        ms::lib::crash_dump
        ms::lib::system_info
        silibs::idsw_kng
    PUBLIC
        fwlib.service.iccBase
        ms::lib::hsp_firmware_headers
        
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::boot_status ALIAS ${TARGET})
