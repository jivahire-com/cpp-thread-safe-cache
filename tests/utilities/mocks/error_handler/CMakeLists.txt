set(TARGET kng.fwlib.port.error_handler)

add_library(${TARGET} 
    STATIC 
    error_handler.c
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.platform.errorhandler
    PRIVATE
        ms::cmocka::mocks
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::error_handler ALIAS ${TARGET})
