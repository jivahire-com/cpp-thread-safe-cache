# define target for power service
set(TARGET kng.service.health_monitor)

add_library(${TARGET}
    STATIC
        hm_error_report_ghes.c
        hm_error_domain_register.c
        hm_error_report_cper.c
        hm_error_report_ue.c
        hm_error_injection.c
        hm_error_domain_mcp.c
        hm_error_domain_accel.c
        hm_error_domain_hsp.c
        hm_main.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
        ${REPO_ROOT_DIR}/src/libs/icc_platform_defines
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.service.iccBase
        ms::lib::error_codes
        silibs::semaphore
        silibs::icc_mhu
    PRIVATE
        fwlib.lib.status
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::spi_bridge
        ms::lib::crash_dump
        ms::lib::icc_platform_defines
        ms::service::atu_service      
        kng.lib.hsp_firmware_headers 
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::health_monitor ALIAS ${TARGET})
