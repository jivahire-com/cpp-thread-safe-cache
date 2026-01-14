# Copyright (c) Microsoft Corporation. All rights reserved.

set (TARGET utils)

# Add target for the service
add_library(
    ${TARGET} STATIC
        utils.c
)

target_include_directories(
    ${TARGET} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    ${TARGET}
    PUBLIC
        ms::lib::arm_intrinsic
        ms::lib::corebits

)


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::utils ALIAS ${TARGET})
