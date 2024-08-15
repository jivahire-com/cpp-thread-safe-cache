set(TARGET kng.lib.mesh)

add_library(${TARGET}
    STATIC
        src/mesh.c
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.service.iccBase
    silibs_platform
    ms::lib::crash_dump::scp
    ms::lib::error_codes
    ms::lib::hsp_firmware_headers
    ms::lib::register_headers
    ms::lib::system_info
    silibs::cmn800
    silibs::cmn800_skus
    silibs::cmn800_sequence
    silibs::idhw
    silibs::idsw
    silibs::idsw_kng
    silibs::kng_silibs_platform
    silibs::ppu_v1
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::mesh ALIAS ${TARGET})
