# Copyright (c) Microsoft Corporation. All rights reserved.

# HW semaphore
set(TARGET kng.lib.hw_semaphore)

set (SOURCES
    src/hw_semaphore.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::error_codes
        silibs::semaphore
    PRIVATE
        ms::service::atu_service
        silibs::idhw
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::hw_semaphore ALIAS ${TARGET})
