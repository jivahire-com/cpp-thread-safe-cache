set(TARGET kng.lib.boot_status)

add_library(${TARGET}
    STATIC
        boot_status.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        ms::lib::hsp_firmware_headers
        ms::lib::crash_dump
        ms::lib::system_info
    PUBLIC
        fwlib.service.iccBase
        silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::boot_status ALIAS ${TARGET})
