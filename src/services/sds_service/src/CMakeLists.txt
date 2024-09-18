# define target for power service
set(TARGET kng.service.sds_service)

add_library(${TARGET}
    STATIC
        sds_main.c
        sds_api_impl.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
        fwlib.lib.status
        fwlib.lib.spinlock
        fwlib.lib.init
        ms::lib::error_codes
        ms::lib::crash_dump
        ms::service::atu_service
        silibs::idsw
        silibs::idsw_kng
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::sds_service ALIAS ${TARGET})
