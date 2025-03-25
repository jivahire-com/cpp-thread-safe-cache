set(TARGET kng.service.wdog_service)

set (SOURCES 
        src/wdog_service.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::error_codes
    PRIVATE
        ms::lib::threadx
        silibs::cmsdk_wd
        silibs::kng_silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::wdog ALIAS ${TARGET})
