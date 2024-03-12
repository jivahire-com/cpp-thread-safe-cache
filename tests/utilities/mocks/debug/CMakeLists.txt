set(TARGET kng.mock.debug)

add_library(${TARGET} 
    STATIC 
        debug_mocks.c
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
    PRIVATE
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::debug ALIAS ${TARGET})