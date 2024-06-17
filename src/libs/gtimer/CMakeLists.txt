set(TARGET kng.lib.gtimer)

add_library(${TARGET}
    STATIC
        gtimer.c
)

target_link_libraries(${TARGET}
    PRIVATE
    ms::lib::register_headers
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::gtimer ALIAS ${TARGET})
