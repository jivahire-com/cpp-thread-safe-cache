# define target for atu service
set(TARGET kng.service.atu_service)

add_library(${TARGET}
    STATIC
        atu_main.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::lib::css
        ms::lib::register_headers
        ms::lib::error_codes
        ms::lib::crash_dump
        silibs::kng_silibs_platform
        silibs_platform
        silibs::atu
        silibs::idsw
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::atu_service ALIAS ${TARGET})
