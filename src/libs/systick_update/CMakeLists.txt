# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET systick_update)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
            systick_update.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
        PRIVATE
            
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            ms::cmsis::interface
            silibs::idsw
            silibs::idsw_kng
            silibs::platform
            ms::lib::error_codes
)

target_compile_definitions(
    ${TARGET} PRIVATE
        TX_TIMER_TICKS_PER_SECOND=${TX_CORE_TICKS_PER_SECOND}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::systick_update ALIAS ${TARGET})
