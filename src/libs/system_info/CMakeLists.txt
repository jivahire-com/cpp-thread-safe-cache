set(TARGET kng.lib.system_info)

add_library(${TARGET}
    STATIC
        system_info.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        ms::lib::register_headers
        ms::lib::hsp_firmware_headers
        silibs::kng_silibs_platform
        silibs::platform
    PUBLIC
        fwlib.service.iccBase
        silibs::idsw_kng
        silibs::platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::system_info ALIAS ${TARGET})
