set(TARGET kng.lib.system_info)

add_library(${TARGET}
    STATIC
        system_info.c
)

target_link_libraries(${TARGET}
    PRIVATE
    ms::lib::register_headers
    ms::lib::hsp_firmware_headers
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::system_info ALIAS ${TARGET})
