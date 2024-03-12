set(TARGET kng.mock.threadx)

add_library(${TARGET} 
    STATIC 
        cortex_m7_mocks.c
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        ms::cmocka::mocks
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::cortex_m7 ALIAS ${TARGET})
