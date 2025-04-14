set(TARGET kng.lib.cxl)
add_library(${TARGET} STATIC cxl.c)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        silibs::idsw
        silibs::idsw_kng
        ms::service::atu_service
        fwlib.lib.cfg_mgr
)

add_library(ms::lib::cxl ALIAS ${TARGET})

# Enable linting, formatting, and coverage
lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})