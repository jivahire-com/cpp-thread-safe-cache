set(TARGET kng.lib.variable_services)

set (SOURCES 
    variable_services_sync.c
    variable_services_helper.c
)
add_library(${TARGET}  STATIC ${SOURCES})

target_link_libraries(
    ${TARGET}
        PUBLIC
            fwlib.lib.status
        PRIVATE
            fwlib.service.iccBase
            kng_silibs_platform
            ms::lib::crash_dump
            ms::lib::error_codes
            ms::lib::hsp_firmware_headers
            ms::lib::system_info
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::variable_services ALIAS ${TARGET})
