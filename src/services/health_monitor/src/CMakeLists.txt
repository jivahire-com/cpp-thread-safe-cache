# define target for power service
set(TARGET kng.service.health_monitor)

add_library(${TARGET}
    STATIC
        hm_error_report_ghes.c
        hm_error_domain_register.c
        hm_error_report_cper.c
        hm_error_report_ue.c
        hm_error_injection.c
        hm_main.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.service.iccBase
        ms::lib::error_codes
    PRIVATE
        fwlib.lib.status
        fwlib.lib.init
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::spi_bridge
        silibs::semaphore
        ms::lib::crash_dump
        ms::lib::icc_platform_defines    
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::health_monitor ALIAS ${TARGET})
