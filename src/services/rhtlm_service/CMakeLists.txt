set(TARGET kng.service.rhtlm_service)

set(SOURCES
    rhtlm_service.c
)

add_library(${TARGET}
    STATIC
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
    ms::lib::error_codes
    silibs::ddrss
    ms::lib::ddr_manager
    ms::lib::ddrss_init
    PRIVATE
    ms::lib::icc_platform_defines
    ms::lib::gtimer
    ms::lib::debug
    fwlib.lib.cfg_mgr
    fwlib.service.cli
    ms::lib::error_codes
    fwlib.lib.interface.platform.threadx
    fwlib.lib.interface.platform.errorhandler
    silibs::kng_silibs_platform
    silibs::idsw
    silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::service::rhtlm_service ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})