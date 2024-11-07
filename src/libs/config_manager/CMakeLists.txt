set(TARGET kng.lib.config_manager)

add_library(${TARGET}
    STATIC
        src/config_manager.c
        src/config_manager_hsp.c
)

target_link_libraries(${TARGET}
    PUBLIC 
    fwlib.lib.cfg_mgr
    PRIVATE
    kng_silibs_platform
    silibs::platform
    fwlib.lib.status
    fwlib.lib.utils
    fwlib.lib.spinlock
    ms::lib::error_codes
    ms::lib::crash_dump
    ms::lib::variable_services
    ms::lib::system_info
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::config_manager ALIAS ${TARGET})
