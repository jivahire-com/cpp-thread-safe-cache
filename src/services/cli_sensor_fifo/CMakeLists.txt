set(TARGET kng.service.sensor_fifo.cli)

add_library(${TARGET}
    STATIC
        sensor_fifo_cli_service.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
        fwlib.service.cli
        ms::driver::sensor_fifo
        ms::service::sensor_fifo
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::service::sensor_fifo::cli ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
