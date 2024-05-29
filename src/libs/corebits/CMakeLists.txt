set (TARGET kng.lib.corebits)

# Add target for the service
add_library(
    ${TARGET} STATIC
        corebits.c
)

target_include_directories(
    ${TARGET} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::corebits ALIAS ${TARGET})
