# define target for power service
set(TARGET kng.service.power_service)

add_library(${TARGET}
    STATIC
        power_main.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.platform.errorhandler
        fwlib.platform.threadx
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::power ALIAS ${TARGET})
