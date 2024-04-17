
set(TARGET stdio_textio)

set(SRC
    ./stdio_textio_mocks.c
)

set(LIBS 
        PRIVATE
            fwlib.DriverInterface.TextIo
            fwlib.utils
            ms::cmocka::mocks
)

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::stdio_textio ALIAS ${TARGET})
