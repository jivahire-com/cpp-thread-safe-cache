enabled_on(arm-eabi-aarch)
set (TARGET arm_utils)

# Add target for the service
add_library(
    ${TARGET} STATIC
        arm_utils.c
)

target_include_directories(
    ${TARGET} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::arm_utils ALIAS ${TARGET})
